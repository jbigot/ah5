/*******************************************************************************
 * Copyright (c) 2013-2014, Julien Bigot - CEA (julien.bigot@cea.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of the <organization> nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ah5.h"


/** Maximum number of thread supported in the multiple thread copy
 *
 * \todo automatically detect the actual max number of thread or dynamically
 * detect it so as not to crash on MIC for example
 */
#define MAX_NB_THREAD 32


/** Maximum number of dimensions supported for arrays
 */
#define MAX_RANK 7


/** Represents an HDF5-write call to make
 */
typedef struct data_id {

	/** The data
	 */
	void* buf;

	/** Number of dimensions
	 */
	unsigned rank;

	/** Dimensions of the array
	 */
	hsize_t dims[MAX_RANK];

	/** Lower bound of the part to write
	 */
	hsize_t lbounds[MAX_RANK];

	/** Upper bound of the part to write
	 */
	hsize_t ubounds[MAX_RANK];

	/** Name of the Data
	 */
	char* name;

	/** HDF5 type of the Data
	 */
	hid_t type;

} data_id_t;


/** The various commands that can be issued to the writer thread
 */
typedef enum thread_command {
	CMD_WAIT, /**< nothing to do, just wait */
	CMD_WRITE, /**< start executing the command list */
	CMD_TERMINATE /**< stop execution */
} thread_command_t;


/** Status of the asynchronous HDF5 instance
 */
struct ah5 {

	/** a mutex controling access to this instance */
	pthread_mutex_t mutex;

	/** a condition variable used to signal that the command has changed */
	pthread_cond_t cond;

	/** the command to execute */
	thread_command_t thread_cmd;

	/** the thread executing the command list */
	pthread_t thread;

	/** the name of the file to write */
	char* file_name;

	/** The buffer where the data is copied */
	void* data_buffer;

	/** The actual command list */
	data_id_t* data;

	/** The number of commands in the list */
	size_t data_size;

	/** the file where to log */
	FILE* log_file;

	/** the verbosity level */
	int log_verbosity;

	/** whether to write scalars as a 1D size 1 array */
	int scalar_as_array;

	/** whether to use all core for copies */
	int parallel_copy;

};


#define LOG_ERROR( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_ERROR) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Error: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)



#define LOG_WARNING( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_WARNING) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Warning: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)


#define LOG_STATUS( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_STATUS) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Status: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)


#ifndef NDEBUG
#define LOG_DEBUG( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_DEBUG) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Log: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)
#else
#define LOG_DEBUG( ... )
#endif


#define SIGNAL_ERROR do {\
	int errno_save = errno;\
	LOG_ERROR(" ----- Fatal: exiting -----\n");\
	errno = errno_save;\
	perror(NULL);\
	exit(errno_save);\
} while (0)


#define RETURN_ERROR do {\
	int errno_save = errno;\
	LOG_WARNING(" ----- Leaving function -----\n");\
	errno = errno_save;\
	perror(NULL);\
	errno = errno_save;\
	return errno_save;\
} while (0)


/** Returns the number of microseconds elapsed since EPOCH
 * @returns the number of microseconds elapsed since EPOCH
 */
inline static int64_t clockget()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t)tv.tv_sec*1000*1000 + tv.tv_usec;
}

#if H5_VERS_MAJOR >= 1 && H5_VERS_MINOR >= 8 && H5_VERS_RELEASE >= 14
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_ID_g
#else
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_g
#endif

/** The function executed by the writer thread
 * @param self_void a pointer to the instance state as a void*
 * @returns NULL
 */
static void* writer_thread_loop( void* self_void )
{
	ah5_t self = self_void;
	if ( pthread_mutex_lock(&(self->mutex)) ) SIGNAL_ERROR;
	LOG_STATUS("async HDF5 thread started");
	for (;;) {
		int64_t start_time;
		hid_t file_id;
		size_t did;

		/* when the command is to wait ... wait for it to change */
		while ( self->thread_cmd ==  CMD_WAIT ) {
			if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) SIGNAL_ERROR;
		}
		LOG_DEBUG("async HDF5 thread executing command");
		/* if the command is to stop ... stop */
		if ( self->thread_cmd == CMD_TERMINATE ) {
			LOG_DEBUG("async HDF5 thread executing terminate command");
			pthread_mutex_unlock(&(self->mutex));
			return NULL;
		}
		/* otherwise, the command is to execute the write list */
		assert( self->thread_cmd == CMD_WRITE );
		LOG_DEBUG("async HDF5 thread executing write command");

		start_time = clockget();
		file_id = H5Fcreate( self->file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
		
		for ( did=0; did<self->data_size; ++did ) {
			hid_t space_id, plist_id, dset_id;
			LOG_DEBUG("async HDF5 writing data[%lu]: %s of rank %u", (unsigned long)did, self->data[did].name, (unsigned)self->data[did].rank);
			space_id = H5Screate_simple(self->data[did].rank, self->data[did].dims, NULL);
			plist_id = H5Pcreate(CLS_DSET_CREATE);
			if ( H5Pset_layout(plist_id, H5D_CONTIGUOUS) ) SIGNAL_ERROR;
#if ( H5Dcreate_vers == 2 )
			dset_id = H5Dcreate2(file_id, self->data[did].name, self->data[did].type,
					space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
#else
			dset_id = H5Dcreate( file_id, self->data[did].name, self->data[did].type,
					space_id, H5P_DEFAULT);
#endif
			if ( H5Dwrite(dset_id, self->data[did].type, H5S_ALL, H5S_ALL, H5P_DEFAULT,
					self->data[did].buf) ) SIGNAL_ERROR;
			if ( H5Dclose(dset_id) ) SIGNAL_ERROR;
			if ( H5Pclose(plist_id) ) SIGNAL_ERROR;
			if ( H5Sclose(space_id) ) SIGNAL_ERROR;
		}

		LOG_DEBUG("async HDF5 closing file");
		if ( H5Fclose(file_id) ) SIGNAL_ERROR;
		LOG_DEBUG("async HDF5 write duration: %" PRId64 "us", clockget()-start_time);

		/* once the write list has been fully executed, tell oneself to wait for the next one */
		self->thread_cmd = CMD_WAIT;
		/* and wake up the main thread potentially waiting for us */
		if ( pthread_cond_signal(&(self->cond)) ) SIGNAL_ERROR;
	}
}


/** This function has the exact same prototype as memcpy and does the exact same thing,
 * except it does it in parallel
 * @param dest the destination of the copy
 * @param src the source of the copy
 * @param size the number of bytes to copy
 * @return a pointer to dest
 */
static void* memcpy_omp( void* dest, void* src, size_t size )
{
#ifdef _OPENMP
	size_t thread_disp[MAX_NB_THREAD+1];
	int nbthread = omp_get_num_threads();
	int tid;
	if ( nbthread > MAX_NB_THREAD ) nbthread = MAX_NB_THREAD;
	thread_disp[0] = 0;
	for ( tid = 0; tid<nbthread; ++tid ) {
		size_t thread_size = size / (4*1024); /* size in ~pages (4k) */
		/* get a fair share of pages (rounded above) amongst remaining threads */
		thread_size = ( (thread_size+nbthread-tid-1) / (nbthread-tid) );
		thread_size *= (4*1024); /* get back as Bytes */
		size -= thread_size; /* remove from the size remaining to write */
		if ( size < (4*1024) ) { /* if the remaining size is not a full page */
			thread_size += size; /* write it */
			size = 0;
		}
		thread_disp[tid+1] = thread_disp[tid]+thread_size;
	}
	assert(size==0);
	#pragma omp parallel for schedule(static)
	for (tid = 0; tid<nbthread; ++tid ) {
		memcpy(((char*)dest)+thread_disp[tid], ((char*)src)+thread_disp[tid],
				thread_disp[tid+1]-thread_disp[tid]);
	}
	return dest;
#else
	return memcpy(dest, src, size);
#endif
}


/** Copies a slice of a nD array (in fact a block) from src to dest.
 * @param dest the destination of the block (contiguous)
 * @param src the source array where the block is
 * @param type the type of elements in the array
 * @param rank the number of dimensions of the array
 * @param sizes the sizes of the array in each dimension
 * @param lbounds the lower bounds of the block in each dimension
 * @param ubounds the upper bounds of the block in each dimension
 * @param parallelism whether to do the copy in parallel
 * @return dest
 */
static void* slicecpy( void* dest, void* src, hid_t type, unsigned rank, hsize_t* sizes,
		hsize_t* lbounds, hsize_t* ubounds, int parallelism )
{
	size_t ii, src_block_size, dst_block_size;

	if ( rank == 0 ) {
		if ( parallelism ) {
			memcpy_omp(dest, src, H5Tget_size(type));
		} else {
			memcpy(dest, src, H5Tget_size(type));
		}
		return dest;
	}

	src_block_size = 1;
	dst_block_size = 1;
	for ( ii=1; ii<rank; ++ii ) {
		src_block_size *= sizes[ii];
		dst_block_size *= (ubounds[ii]-lbounds[ii]);
	}

	if ( src_block_size == dst_block_size ) {
		if ( parallelism ) {
			memcpy_omp(dest, ((char*)src)+src_block_size*lbounds[0]*H5Tget_size(type),
					H5Tget_size(type)*src_block_size*(ubounds[0]-lbounds[0]) );
		} else {
			memcpy(dest, ((char*)src)+src_block_size*lbounds[0]*H5Tget_size(type),
					H5Tget_size(type)*src_block_size*(ubounds[0]-lbounds[0]) );
		}
	} else {
		if ( parallelism ) {
			#pragma omp for
			for ( ii = lbounds[0]; ii<ubounds[0]; ++ii ) {
				slicecpy(
						((char*)dest)+dst_block_size*(ii-lbounds[0])*H5Tget_size(type),
						((char*)src)+src_block_size*ii*H5Tget_size(type),
						type, rank-1, sizes+1, lbounds+1, ubounds+1,
						0 );
			}
		} else {
			for ( ii = lbounds[0]; ii<ubounds[0]; ++ii ) {
				slicecpy(
						((char*)dest)+dst_block_size*(ii-lbounds[0])*H5Tget_size(type),
						((char*)src)+src_block_size*ii*H5Tget_size(type),
						type, rank-1, sizes+1, lbounds+1, ubounds+1,
						0 );
			}
		}
	}
	return dest;
}


/** Waits for the worker thread to finish its previous run
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 */
inline static int writer_thread_wait( ah5_t self )
{
	size_t ii;
	/* wait for the writer thread */
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	/* wait for a potential previous write command to be executed */
	while ( self->thread_cmd ==  CMD_WRITE ) {
		LOG_STATUS("waiting for writer thread");
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
	}
	/* free the previously allocated memory for field names */
	for ( ii = 0; ii<self->data_size; ++ii ) {
		free(self->data[ii].name);
	}
	/* reset the number of records */
	self->data_size = 0;
	return 0;
}


int ah5_init( ah5_t* pself )
{
	ah5_t self = malloc(sizeof(struct ah5));
	if ( H5open() ) RETURN_ERROR;
	self->file_name = NULL;
	self->log_file = NULL;
	self->log_verbosity = VERBOSITY_WARNING;
	self->thread_cmd = CMD_WAIT;
	self->data_buffer = NULL;
	self->data = NULL;
	self->data_size = 0;
	self->scalar_as_array = 1;
	self->parallel_copy = 1;
	if ( pthread_mutex_init(&(self->mutex), NULL) ) RETURN_ERROR;
	if ( pthread_cond_init(&(self->cond), NULL) ) RETURN_ERROR;
	if ( pthread_create(&(self->thread), NULL, writer_thread_loop, self) ) RETURN_ERROR;
	LOG_STATUS("initialized Async HDF5 instance");
	*pself = self;
	return 0;
}


int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->log_verbosity = log_lvl;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_set_logfile( ah5_t self, char* log_file )
{
	int log_fd;
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	if ( self->log_file ) fclose(self->log_file);
	log_fd = open(log_file, O_WRONLY|O_APPEND|O_CREAT|O_SYNC|O_DSYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if ( log_fd == -1 ) RETURN_ERROR;
	self->log_file = fdopen(log_fd, "a");
	if ( !self->log_file ) RETURN_ERROR;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_set_scalarray( ah5_t self, int scalar_as_array )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->scalar_as_array = scalar_as_array;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_set_paracopy( ah5_t self, int parallel_copy )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->parallel_copy = parallel_copy;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_finalize( ah5_t self )
{
	/* wait for the writer thread to finish its work */
	if ( writer_thread_wait(self) ) RETURN_ERROR;
	/* tell the writer thread to terminate */
	self->thread_cmd = CMD_TERMINATE;
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	/* wait for the writer thread to terminate */
	if ( pthread_join( self->thread, NULL) ) RETURN_ERROR;
	/* free all memory */
	free(self->data);
	free(self->data_buffer);
	free(self->file_name);
	LOG_STATUS("finalized Async HDF5 instance");
	if ( self->log_file ) fclose(self->log_file);
	free(self);
	return 0;
}


int ah5_start( ah5_t self, char* file_name )
{
	/* wait for the writer thread to finish its work */
	if ( writer_thread_wait(self) ) RETURN_ERROR;
	LOG_DEBUG("starting new command list");
	/* store the file name */
	self->file_name = realloc(self->file_name, strlen(file_name)+1);
	strcpy(self->file_name, file_name);
	return 0;
}


int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
		hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds )
{
	int ii;
	LOG_DEBUG("adding a write command to the list");
	/* increase the array containing all write commands */
	++self->data_size;
	self->data = realloc(self->data, (self->data_size)*sizeof(data_id_t));
	if ( self->scalar_as_array ) {
		/* replace scalars by rank 1 array */
		hsize_t hsize_zero = 0;
		hsize_t hsize_one = 1;
		if ( rank == 0 ) {
			rank = 1;
			dims = &hsize_one;
			lbounds = &hsize_zero;
			ubounds = &hsize_one;
		}
	}
	/* save the info in the last element of the array */
	self->data[self->data_size-1].buf = data;
	self->data[self->data_size-1].rank = rank;
	for ( ii = 0; ii<rank; ++ii ) {
		self->data[self->data_size-1].dims[ii] = dims[ii];
		self->data[self->data_size-1].lbounds[ii] = lbounds[ii];
		self->data[self->data_size-1].ubounds[ii] = ubounds[ii];
	}
	self->data[self->data_size-1].name = malloc(strlen(name)+1);
	strcpy(self->data[self->data_size-1].name, name);
	self->data[self->data_size-1].type = type;
	LOG_DEBUG("added writing command for data[%lu]: %s of rank %u", (unsigned long)self->data_size-1, self->data[self->data_size-1].name, (unsigned)self->data[self->data_size-1].rank);
	return 0;
}


int ah5_finish( ah5_t self )
{
	size_t buf_size = 0;
	size_t ii;
	void* buf;
	int64_t start_time = clockget();
	LOG_DEBUG("sealing write command list");
	/* compute the total size of the data */
	for ( ii = 0; ii<self->data_size; ++ii ) {
		size_t data_size = H5Tget_size(self->data[ii].type);
		unsigned dim;
		for ( dim = 0; dim < self->data[ii].rank; ++dim ) {
			/* ubounds is just after the data, so the difference with lbound is the size */
			data_size *= self->data[ii].ubounds[dim]-self->data[ii].lbounds[dim];
		}
		buf_size += data_size;
	}
	/* allocate a buffer able to contain it all */
	self->data_buffer = realloc(self->data_buffer, buf_size);
	/* copy the data into the bufer */
	buf = self->data_buffer;
	for ( ii = 0; ii<self->data_size; ++ii ) {
		size_t data_size;
		unsigned dim;
		int64_t start_time = clockget();
		slicecpy(buf, self->data[ii].buf, self->data[ii].type, self->data[ii].rank, self->data[ii].dims,
						self->data[ii].lbounds, self->data[ii].ubounds, self->parallel_copy);
		data_size = H5Tget_size(self->data[ii].type);
		/* since only the data has been copied, update dims, lbounds & ubounds */
		for ( dim = 0; dim<self->data[ii].rank; ++dim ) {
			data_size *= self->data[ii].ubounds[dim]-self->data[ii].lbounds[dim];
			self->data[ii].dims[dim] = self->data[ii].ubounds[dim] - self->data[ii].lbounds[dim];
			self->data[ii].ubounds[dim] -= self->data[ii].lbounds[dim];
			self->data[ii].lbounds[dim] = 0;
		}
		self->data[ii].buf = buf;
		buf = ((char*)buf) + data_size;
		LOG_DEBUG("copy duration: %" PRId64 "us", clockget()-start_time);
	}
	/* wake up the writer thread */
	self->thread_cmd = CMD_WRITE;
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	LOG_STATUS("command list finished, triggering worker thread");
	LOG_DEBUG("command list sealing duration: %" PRId64 "us", clockget()-start_time);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}

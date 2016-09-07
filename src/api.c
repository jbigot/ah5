/*******************************************************************************
 * Copyright (c) 2013-2016, Julien Bigot - CEA (julien.bigot@cea.fr)
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

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#ifndef NO_MALLOC_USABLE
#include <malloc.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ah5.h"
#include "ah5_impl.h"
#include "logging.h"

#ifndef NO_MALLOC_USABLE
#define MALL_SZ(BUF, ALLSZ) \
	malloc_usable_size(BUF);
#else
	ALLSZ
#endif

#if H5_VERS_MAJOR >= 1 && H5_VERS_MINOR >= 8 && H5_VERS_RELEASE >= 14
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_ID_g
#else
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_g
#endif


/** Grows the memory buffer similarly to realloc, but disards the currently
 * held data.
 * 
 * @param self a pointer to the instance state
 * @param size the requested size for the buffer
 * @returns 0 on success, non-null on error
 */
static int growbuffer(ah5_t self, size_t size)
{
	if ( size > self->data_buffer_size ) {
		LOG_DEBUG("Growing buffer size");
		freebuffer(self);
		self->data_buffer = malloc(size);
		self->data_buffer_size = MALL_SZ(self->data_buffer, size);
	}
}


static int freebuffer( ah5_t self )
{
	free(self->data_buffer);
	self->data_buffer_size = 0;
	return 0;
}


int ah5_init( ah5_t* pself )
{
	if ( H5open() ) RETURN_ERROR;
	ah5_t self = malloc(sizeof(struct ah5));
	self->data_buf.content = NULL;
	self->data_buf.max_size = 0;
	self->data_buf.strategy = BUF_BASE;
	self->data_buf.used_size = 0;
	self->commands.content = NULL;
	self->commands.file = 0;
	self->commands.max_size = 0;
	self->commands.used_size = 0;
	self->logging.file = stderr;
	self->logging.closing_strategy = FILE_KEEP_OPEN;
	self->logging.verbosity = AH5_VERBOSITY_ERROR;
	if ( pthread_mutex_init(&(self->mutex), NULL) ) RETURN_ERROR;
	if ( pthread_cond_init(&(self->cond), NULL) ) RETURN_ERROR;
	if ( pthread_create(&(self->thread), NULL, writer_thread_loop, self) ) RETURN_ERROR;
	self->parallel_copy = 1;
	LOG_STATUS("initialized Async HDF5 instance");
	*pself = self;
	return 0;
}






int ah5_init_file( ah5_t* pself, const char *dirname, size_t max_size )
{
	if ( ah5_init(pself) ) return 1;
	ah5_t self = *pself;
	self->data_buf.strategy |= BUF_MMAPED;
	if ( max_size ) {
		size_t pagesize = sysconf(_SC_PAGE_SIZE);
		self->data_buf.max_size = ( max_size / pagesize ) * pagesize;
		int fd = open(filename, O_CREAT|O_NOATIME|O_TMPFILE|O_RDWR|O_TRUNC);
		self->data_buf.content = mmap(NULL, self->data_buf.max_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_HUGETLB, fd, 0);
	} else {
		self->data_buf.strategy |= BUF_DYNAMIC;
		size_t pagesize = sysconf(_SC_PAGE_SIZE);
		self->data_buf.max_size = pagesize;
		int fd = open(filename, O_CREAT|O_NOATIME|O_TMPFILE|O_RDWR|O_TRUNC);
		self->data_buf.content = mmap(NULL, self->data_buf.max_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_HUGETLB, fd, 0);
	}
	return 0;
}


int ah5_init_mem( ah5_t* pself, void *buffer, size_t max_size )
{
	if ( ah5_init(pself) ) return 1;
	ah5_t self = *pself;
	self->data_buf.strategy |= BUF_MALLOCED;
	if ( max_size ) {
		self->data_buf.max_size = max_size;
		self->data_buf.content = buffer;
	} else {
		self->data_buf.strategy |= BUF_DYNAMIC;
	}
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


int ah5_set_paracopy( ah5_t self, int parallel_copy )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->parallel_copy = parallel_copy;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_open( ah5_t self, char* file_name )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	// Wait for the commmand list to be empty
	while ( self->commands ) {
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
	}
	LOG_DEBUG("async HDF5 opening file");
	self->file = H5Fcreate( file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
	pthread_mutex_unlock(&(self->mutex));
	return 0;
}

int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
				hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds )
{
	
}



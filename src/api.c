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

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>

#include "ah5.h"
#include "ah5_impl.h"
#include "logging.h"
#include "memhandling.h"
#include "command_list.h"
#include "runner_thread.h"


int ah5_init_base( ah5_t* pself )
{
	ah5_t self = malloc(sizeof(struct ah5_s));
	if ( H5open() ) RETURN_ERROR(self->logging);
	self->thread_stop = 0;
	self->parallel_copy = 1;
	cl_init(&self->commands);
	buf_init_empty(&self->data_buf);
	log_init(&self->logging);
	if ( pthread_mutex_init(&(self->mutex), NULL) ) RETURN_ERROR(self->logging);
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	if ( pthread_cond_init(&(self->cond), NULL) ) RETURN_ERROR(self->logging);
	if ( pthread_create(&(self->thread), NULL, runner_thread_main, self) ) RETURN_ERROR(self->logging);
	LOG_STATUS(self->logging, "initialized Async HDF5 instance");
	*pself = self;
	return 0;
}


int ah5_init_file( ah5_t* pself, const char *dirname, size_t max_size )
{
	if ( ah5_init_base(pself) ) return 1;
	ah5_t self = *pself;
	buf_init_file(&self->data_buf, dirname, max_size);
	if ( pthread_mutex_unlock(&self->mutex) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_init_mem( ah5_t* pself, void *buffer, size_t max_size )
{
	if ( ah5_init_base(pself) ) return 1;
	ah5_t self = *pself;
	buf_init_mem(&self->data_buf, buffer, max_size);
	if ( pthread_mutex_unlock(&self->mutex) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_init( ah5_t* pself )
{
	return ah5_init_mem(pself, NULL, 0);
}


int ah5_finalize( ah5_t self )
{
	// wait for the writer thread to finish its work; this locks the mutex
	if ( runner_thread_wait(self) ) RETURN_ERROR(self->logging);
	
	// tell the writer thread to terminate
	self->thread_stop = 1;
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR(self->logging);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	
	LOG_STATUS(self->logging, "finalizing Async HDF5 instance");
	
	// cleanup resources
	pthread_join( self->thread, NULL);
	pthread_cond_destroy(&self->cond);
	pthread_mutex_destroy(&self->mutex);
	buf_free(&self->data_buf);
	assert(cl_empty(&self->commands));
	
	log_destroy(&self->logging);
	free(self);
	
	return 0;
}


int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	log_set_lvl(&self->logging, log_lvl);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_set_logfile( ah5_t self, char* log_file_name )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	log_set_filename(&self->logging, log_file_name);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_set_logfile_f( ah5_t self, FILE* log_file )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	log_set_file(&self->logging, log_file, 0);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_set_logfile_desc( ah5_t self, int log_file_desc )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	log_set_filedesc(&self->logging, log_file_desc, 0);
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_set_paracopy( ah5_t self, int parallel_copy )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	self->parallel_copy = parallel_copy;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}


int ah5_start(ah5_t self, char* file_name) { return ah5_open(self, file_name); }

int ah5_open( ah5_t self, char* file_name )
{
	// wait for the writer thread to finish its work; this locks the mutex
	if ( runner_thread_wait(self) ) RETURN_ERROR(self->logging);
	
	LOG_DEBUG(self->logging, "async HDF5 opening file");
	
	// open the file
	self->file = H5Fcreate( file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
	
	// don't release the lock, close will do it
	return 0;
}


int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
				hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds )
{
	// don't take the lock, open should have done it
	
	// create a new write command in the list
	LOG_DEBUG(self->logging, "adding a write command to the list");	
	data_write_t *command = cl_insert_tail(&self->commands);
	
	// fill the command
	command->buf = data;
	command->rank = rank;
	for ( int ii = 0; ii<rank; ++ii ) {
		command->dims[ii] = dims[ii];
		command->lbounds[ii] = lbounds[ii];
		command->ubounds[ii] = ubounds[ii];
	}
	size_t name_len = strlen(name)+1;
	command->name = malloc(name_len);
	memcpy(command->name, name, name_len);
	command->type = type;
	
	LOG_DEBUG(self->logging, "added writing command for data %s of rank %u", name, (unsigned)rank);
	return 0;
}


int ah5_finish(ah5_t self) { return ah5_close(self); }

int ah5_close( ah5_t self )
{
	LOG_DEBUG(self->logging, "sealing write command list");
	int64_t start_time = clockget();
	
	ptrdiff_t buf_size = 0;
	// compute the total size of the data
	for ( data_write_t *cmd = cl_head(&self->commands); cmd; cmd = cmd->next ) {
		ptrdiff_t data_size = H5Tget_size(cmd->type);
		for ( unsigned dim = 0; dim < cmd->rank; ++dim ) {
			// ubounds is just after the data, so the difference with lbound is the size
			data_size *= cmd->ubounds[dim]-cmd->lbounds[dim];
		}
		buf_size += data_size;
	};
	
	// try to grow the buffer to contain the whole data
	buf_grow(&self->data_buf, buf_size);
	
	// copy the data into the buffer
	void *buf = self->data_buf.content;
	data_write_t *wrt;
	for ( wrt = cl_tail(&self->commands); wrt; wrt = wrt->prev ) {
		int64_t cmd_start_time = clockget();
		
		// stop the copy if there isn't enough space in the buffer
		ptrdiff_t data_size = H5Tget_size(wrt->type);
		ptrdiff_t buf_sz = (char*)(self->data_buf.content)-(char*)buf + self->data_buf.max_size;
		if ( buf_sz < data_size ) break;
		
		// do the actual copy for this command
		slicecpy(buf, wrt->buf, wrt->type, wrt->rank, wrt->dims, wrt->lbounds, 
				wrt->ubounds, self->parallel_copy);
		wrt->buf = buf;
		buf = ((char*)buf) + data_size;
		
		// since only the data has been copied, update dims, lbounds & ubounds
		for ( unsigned dim = 0; dim<wrt->rank; ++dim ) {
			data_size *= wrt->ubounds[dim]-wrt->lbounds[dim];
			wrt->dims[dim] = wrt->ubounds[dim] - wrt->lbounds[dim];
			wrt->ubounds[dim] -= wrt->lbounds[dim];
			wrt->lbounds[dim] = 0;
		}
		
		LOG_DEBUG(self->logging, "copy duration for %s: %" PRId64 "us", wrt->name, clockget()-cmd_start_time);
	};
	
	// handle those write commands we didn't manage to store because of missing space
	if (wrt) while ( cl_head(&self->commands) !=  wrt ) {
		data_write_t *cmd = cl_remove_head(&self->commands);
		int64_t cmd_start_time = clockget();
		
		dw_run(cmd, self->file, self->logging);
		dw_free(cmd);
		
		LOG_DEBUG(self->logging, "Sync write duration for %s: %" PRId64 "us", wrt->name, clockget()-cmd_start_time);
	}
	
	/* wake up the writer thread */
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR(self->logging);
	LOG_STATUS(self->logging, "command list finished, triggering worker thread");
	LOG_DEBUG(self->logging, "command list sealing duration: %" PRId64 "us", clockget()-start_time);
  
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	return 0;
}

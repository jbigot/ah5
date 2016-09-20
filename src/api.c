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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#include "ah5.h"
#include "ah5_impl.h"
#include "logging.h"
#include "memhandling.h"
#include "command_list.h"
#include "runner_thread.h"


int ah5_init_base( ah5_t* pself )
{
	ah5_t self = malloc(sizeof(struct ah5_s));
	if ( H5open() ) RETURN_ERROR;
	self->data_buf.content = NULL;
	self->data_buf.max_size = 0;
	self->data_buf.strategy = BUF_BASE;
	self->data_buf.used_size = 0;
	self->commands = NULL;
	self->thread_stop = 0;
	self->file = 0;
	self->parallel_copy = 1;
	self->logging.file = stderr;
	self->logging.closing_strategy = FILE_KEEP_OPEN;
	self->logging.verbosity = AH5_VERB_ERROR;
	if ( pthread_mutex_init(&(self->mutex), NULL) ) RETURN_ERROR;
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	if ( pthread_cond_init(&(self->cond), NULL) ) RETURN_ERROR;
	if ( pthread_create(&(self->thread), NULL, runner_thread_main, self) ) RETURN_ERROR;
	LOG_STATUS("initialized Async HDF5 instance");
	*pself = self;
	return 0;
}


int ah5_init_file( ah5_t* pself, const char *dirname, size_t max_size )
{
	if ( ah5_init_base(pself) ) return 1;
	ah5_t self = *pself;
	self->data_buf.strategy |= BUF_MMAPED;
	if ( max_size ) {
		size_t pagesize = sysconf(_SC_PAGE_SIZE);
		self->data_buf.max_size = ( max_size / pagesize ) * pagesize;
		int fd = open(dirname, O_CREAT|O_NOATIME|O_TMPFILE|O_RDWR|O_TRUNC);
		self->data_buf.content = mmap(NULL, self->data_buf.max_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_HUGETLB, fd, 0);
	} else {
		self->data_buf.strategy |= BUF_DYNAMIC;
		size_t pagesize = sysconf(_SC_PAGE_SIZE);
		self->data_buf.max_size = pagesize;
		int fd = open(dirname, O_CREAT|O_NOATIME|O_TMPFILE|O_RDWR|O_TRUNC);
		self->data_buf.content = mmap(NULL, self->data_buf.max_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_HUGETLB, fd, 0);
	}
	if ( pthread_mutex_unlock(&self->mutex) ) RETURN_ERROR;
	return 0;
}


int ah5_init_mem( ah5_t* pself, void *buffer, size_t max_size )
{
	if ( ah5_init_base(pself) ) return 1;
	ah5_t self = *pself;
	self->data_buf.strategy |= BUF_MALLOCED;
	if ( max_size ) {
		self->data_buf.max_size = max_size;
		self->data_buf.content = buffer;
	} else {
		self->data_buf.strategy |= BUF_DYNAMIC;
	}
	if ( pthread_mutex_unlock(&self->mutex) ) RETURN_ERROR;
	return 0;
}


int ah5_init( ah5_t* pself )
{
	return ah5_init_mem(pself, NULL, 0);
}


int ah5_finalize( ah5_t self )
{
	/* wait for the writer thread to finish its work */
	if ( runner_thread_wait(self) ) RETURN_ERROR;
	
	/* tell the writer thread to terminate */
	self->thread_stop = 1;
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	
	// cleanup resources
	pthread_join( self->thread, NULL);
	if ( self->logging.closing_strategy == FILE_CLOSE ) {
		fclose(self->logging.file);
	}
	pthread_mutex_destroy(&self->mutex);
	pthread_cond_destroy(&self->cond);
	freebuffer(&self->data_buf);
	
	/* free all memory */
	free(self->data_buf.content);
	free(self);
	
	LOG_STATUS("finalized Async HDF5 instance");
	return 0;
}


int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->logging.verbosity = log_lvl;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_set_logfile( ah5_t self, char* log_file_name )
{
	int log_fd;
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	if ( self->logging.closing_strategy == FILE_CLOSE ) {
		fclose(self->logging.file);
	}
	log_fd = open(log_file_name, O_WRONLY|O_APPEND|O_CREAT|O_SYNC|O_DSYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if ( log_fd == -1 ) RETURN_ERROR;
	self->logging.file = fdopen(log_fd, "a");
	if ( !self->logging.file ) RETURN_ERROR;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_set_logfile_f( ah5_t self, FILE* log_file_name );


int ah5_set_logfile_desc( ah5_t self, int log_file_desc );


int ah5_set_paracopy( ah5_t self, int parallel_copy )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	self->parallel_copy = parallel_copy;
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ah5_open( ah5_t self, char* file_name )
{
	/* wait for the writer thread to finish its work */
	if ( runner_thread_wait(self) ) RETURN_ERROR;
	LOG_DEBUG("async HDF5 opening file");
	/* store the file name */
	self->file = H5Fcreate( file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
	return 0;
}


int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
				hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds )
{
	LOG_DEBUG("adding a write command to the list");
	
	/* increase the array containing all write commands */
	write_list_t *command = malloc(sizeof(write_list_t));
	if ( self->commands ) {
		command->previous = self->commands->previous;
		self->commands->previous->next = command;
		command->next = self->commands;
		self->commands->previous = command;
	} else {
		command->next = command;
		command->previous = command;
		self->commands = command;
	}
	
	command->content.buf = data;
	command->content.rank = rank;
	for ( int ii = 0; ii<rank; ++ii ) {
		command->content.dims[ii] = dims[ii];
		command->content.lbounds[ii] = lbounds[ii];
		command->content.ubounds[ii] = ubounds[ii];
	}
	command->content.name = malloc(strlen(name)+1);
	strcpy(command->content.name, name);
	command->content.type = type;
	
	LOG_DEBUG("added writing command for data %s of rank %u", name, (unsigned)rank);
	return 0;
}


int ah5_close( ah5_t self )
{
	LOG_DEBUG("sealing write command list");
	int64_t start_time = clockget();
	
	size_t buf_size = 0;
	/* compute the total size of the data */
	write_list_t *cmd = self->commands;
	if ( cmd ) do {
		size_t data_size = H5Tget_size(cmd->content.type);
		for ( unsigned dim = 0; dim < cmd->content.rank; ++dim ) {
			/* ubounds is just after the data, so the difference with lbound is the size */
			data_size *= cmd->content.ubounds[dim]-cmd->content.lbounds[dim];
		}
		buf_size += data_size;
		cmd = cmd->next;
	} while ( cmd->next != self->commands );
	
	/* allocate a buffer able to contain it all */
	self->data_buf.content = realloc(self->data_buf.content, buf_size);
	
	/* copy the data into the bufer */
	void *buf = self->data_buf.content;
	cmd = self->commands;
	if ( cmd ) do {
		int64_t start_time = clockget();
		slicecpy(buf, cmd->content.buf, cmd->content.type, cmd->content.rank, cmd->content.dims,
						cmd->content.lbounds, cmd->content.ubounds, self->parallel_copy);
		size_t data_size = H5Tget_size(cmd->content.type);
		/* since only the data has been copied, update dims, lbounds & ubounds */
		for ( unsigned dim = 0; dim<cmd->content.rank; ++dim ) {
			data_size *= cmd->content.ubounds[dim]-cmd->content.lbounds[dim];
			cmd->content.dims[dim] = cmd->content.ubounds[dim] - cmd->content.lbounds[dim];
			cmd->content.ubounds[dim] -= cmd->content.lbounds[dim];
			cmd->content.lbounds[dim] = 0;
		}
		cmd->content.buf = buf;
		buf = ((char*)buf) + data_size;
		LOG_DEBUG("copy duration: %" PRId64 "us", clockget()-start_time);
	} while ( cmd->next != self->commands );
	
	/* wake up the writer thread */
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	LOG_STATUS("command list finished, triggering worker thread");
	LOG_DEBUG("command list sealing duration: %" PRId64 "us", clockget()-start_time);
  
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}

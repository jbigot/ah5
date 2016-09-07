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

#include <string.h>
#include <stdlib.h>

#include "runner_thread.h"

#include "status.h"
#include "logging.h"
#include "memhandling.h"

#include "command_queue_dynamic.h"

int cqd_push_open( command_queue_dynamic_t *self, char* file_name )
{
	/* wait for the writer thread to finish its work */
	if ( runner_thread_wait(self) ) RETURN_ERROR;
	
	LOG_DEBUG("starting new command list");
	
	/* reserve memory for an open command */
	size_t alloc_sz = sizeof(command_kind_t) + sizeof(command_open_t) + strlen(file_name);
	self->content = realloc(self->content, self->content_sz+alloc_sz);
	command_kind_t *kind = self->content+self->content_sz;
	command_open_t *command = (void*)(kind+1);
	self->content_sz += alloc_sz;
	
	/* fill the command with the required info */
	strcpy(command->file_name, file_name);
	
	/* initialize the buffer size to 0 */
	self->buffer_sz = 0;
	
	return 0;
}


int cqd_push_write( command_queue_dynamic_t *self, void* data, char* name, hid_t type, int rank,
		hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds )
{
	/* no need to lock the thread here, open did it for us */
	
	LOG_DEBUG("adding a write command to the list");
	
	/* reserve memory for a write command */
	size_t alloc_sz = sizeof(command_kind_t) + sizeof(command_write_t) + strlen(name);
	self->content = realloc(self->content, self->content_sz+alloc_sz);
	command_kind_t *kind = self->content+self->content_sz;
	command_write_t *command = (void*)(kind+1);
	self->content_sz += alloc_sz;

	/* fill the command with the required info */
	command->buf = data;
	command->rank = rank;
	for ( int ii = 0; ii<rank; ++ii ) {
		command->dims[ii] = dims[ii];
		command->lbounds[ii] = lbounds[ii];
		command->ubounds[ii] = ubounds[ii];
	}
	command->type = type;
	strcpy(command->name, name);
	
	/* grow the required buffer size by this write size */
	size_t data_size = H5Tget_size(type);
	for ( unsigned dim = 0; dim < rank; ++dim ) {
		/* ubounds is just after the data, so the difference with lbound is the size */
		data_size *= ubounds[dim]-lbounds[dim];
	}
	self->buffer_sz += data_size;
	
	return 0;
}


int cqd_push_close( command_queue_dynamic_t *self )
{
	/* no need to lock the thread here, open did it for us */
	
	LOG_DEBUG("sealing write command list");
	int64_t start_time = clockget();
	
	/* reserve memory for the command & the whole data */
	size_t alloc_sz = sizeof(command_kind_t) + self->buffer_sz;
	self->content = realloc(self->content, self->content_sz+alloc_sz);
	command_kind_t *kind = self->content+self->content_sz;
	void *data_buf = (void*)(kind+1);
	self->content_sz += alloc_sz;

	/* copy the data into the bufer */
	void* buf = data_buf;
	for ( size_t ii = 0; ii<self->data_size; ++ii ) {
		slicecpy(buf, self->data[ii].buf, self->data[ii].type, self->data[ii].rank, self->data[ii].dims,
						self->data[ii].lbounds, self->data[ii].ubounds, self->parallel_copy);
		size_t data_size = H5Tget_size(self->data[ii].type);
		/* since only the data has been copied, update dims, lbounds & ubounds */
		for ( unsigned dim = 0; dim<self->data[ii].rank; ++dim ) {
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

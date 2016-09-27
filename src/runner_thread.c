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

#include <pthread.h>
#include <assert.h>
#include <hdf5_hl.h>

#include "ah5_impl.h"
#include "logging.h"
#include "command_list.h"

#include "runner_thread.h"


int runner_thread_wait( ah5_t self )
{
	/* wait for the writer thread */
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR(self->logging);
	/* wait for a potential previous write command to be executed */
	while ( self->commands.head ) {
		LOG_STATUS(self->logging, "waiting for writer thread");
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR(self->logging);
	}
	return 0;
}


void* runner_thread_main( void* self_void )
{
	ah5_t self = self_void;
	if ( pthread_mutex_lock(&(self->mutex)) ) SIGNAL_ERROR(self->logging);
	LOG_STATUS(self->logging, "async HDF5 thread started");
	for (;;) {
		
		/* when the command is to wait ... wait for it to change */
		while ( ! self->commands.head && ! self->thread_stop ) {
			if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) SIGNAL_ERROR(self->logging);
		}
		
		/* if the command is to stop ... stop */
		if ( self->thread_stop ) {
			LOG_DEBUG(self->logging, "async HDF5 thread executing terminate command");
			pthread_mutex_unlock(&(self->mutex));
			return NULL;
		}
		
		/* otherwise, the command is to execute the write list */
		LOG_DEBUG(self->logging, "async HDF5 thread executing write commands");
		int64_t start_time = clockget();
		
		while ( self->commands.head ) {
			data_write_t *cmd = self->commands.head;
			LOG_DEBUG(self->logging, "async HDF5 writing data %s of rank %u", cmd->name, (unsigned)cmd->rank);
			dw_run(cmd, self->file, self->logging);
			cl_remove_head(&self->commands);
		}

		LOG_DEBUG(self->logging, "async HDF5 closing file");
		if ( H5Fclose(self->file) ) SIGNAL_ERROR(self->logging);
		
		LOG_DEBUG(self->logging, "async HDF5 write duration: %" PRId64 "us", clockget()-start_time);
		/* once the write list has been fully executed, wake up the main thread
		 * potentially waiting for us */
		if ( pthread_cond_signal(&(self->cond)) ) SIGNAL_ERROR(self->logging);
	}
}



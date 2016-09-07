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

#include "logging.h"

#include "command_queue_static.h"
#include "command_queue_dynamic.h"

#include "runner_thread.h"

/** Waits for the worker thread to finish its previous run
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 */
inline static int runner_thread_wait( command_queue_t *self )
{
	/* wait for the writer thread */
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	/* wait for a potential previous write command to be executed */
	while ( self->thread_cmd ==  CMD_WORK ) {
		LOG_STATUS("waiting for writer thread");
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
	}
	return 0;
}

/** The function executed by the writer thread
 * @param self_void a pointer to the instance state as a void*
 * @returns NULL
 */
static void* runner_thread_main( void* self_void )
{
	command_queue_t *self = self_void;
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	LOG_STATUS("async HDF5 thread started");
	for (;;) {
		/* when the command is to wait ... wait for it to change */
		while ( self->thread_cmd ==  CMD_WAIT ) {
			if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
		}
		LOG_DEBUG("async HDF5 thread executing command");
		/* if the command is to stop ... stop */
		if ( self->thread_cmd == CMD_TERMINATE ) {
			LOG_DEBUG("async HDF5 thread executing terminate command");
			pthread_mutex_unlock(&(self->mutex));
			return NULL;
		}
		/* otherwise, the command is to execute the write list */
		assert( self->thread_cmd == CMD_WORK );
		LOG_DEBUG("async HDF5 thread executing work command");
		
		int64_t start_time = clockget();
		
		int empty; cq_is_empty(self, empty);
		while ( !empty ) {
			command_kind_t command_kind;
			void *command;
			cq_back(self, &command_kind, &command);
			
			switch ( command_kind ) {
			case CK_OPEN: {
				execute_open(self, (command_open_t*)command);
			} break;
			case CK_WRITE: {
				execute_write(self, (command_write_t*)command);
			} break;
			case CK_CLOSE: {
				execute_close(self);
			} break;
			}
			
			cq_pop(self);
		}
		
		LOG_DEBUG("async HDF5 command execution duration: %" PRId64 "us", clockget()-start_time);

		/* once the write list has been fully executed, tell oneself to wait for the next one */
		self->thread_cmd = CMD_WAIT;
		/* and wake up the main thread potentially waiting for us */
		if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	}
}

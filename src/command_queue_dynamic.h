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

#ifndef AH5_COMMAND_QUEUE_DYNAMIC_H__
#define AH5_COMMAND_QUEUE_DYNAMIC_H__

#include "command_queue.h"

typedef struct command_queue_dynamic_s
{
	command_queue_t queue;

	size_t content_sz;

	void *content;
	
	size_t buffer_sz;

} command_queue_dynamic_t;


int cqd_push_open( command_queue_dynamic_t *self, char* file_name );


int cqd_push_write( command_queue_dynamic_t *self, void* data, char* name,
		hid_t type, int rank, hsize_t* dims, hsize_t* lbounds,
		hsize_t* ubounds );


int cqd_push_close( command_queue_dynamic_t *self );


int cqd_back( command_queue_dynamic_t *self, command_kind *kind,
		void** command );


int cqd_pop( command_queue_dynamic_t *self );


int cqd_is_empty( command_queue_dynamic_t *self, int *empty );

#endif /* AH5_COMMAND_QUEUE_DYNAMIC_H__ */

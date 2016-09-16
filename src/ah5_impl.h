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

#ifndef AH5_IMPL_H__
#define AH5_IMPL_H__

#include <stdio.h>
#include <pthread.h>

#include "ah5.h"


typedef struct write_list_s write_list_t;


/** The various ways the memory buffer can be handled
 */
typedef enum buffer_strategy
{
	BUF_BASE=0, /**< the buffer is allocated to a fixed size */
	BUF_DYNAMIC=1, /**< the buffer can be grown fit the data */
	BUF_MMAPED=2, /**< the buffer has been mmap'ed by Ah5 and should be freed */
	BUF_MALLOCED=4 /**< the buffer has been malloc'ed by Ah5 and should be freed */
} buffer_strategy_t;


/** Status of the asynchronous HDF5 instance
 */
struct ah5_s {

	/** a mutex controling access to this instance
	 */
	pthread_mutex_t mutex;

	/** a condition variable used to signal that the instance content has changed
	 */
	pthread_cond_t cond;

	struct {
		/** the way the memory buffer should be handled. This is a logical OR
		 *  of buffer_strategy_t
		 */
		int strategy;

		/** The buffer where the data is copied
		 */
		void *content;

		/** the used size in the memory buffer in bytes
		 */
		size_t used_size;

		/** the maximum size of the memory buffer in bytes
		 */
		size_t max_size;
		
	} data_buf;

	/** The actual command list or NULL if empty
	 */
	write_list_t* commands;

	/** the thread executing the command list
	 */
	pthread_t thread;

	/** Whether to stop the thread executing the command list
	 */
	int thread_stop;

	/** the opened file commands relate to
	 */
	hid_t file;
	
	/** whether to use all core for copies
	 */
	int parallel_copy;

	struct {
		/** the file where to log
		 */
		FILE* file;
		
		enum {
			FILE_CLOSE,
			FILE_KEEP_OPEN
		} closing_strategy;

		/** the verbosity level
		 */
		ah5_verbosity_t verbosity;
		
	} logging;

};

#endif /* AH5_IMPL_H__ */

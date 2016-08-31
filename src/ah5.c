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




/** The various ways the memory buffer can be handled
 */
typedef enum buffer_strategy {
	BUF_DYNAMIC, /**< the buffer is dynamically allocated to fit the data */
	BUF_FIXED, /**< the buffer is allocated to a fixed size */
	BUF_MMAPED /**< the buffer is mmaped in a file */
} buffer_strategy_t;


/** Status of the asynchronous HDF5 instance
 */
struct ah5 {

	/** The buffer where the data is copied */
	void* data_buffer;

	/** the size of the memory buffer in bytes, the buffer might not be fully used */
	size_t data_buffer_size;

	/** the way the memory buffer should be handled */
	buffer_strategy_t data_buffer_strategy;

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


#ifndef NO_MALLOC_USABLE
#define MALL_SZ(BUF, ALLSZ) \
	malloc_usable_size(BUF);
#else
	ALLSZ
#endif

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


int ah5_init( ah5_t* pself )
{
	ah5_t self = malloc(sizeof(struct ah5));
	if ( H5open() ) RETURN_ERROR;
	self->file_name = NULL;
	self->log_file = NULL;
	self->log_verbosity = VERBOSITY_WARNING;
	self->thread_cmd = CMD_WAIT;
	self->data_buffer = NULL;
	self->data_buffer_size = 0;
	self->data_buffer_strategy = BUF_DYNAMIC;
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
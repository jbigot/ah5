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

#include <assert.h>
#include <malloc.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "ah5_impl.h"
#include "logging.h"

#include "memhandling.h"


#ifndef NO_MALLOC_USABLE
#define MALL_SZ(BUF, ALLSZ) \
        malloc_usable_size(BUF);
#else
        ALLSZ
#endif


static int freebuffer( ah5_t self )
{
	free(self->data_buf.content);
	self->data_buf.max_size = 0;
	return 0;
}


/** Grows the memory buffer similarly to realloc, but disards the currently
 * held data.
 * 
 * @param self a pointer to the instance state
 * @param size the requested size for the buffer
 * @returns 0 on success, non-null on error
 */
int growbuffer(ah5_t self, size_t size)
{
	if ( size > self->data_buf.max_size ) {
		LOG_DEBUG("Growing buffer size");
		freebuffer(self);
		self->data_buf.content = malloc(size);
		self->data_buf.max_size = MALL_SZ(self->data_buf.content, size);
	}
	return 0;
}


void* memcpy_omp( void* dest, void* src, size_t size )
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


void* slicecpy( void* dest, void* src, hid_t type, unsigned rank, hsize_t* sizes,
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

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

#include <stdint.h>
#include <pthread.h>

#include "status.h"

#include "ring_queue.h"


/** one element in the ring buffer, content + boilerplate
 */
typedef struct ring_queue_elem_s
{
	/** adress of the next element (or NULL if this is the front) */
	struct ring_queue_elem_s *next;
	
	/** size of the element content */
	size_t size;
	
	/** the actual content, don't pay attention to the type, only here to ensure double alignment */
	double content[1];
	
} ring_queue_elem_t;


struct ring_queue_s
{
	/** a mutex controling access to this instance */
	pthread_mutex_t mutex;

	/** a condition variable used to signal that the queue content has changed */
	pthread_cond_t cond;
	
	/** Total size: [circ] + [exp] + [free] as specified by the user
	 */
	size_t total_size;

	/** size of the actual circular buffer, i.e. [circ]
	 */
	size_t circ_size;

	/** size of the actual used size, i.e. [circ] + [exp]
	 */
	size_t used_size;
	
	/** Back of the queue (i.e. first inserted element)
	 */
	ring_queue_elem_t *back;

	/** Front of the queue (i.e. last inserted element)
	 */
	ring_queue_elem_t *front;

	/** The actual buffer that goes [circ][exp][free] where [circ] is the memory
	 * used for the actual circular buffer, [exp] is the "expansion area" where
	 * data is stored when the circular buffer size proves too small and [free]
	 * is the free space not yet used.
	 * 
	 * Don't pay attention to the type, only here to ensure alignment
	 */
	double buffer[1];
	
};


int ring_queue_init( ring_queue_t* self, size_t buffer_size )
{
	if ( buffer_size <= sizeof(ring_queue_t) ) return 1;
	if ( pthread_mutex_init(&(self->mutex), NULL) ) RETURN_ERROR;
	if ( pthread_cond_init(&(self->cond), NULL) ) RETURN_ERROR;
	/* try to align the buffer */
	self->total_size = buffer_size - ((char*)self-(char*)(self->buffer));
	return 0;
}


int ring_queue_destroy( ring_queue_t *self )
{
	if ( self->back ) return 1;

	if ( pthread_cond_destroy(&(self->cond)) ) RETURN_ERROR;
	if ( pthread_mutex_destroy(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ring_queue_push( ring_queue_t *self, size_t size, void **data )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	
	/* size of the data to insert in the buffer */
	size_t insert_size = sizeof(ring_queue_elem_t)+size-1;
	
	/* cannot insert objects bigger than the max size */
	if ( insert_size > self->total_size ) {
		if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
		return 1;
	}
	
	/* the location of the inserted item */
	ring_queue_elem_t *elem = NULL;
	while ( !elem ) {
		/* case where the buffer is empty */
		if ( !self->front ) {
			elem = (ring_queue_elem_t*) self->buffer;
			if ( self->circ_size < insert_size ) self->circ_size = insert_size;
			if ( self->used_size < self->circ_size ) {
				self->used_size = self->circ_size;
			}
			/* since we're the only element, we're to become the back in
			   addition of the front */
			self->back = elem; 
		/* case where the buffer is expanding */
		} else if ( self->circ_size != self->used_size ) {
			/* there is enough space to insert our data */
			if ( self->used_size+insert_size <= self->total_size ) {
				elem = (ring_queue_elem_t*) 
						(self->front->content + self->front->size);
				self->used_size += insert_size;
			}
		/* case where everything is in the circular buffer and the remaining
		   space is inbetween front and back */
		} else if ( self->front < self->back ) {
			/* case where there is enough space to insert the data here */
			if ( (char*) self->front->content+self->front->size-1+insert_size 
					<= (char*) self->back ) {
				elem = (ring_queue_elem_t*)
						(self->front->content + self->front->size);
			/* case where there is enough space to insert our data  in [ext]:
			   start expanding */
			} else if ( self->used_size+insert_size <= self->total_size ) {
				elem = (ring_queue_elem_t*) (self->buffer + self->used_size);
				self->used_size += insert_size;
			}
		/* case where everything is in the circular buffer and the remaining
		   space contains the buffer wrap */
		} else {
			/* case where there is enough space to insert the data at the end */
			if ( (char*) self->front->content+self->front->size-1+insert_size
					<= (char*)self->buffer+self->circ_size ) {
				elem = (ring_queue_elem_t*)
						(self->front->content + self->front->size);
			/* case where there is enough space to insert the data at the start */
			} else if ( (char*)self->buffer+insert_size <= (char*) self->back ) {
				elem = (ring_queue_elem_t*) self->buffer;
			/* case where there is enough space to insert our data  in [ext]:
			   start expanding */
			} else if ( self->used_size+insert_size <= self->total_size ) {
				elem = (ring_queue_elem_t*) (self->buffer + self->used_size);
				self->used_size += insert_size;
			}
		}
		/* if there wasn't enough space to insert our data, wait for something
		   to change */
		if ( !elem ) { 
			if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
		}
	}
	/* actually do the insertion */
	elem->next = NULL;
	elem->size = size;
	*data = elem->content;
	self->front->next = elem;
	self->front = elem;
	
	// notify that the content of the queue changed
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ring_queue_back( ring_queue_t *self, size_t *size, void **data )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	
	while ( !self->back ) {
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
	}
	
	*size = self->back->size;
	*data = self->back->content;
	
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}


int ring_queue_pop( ring_queue_t *self )
{
	if ( pthread_mutex_lock(&(self->mutex)) ) RETURN_ERROR;
	
	while ( !self->back ) {
		if ( pthread_cond_wait(&(self->cond), &(self->mutex)) ) RETURN_ERROR;
	}
	
	self->back = self->back->next;
	
	/* case where we emptied the queue */
	if ( !self->back ) {
		self->front = NULL;
	/* case where we consumed the last element from the circular part of the
	   buffer paradoxically making it completely circular again */
	} else if ( (char*) self->back >= (char*)self->buffer + self->circ_size ) {
		self->circ_size = self->used_size;
	}
	
	// notify that the content of the queue changed
	if ( pthread_cond_signal(&(self->cond)) ) RETURN_ERROR;
	
	if ( pthread_mutex_unlock(&(self->mutex)) ) RETURN_ERROR;
	return 0;
}

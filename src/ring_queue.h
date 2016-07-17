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


/** A producer-consumer circular buffer implementation of a queue
 * 
 * It uses an algorithm inspired by http://blog.labix.org/2010/12/23/efficient-algorithm-for-expanding-circular-buffers
 */
typedef struct ring_queue_s ring_queue_t;


/** Initializes a new ring queue
 * 
 * @param[in] self a buffer to contain the queue.
 * @param[in] buffer_size the size of the buffer in bytes.
 * @returns 0 on success otherwise on error
 */
int ring_queue_init( ring_queue_t *self, size_t buffer_size );


/** Destroys an empty queue.
 * 
 * @param self the queue to destroy.
 * @returns 0 on success otherwise on error
 */
int ring_queue_destroy( ring_queue_t *self );



/** Reserves some space at the front of the queue to store data.
 * 
 * Blocks if there is not enough space to store the data until enough has been
 * consumed.
 * @param[in] self the queue where to push the data.
 * @param[in] size the size of the data to store
 * @param[out] data a pointer to the allocated space that can be written to
 * @returns 0 on success otherwise on error
 */
int ring_queue_push( ring_queue_t *self, size_t size, void **data );



/** Retrieves data from the back of the queue to access it without removing it.
 * 
 * Blocks if there is no data in the queue until some has been produced.
 * @param[in] self the queue where to look for data.
 * @param[out] size the size of the retrieved data.
 * @param[out] data a pointer to the retrieved data that can be read from
 * @returns 0 on success otherwise on error
 */
int ring_queue_back( ring_queue_t *self, size_t *size, void **data );



/** Pops the data at the back of the queue.
 * 
 * Blocks if there is no data in the queue until some has been produced.
 * @param[in] self the queue from which to pop data.
 * @returns 0 on success otherwise on error
 */
int ring_queue_pop( ring_queue_t *self );

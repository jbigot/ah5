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

/** Maximum number of thread supported in the multiple thread copy
 *
 * \todo automatically detect the actual max number of thread or dynamically
 * detect it so as not to crash on MIC for example
 */
#define MAX_NB_THREAD 32


/** This function has the exact same prototype as memcpy and does the exact same
 * thing, except it does it in parallel
 * @param dest the destination of the copy
 * @param src the source of the copy
 * @param size the number of bytes to copy
 * @return a pointer to dest
 */
void* memcpy_omp( void* dest, void* src, size_t size );


/** Copies a slice of a nD array (in fact a block) from src to dest.
 * @param dest the destination of the block (contiguous)
 * @param src the source array where the block is
 * @param type the type of elements in the array
 * @param rank the number of dimensions of the array
 * @param sizes the sizes of the array in each dimension
 * @param lbounds the lower bounds of the block in each dimension
 * @param ubounds the upper bounds of the block in each dimension
 * @param parallelism whether to do the copy in parallel
 * @return dest
 */
void* slicecpy( void* dest, void* src, hid_t type, unsigned rank, hsize_t* sizes,
		hsize_t* lbounds, hsize_t* ubounds, int parallelism );


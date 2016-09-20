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

#ifndef AH5_MEMHANDLING_FWD_H__
#define AH5_MEMHANDLING_FWD_H__

/** The various ways the memory buffer can be handled
 */
typedef enum
{
	/// the buffer is allocated to a fixed size
	BUF_BASE=0,
	
	/// the buffer can be grown fit the data
	BUF_DYNAMIC=1,
	
	/// the buffer has been mmap'ed by Ah5 and should be free'd
	BUF_MMAPED=2,
	
	/// the buffer has been malloc'ed by Ah5 and should be munmap'ed
	BUF_MALLOCED=4
	
} buffer_strategy_t;


typedef struct data_buf_s data_buf_t;

#endif //AH5_MEMHANDLING_FWD_H__

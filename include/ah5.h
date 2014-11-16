/*******************************************************************************
 * Copyright (c) 2013-2014, Julien Bigot - CEA (julien.bigot@cea.fr)
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

#ifndef ASYNC_HDF5_H__
#define ASYNC_HDF5_H__

#include <hdf5.h>

typedef enum {
	VERBOSITY_ERROR=0,
	VERBOSITY_WARNING,
	VERBOSITY_STATUS,
	VERBOSITY_DEBUG
} ah5_verbosity_t;

typedef struct ah5* ah5_t;

/** Initializes the asynchronous HDF5 writer instance
 * @param self a pointer to the instance state to be allocated
 * @returns 0 on success, non-null on error
 * @post the writer thread is ready
 */
int ah5_init( ah5_t* pself );

/** Sets the minimum verbosity level to actually log
 * @param self a pointer to the instance state
 * @param log_lvl the minimum verbosity level to actually log
 * @returns 0 on success, non-null on error
 * @invariant the writer thread is ready
 */
int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl );

/** Sets the the file where to log (stderr by default)
 * @param self a pointer to the instance state
 * @param log_file the file where to log
 * @returns 0 on success, non-null on error
 * @invariant the writer thread is ready
 */
int ah5_set_logfile( ah5_t self, char* log_file );

/** Sets whether to write scalars as a 1D size 1 array
 * @param self a pointer to the instance state
 * @param scalar_as_array whether to write scalars as a 1D size 1 array
 * @returns 0 on success, non-null on error
 * @invariant the writer thread is ready
 */
int ah5_set_scalarray( ah5_t self, int scalar_as_array );

/** Sets whether to do copies in parallel using all cores
 * @param self a pointer to the instance state
 * @param parallel_copy Whether to do copies in parallel using all cores
 * @returns 0 on success, non-null on error
 * @invariant the writer thread is ready
 */
int ah5_set_paracopy( ah5_t self, int parallel_copy );

/** Finalizes the asynchronous HDF5 writer instance
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 * @pre the writer thread is ready
 */
int ah5_finalize( ah5_t self );

/** Starts a file writing command list, wait for the previous write to finish
 * @param self a pointer to the instance state
 * @param file_name the name of the file where to write the data
 * @returns 0 on success, non-null on error
 * @pre the writer thread is ready
 * @post the writer thread is blocked
 */
int ah5_start( ah5_t self, char* file_name );

/** Issues a HDF5 write command, copy it in the command list
 * @param self a pointer to the instance state
 * @param data a pointer to the data
 * @param name the name of the HDF5 field
 * @param type the HDF5 type of the data
 * @param rank the number of dimensions of the data array (0 for scalar)
 * @param dims the dimensions of the array containing the data
 * @param lbounds the index of the first element to write in each dimension
 * @param ubounds the index of the first element to not write in each dimension
 * @returns 0 on success, non-null on error
 * @pre the writer thread is blocked
 * @post the writer thread is blocked
 */
int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
				hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds );

/** Finishes a file writing command list, copies the data to a local buffer and
 * launches the writer thread
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 * @pre the writer thread is blocked
 * @post the writer thread is ready
 */
int ah5_finish( ah5_t self );

#endif /* ASYNC_HDF5_H__ */

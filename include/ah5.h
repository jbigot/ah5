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

#ifndef AH5_H__
#define AH5_H__

#include <hdf5.h>

/** Verbosity levels supported by AH5
 */
typedef enum {
	VERBOSITY_ERROR=0, ///< only print a message on error
	VERBOSITY_WARNING, ///< print a message on error or for warnings
	VERBOSITY_STATUS, ///< print messages to follow even normal operation
	/// print everything, equal to VERBOSITY_STATUS for non-debug builds of Ah5
	VERBOSITY_DEBUG
} ah5_verbosity_t;

/** The main Ah5 type
 */
typedef struct ah5* ah5_t;

/** Initializes the asynchronous HDF5 writer instance
 * @param self a pointer to the instance state to be allocated
 * @returns 0 on success, non-null on error
 */
int ah5_init( ah5_t* pself );

/** Finalizes the asynchronous HDF5 writer instance
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 */
int ah5_finalize( ah5_t self );

/** Sets the minimum verbosity level to actually log
 * @param self a pointer to the instance state
 * @param log_lvl the minimum verbosity level to actually log
 * @returns 0 on success, non-null on error
 */
int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl );

/** Sets the file where to log
 * @param self a pointer to the instance state
 * @param log_file the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile( ah5_t self, FILE* log_file );

/** Sets the FS name of the file where to log
 * @param self a pointer to the instance state
 * @param log_file_name the FS name of the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile_name( ah5_t self, char* log_file_name );

/** Sets the descriptor of the file where to log
 * @param self a pointer to the instance state
 * @param log_file_desc the descriptor of the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile_desc( ah5_t self, int log_file_desc );

/** Sets whether to use all OpenMP threads
 * @param self a pointer to the instance state
 * @param omp (0 or 1) whether to use all OpenMP threads
 * @returns 0 on success, non-null on error
 */
int ah5_set_omp( ah5_t self, int omp );

/** Sets the buffering strategy to file with a maximum size
 * @param self a pointer to the instance state
 * @param file_name the name of the 
 * @param max_size the maximum size of the buffer or 0 for unlimited
 * @returns 0 on success, non-null on error
 */
int ah5_set_strat_filebuf( ah5_t self, const char *filename, size_t max_size );

/** Sets the buffering strategy to memory with a maximum size
 * @param self a pointer to the instance state
 * @param buffer the adress of the buffer in memory or NULL to auto-allocate
 * @param max_size the maximum size of the buffer or 0 for unlimited
 * @returns 0 on success, non-null on error
 */
int ah5_set_strat_membuf( ah5_t self, void *buffer, size_t max_size );

/** Sets the buffering strategy to dynamic memory with a maximum number of
 * concurrent buffered files.
 * @param self a pointer to the instance state
 * @param nfile the maximum number of concurrent buffered files, 0 for unlimited
 * @returns 0 on success, non-null on error
 */
int ah5_set_strat_dynmem( ah5_t self, int nfile );

/** Issues a file open command to the queue
 * @param self a pointer to the instance state
 * @param file_name the name of the file where to write the data
 * @returns 0 on success, non-null on error
 */
int ah5_open( ah5_t self, char* file_name );

/** Issues a write command to the queue
 * 
 * The provided data should remain valid until the following close command
 * 
 * @param self a pointer to the instance state
 * @param data a pointer to the data; it should remain valid until the
 *             following close command
 * @param name the name of the HDF5 field
 * @param type the HDF5 type of the data
 * @param rank the number of dimensions of the data array (0 for scalar)
 * @param dims the dimensions of the array containing the data
 * @param lbounds the index of the first element to write in each dimension
 * @param ubounds the index of the first element to not write in each dimension
 * @returns 0 on success, non-null on error
 */
int ah5_write( ah5_t self, void* data, char* name, hid_t type, int rank,
				hsize_t* dims, hsize_t* lbounds, hsize_t* ubounds );

/** Issues a file close command to the queue
 * @param self a pointer to the instance state
 * @returns 0 on success, non-null on error
 */
int ah5_close( ah5_t self );

#endif /* AH5_H__ */

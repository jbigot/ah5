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


/** Maximum number of dimensions supported for arrays
 */
#define AH5_MAX_RANK 7


/** Verbosity levels supported by AH5
 */
typedef enum {
	AH5_VERB_ERROR=0, ///< only print a message on error
	AH5_VERB_WARNING, ///< print a message on error or for warnings
	AH5_VERB_STATUS, ///< print messages to follow even normal operation
	/// print everything, equal to VERBOSITY_STATUS for non-debug builds of Ah5
	AH5_VERB_DEBUG
} ah5_verbosity_t;

/** The type of an asynchronous HDF5 (Ah5) writer instance.
 * 
 * Each instance represents a single buffer as well as a given configuration.
 * Calls on this instance will not block except for two cases:
 * * ::ah5_open will block until the previous file write finishes
 * * ::ah5_close will block until enough data has been writen so that the
 *   remaining data fits in the buffer
 */
typedef struct ah5_s* ah5_t;

/** Initializes the asynchronous HDF5 writer instance with a file buffering
 *  strategy
 * 
 * The provided file is used for buffering. If the provided size is 0, the
 * file is grown as required.
 * 
 * @param pself a pointer to the Ah5 instance
 * @param dirname the name of the directory where to do buffering
 * @param max_size the maximum size of the buffer or 0 for unlimited
 * @returns 0 on success, non-null on error
 */
int ah5_init_file( ah5_t* pself, const char *dirname, size_t max_size );

/** Initializes the asynchronous HDF5 writer instance with a memory buffering
 *  strategy.
 * 
 * The provided buffer is used. If NULL is provided, it is allocated by Ah5.
 * If the provided size is 0, the buffer is reallocated using realloc as
 * required.
 * 
 * @param pself a pointer to the Ah5 instance
 * @param file_name the name of the 
 * @param max_size the maximum size of the buffer or 0 for unlimited
 * @returns 0 on success, non-null on error
 */
int ah5_init_mem( ah5_t* pself, void *buffer, size_t max_size );

/** Initializes the asynchronous HDF5 writer instance with a memory buffering
 *  strategy.
 * 
 * The buffer is allocated by Ah5 and reallocated using realloc as required.
 * This is a synonym for ::ah5_init_mem( pself, NULL, 0 );
 * 
 * @param pself a pointer to the Ah5 instance
 * @returns 0 on success, non-null on error
 */
int ah5_init( ah5_t* pself );

/** Finalizes the asynchronous HDF5 writer instance
 * @param self the Ah5 instance
 * @returns 0 on success, non-null on error
 */
int ah5_finalize( ah5_t self );

/** Sets the minimum verbosity level to actually log
 * @param self the Ah5 instance
 * @param log_lvl the minimum verbosity level to actually log
 * @returns 0 on success, non-null on error
 */
int ah5_set_loglvl( ah5_t self, ah5_verbosity_t log_lvl );

/** Sets the FS name of the file where to log
 * @param self the Ah5 instance
 * @param log_file_name the FS name of the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile( ah5_t self, char* log_file_name );

/** Sets the file where to log
 * @param self the Ah5 instance
 * @param log_file the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile_f( ah5_t self, FILE* log_file );

/** Sets the descriptor of the file where to log
 * @param self the Ah5 instance
 * @param log_file_desc the descriptor of the file where to log
 * @returns 0 on success, non-null on error
 */
int ah5_set_logfile_desc( ah5_t self, int log_file_desc );

/** Sets whether to use all OpenMP threads for copy
 * @param self the Ah5 instance
 * @param parallel_copy (0 or 1) whether to use all OpenMP threads
 * @returns 0 on success, non-null on error
 */
int ah5_set_paracopy( ah5_t self, int parallel_copy );

/** Issues a file open command to the queue
 * @param self the Ah5 instance
 * @param file_name the name of the file where to write the data
 * @returns 0 on success, non-null on error
 */
int ah5_open( ah5_t self, char* file_name );

/** Issues a write command to the queue
 * 
 * The provided data should remain valid until the following close command
 * 
 * @param self the Ah5 instance
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
 * @param self the Ah5 instance
 * @returns 0 on success, non-null on error
 */
int ah5_close( ah5_t self );

#endif /* AH5_H__ */

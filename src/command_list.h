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

#ifndef AH5_COMMAND_LIST_H__
#define AH5_COMMAND_LIST_H__

#include <hdf5.h>

#include "logging.h"
#include "ah5.h"


/** Represents an HDF5-write call
 */
typedef struct data_write_s
{
	struct data_write_s *next;
	struct data_write_s *prev;

	/** The data
	 */
	void *buf;

	/** Number of dimensions
	 */
	unsigned rank;

	/** Dimensions of the array
	 */
	hsize_t dims[AH5_MAX_RANK];

	/** Lower bound of the part to write
	 */
	hsize_t lbounds[AH5_MAX_RANK];

	/** Upper bound of the part to write
	 */
	hsize_t ubounds[AH5_MAX_RANK];

	/** HDF5 type of the Data
	 */
	hid_t type;

	/** Name of the Data
	 */
	char *name;

} data_write_t;


void dw_run( data_write_t *cmd, hid_t file, logging_t log );


/** A list of write commands
 */
typedef struct command_list_s
{
	/** The previous element in the list
	 */
	data_write_t *head;
	
	/** The next element in the list
	 */
	data_write_t *tail;
	
} write_list_t;


/** initialize an empty  list
 * @param list the list to initialize
 */
static inline void cl_init( write_list_t *list ) { list->tail = list->head = NULL; }


/** Retrieves the tail of a list
 * @param list the list to access
 * @return the tail of the list
 */
static inline data_write_t *cl_tail( write_list_t *list ) { return list->tail; }


/** Retrieves the head of a list
 * @param list the list to access
 * @return the head of the list
 */
static inline data_write_t *cl_head( write_list_t *list ) { return list->head; }


/** Inserts a new node in the list just before the one provided
 * @param list the list to modify
 * @return the inserted tail
 */
data_write_t *cl_insert_tail( write_list_t *list );


/** Removes the first node from a list
 * @param list the list from which to remove the first node
 */
void cl_remove_head( write_list_t *list );

#endif /* AH5_COMMAND_LIST_H__ */

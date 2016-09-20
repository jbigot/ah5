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

#include "ah5.h"
#include "command_list_fwd.h"


/** Represents an HDF5-write call
 */
typedef struct data_write_s
{
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


/** A node of a double-linked list of data_write_t
 */
struct command_list_s
{
	/** The previous element in the list
	 */
	struct command_list_s *previous;
	
	/** The next element in the list
	 */
	struct command_list_s *next;
	
	/** Access to the list does not grant access to the content of the node.
	 *  It only allows to lock a previously unlocked node or to unlock a node 
	 *  that had been locked by oneself.
	 */
	int content_lock;
	
	/** The actual content of the node
	 */
	data_write_t content;
	
};

#endif /* AH5_COMMAND_LIST_H__ */

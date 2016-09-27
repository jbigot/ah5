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

#include <stdlib.h>

#include "command_list.h"


#if H5_VERS_MAJOR >= 1 && H5_VERS_MINOR >= 8 && H5_VERS_RELEASE >= 14
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_ID_g
#else
#define CLS_DSET_CREATE H5P_CLS_DATASET_CREATE_g
#endif


void dw_run( data_write_t *cmd, hid_t file, logging_t log )
{
	hid_t space_id = H5Screate_simple(cmd->rank, cmd->dims, NULL);
	hid_t plist_id = H5Pcreate(CLS_DSET_CREATE);
	if ( H5Pset_layout(plist_id, H5D_CONTIGUOUS) ) SIGNAL_ERROR(log);
#if ( H5Dcreate_vers == 2 )
	hid_t dset_id = H5Dcreate2( file, cmd->name, cmd->type, space_id,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
#else
	hid_t dset_id = H5Dcreate( file_id, cmd->name, cmd->type, space_id,
			H5P_DEFAULT );
#endif
	if ( H5Dwrite(dset_id, cmd->type, H5S_ALL, H5S_ALL, H5P_DEFAULT,cmd->buf) ) SIGNAL_ERROR(log);
	if ( H5Dclose(dset_id) ) SIGNAL_ERROR(log);
	if ( H5Pclose(plist_id) ) SIGNAL_ERROR(log);
	if ( H5Sclose(space_id) ) SIGNAL_ERROR(log);
}


void cl_remove_head( write_list_t* list )
{
	data_write_t *old_head = list->head;
	list->head = list->head->next;
	if ( list->head ) {
		list->head->prev = NULL;
	} else {
		list->tail = NULL;
	}
	free(old_head);
}


data_write_t *cl_insert_tail( write_list_t *list )
{
	data_write_t *new_node = malloc(sizeof(data_write_t));
	new_node->prev = list->tail;
	new_node->next = NULL;
	if ( list->tail ) {
		list->tail->next = new_node;
	} else {
		list->head = new_node;
	}
	list->tail = new_node;
	return new_node;
}

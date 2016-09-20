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


write_list_t * cl_remove_head( write_list_t* list )
{
	write_list_t *result = list->next;
	if ( list->next == list ) result = NULL;

	list->next->previous = list->previous;
	list->next->previous->next = list->next;

	free(list);
	return result;
}


write_list_t *cl_insert_tail( write_list_t *list )
{
	write_list_t *new_node = malloc(sizeof(write_list_t));
	if ( list ) {
		new_node->previous = list->previous;
		list->previous->next = new_node;
		new_node->next = list;
		list->previous = new_node;
	} else {
		new_node->next = new_node;
		new_node->previous = new_node;
		list = new_node;
	}
	return list;
}

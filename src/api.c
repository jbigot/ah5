/*******************************************************************************
 * Copyright (c) 2015, Julien Bigot - CEA (julien.bigot@cea.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paraconf.h"

#include "ypath.h"
#include "status.h"

#define PC_BUFFER_SIZE 256
#define ERRBUF_SIZE 512

static const char *nodetype[4] = {
	"none",
	"scalar",
	"sequence",
	"mapping"
};

PC_tree_t PC_parse_path(const char *path)
{
	PC_tree_t tree = { PC_OK, NULL, NULL };
	
	FILE *conf_file = fopen(path, "rb");
	if ( !conf_file ) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		handle_error_tree(make_error(PC_SYSTEM_ERROR, errbuf), err0);
	}
	
	PC_errhandler_t handler = PC_errhandler(PC_NULL_HANDLER); // aka PC_try
	tree = PC_parse_file(conf_file);
	PC_errhandler(handler);
	if ( PC_status(tree) ) { // aka PC_catch
		handle_error_tree(make_error(tree.status, "Error while opening file `%s`, %s", path, PC_errmsg()), err1);
	}
	
	fclose(conf_file);
	return tree;
	
err1:
	fclose(conf_file);
	
err0:
	return tree;
}

PC_tree_t PC_parse_file(FILE *conf_file)
{
	PC_tree_t tree = { PC_OK, NULL, NULL };
	
	yaml_parser_t conf_parser; 
	if ( !yaml_parser_initialize(&conf_parser) ) {
		handle_error_tree(make_error(PC_SYSTEM_ERROR, "unable to load yaml library"), err0);
	}
	
	yaml_parser_set_input_file(&conf_parser, conf_file);
	
	yaml_document_t *conf_doc = malloc(sizeof(yaml_document_t));
	if ( !conf_doc ) {
		handle_error_tree(make_error(PC_SYSTEM_ERROR, "unable to allocate memory"), err1);
	}
	
	if ( !yaml_parser_load(&conf_parser, conf_doc) ) {
		if ( conf_parser.context ) {
			handle_error_tree(make_error(PC_INVALID_FORMAT,
					"%lu:%lu: Error: %s \n%lu:%lu: Error: %s",
					(unsigned long) conf_parser.problem_mark.line,
					(unsigned long) conf_parser.problem_mark.column,
					conf_parser.problem,
					(unsigned long) conf_parser.context_mark.line,
					(unsigned long) conf_parser.context_mark.column,
					conf_parser.context), err1);
		} else {
			handle_error_tree(make_error(PC_INVALID_FORMAT, "%lu:%lu: Error: %s",
					(unsigned long) conf_parser.problem_mark.line,
					(unsigned long) conf_parser.problem_mark.column,
					conf_parser.problem), err1);
		}
	}
	
	yaml_parser_delete(&conf_parser);
	
	handle_tree(PC_root(conf_doc), err0);
	
	return tree;
err1:
	yaml_parser_delete(&conf_parser);
err0:
	return tree;
}

PC_tree_t PC_root(yaml_document_t *document)
{
	PC_status_t status = PC_OK;
	PC_tree_t res = { status, document, yaml_document_get_root_node(document) };
	return res;
}

PC_tree_t PC_get(PC_tree_t tree, const char *index_fmt, ...)
{
	va_list ap;
	va_start(ap, index_fmt);
	PC_tree_t res = PC_vget(tree, index_fmt, ap);
	va_end(ap);
	return res;
}

PC_tree_t PC_vget(PC_tree_t tree, const char *index_fmt, va_list va)
{
	if ( tree.status ) return tree;

	int index_size = PC_BUFFER_SIZE;
	char *index = malloc(index_size);
	while ( vsnprintf(index, index_size, index_fmt, va) > index_size ) {
		index_size *= 2;
		index = realloc(index, index_size);
	}

	PC_tree_t res = PC_sget(tree, index);

	free(index);
	return res;
}

PC_status_t PC_len(PC_tree_t tree, int *res)
{
	if ( tree.status ) return tree.status;

	switch ( tree.node->type ) {
	case YAML_SEQUENCE_NODE: {
		*res = tree.node->data.sequence.items.top - tree.node->data.sequence.items.start;
	} break;
	case YAML_MAPPING_NODE: {
		*res = tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start;
	} break;
	case YAML_SCALAR_NODE: {
		*res = tree.node->data.scalar.length;
	} break;
	default: {
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Unknown yaml node type: #%d", tree.node->type);
	} break;
	}
	// the above cases should be exhaustive
	return tree.status;
}

PC_status_t PC_int(PC_tree_t tree, int *res)
{
	if ( tree.status ) return tree.status;

	if ( tree.node->type != YAML_SCALAR_NODE ) {
		return make_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
	}
	char *endptr;
	long result = strtol((char*)tree.node->data.scalar.value, &endptr, 0);
	if ( *endptr ) {
		char *content; tree.status = PC_string(tree, &content);
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Expected integer, found `%s'\n", content);
		free(content);
		return tree.status;
	}
	*res = result;
	return tree.status;
}

PC_status_t PC_double(PC_tree_t tree, double* value)
{
	if ( tree.status ) return tree.status;

	if ( tree.node->type != YAML_SCALAR_NODE ) {
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
	}
	char *endptr;
	*value = strtod((char*)tree.node->data.scalar.value, &endptr);
	if ( *endptr ) {
		char *content; tree.status = PC_string(tree, &content);
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected floating point, found `%s'\n", content);
		free(content);
	}
	return tree.status;
}

PC_status_t PC_string(PC_tree_t tree, char** value)
{
	if ( tree.status ) return tree.status;

	if ( tree.node->type != YAML_SCALAR_NODE ) {
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
	}

	int len; tree.status = PC_len(tree, &len); if (tree.status) return tree.status;
	*value = malloc(len+1);

	strncpy(*value, (char*)tree.node->data.scalar.value, len+1);
	assert((*value)[len]==0);
	return tree.status;
}

PC_status_t PC_broadcast(yaml_document_t* document, int count, int root, MPI_Comm comm)
{
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);
	yaml_emitter_set_width(&emitter, -1);
	yaml_emitter_set_canonical(&emitter, 1);
	yaml_emitter_open(&emitter);

	count = count; // prevent unused warning

	size_t buf_size = PC_BUFFER_SIZE/2;
	unsigned char *buf = 0;
	int err = YAML_WRITER_ERROR;
	unsigned long data_size;
	while ( err == YAML_WRITER_ERROR ) {
		buf_size *= 2;
		buf = realloc(buf, buf_size);
		yaml_emitter_set_output_string(&emitter, buf, buf_size, &data_size);
		err = yaml_emitter_dump(&emitter, document);
	}
	yaml_emitter_close(&emitter);
	yaml_emitter_delete(&emitter);

	MPI_Bcast(&data_size, 1, MPI_LONG, root, comm);
	MPI_Bcast(buf, data_size, MPI_LONG, root, comm);

	int rank; MPI_Comm_rank(comm, &rank);
	if ( rank != root ) {
		yaml_parser_t parser;
		yaml_parser_initialize(&parser);
		yaml_parser_set_input_string(&parser, buf, data_size);
		yaml_parser_load(&parser, document);
	}
	return PC_OK;
}

PC_status_t PC_tree_destroy(PC_tree_t* tree)
{
	yaml_document_delete(tree->document);
	free(tree->document);
	tree->node = NULL;
	return tree->status;
}
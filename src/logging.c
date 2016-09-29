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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ah5.h"

#include "logging.h"


void log_init( logging_t *log )
{
	log->file = stderr;
	log->closing_strategy = FILE_KEEP_OPEN;
	log->verbosity = AH5_VERB_ERROR;
}


void log_destroy( logging_t *log )
{
	if ( log->closing_strategy == FILE_CLOSE ) {
		fclose(log->file);
	}
	log->file = NULL;
	log->closing_strategy = FILE_KEEP_OPEN;
}


void log_set_lvl( logging_t *log, ah5_verbosity_t log_lvl )
{
	log->verbosity = log_lvl;
}


int log_set_filename( logging_t *log, char* file_name )
{
	log_destroy(log);
	int log_fd = open(file_name, O_WRONLY|O_APPEND|O_CREAT|O_SYNC|O_DSYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if ( log_fd == -1 ) RETURN_ERROR(*log);
	log->file = fdopen(log_fd, "a");
	log->closing_strategy = FILE_CLOSE;
	if ( !log->file ) RETURN_ERROR(*log);
	return 0;
}


int log_set_file( logging_t *log, FILE* file, int keep_open )
{
	log_destroy(log);
	log->file = file;
	if ( !keep_open ) log->closing_strategy = FILE_CLOSE;
	return 0;
}


int log_set_filedesc( logging_t *log, int file_desc, int keep_open )
{
	log_destroy(log);
	log->file = fdopen(file_desc, "a");
	if ( !log->file ) RETURN_ERROR(*log);
	if ( !keep_open ) log->closing_strategy = FILE_CLOSE;
	return 0;
}

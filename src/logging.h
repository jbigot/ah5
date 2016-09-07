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

#ifndef AH5_LOGGING_H__
#define AH5_LOGGING_H__

#include <stdio.h>


#define LOG_ERROR( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_ERROR) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Error: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)


#define LOG_WARNING( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_WARNING) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Warning: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)


#define LOG_STATUS( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_STATUS) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Status: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)


#ifndef NDEBUG
#define LOG_DEBUG( ... ) do {\
	if (self->log_verbosity >= VERBOSITY_DEBUG) {\
		FILE* out = self->log_file? self->log_file : stderr;\
		fprintf(out, "*** Log: %s:%d: ", __FILE__, __LINE__);\
		fprintf(out , ##__VA_ARGS__);\
		fprintf(out, "\n");\
		fflush(out);\
	}\
} while (0)
#else
#define LOG_DEBUG( ... )
#endif

#include <errno.h>
#include <stdio.h>


// 	LOG_ERROR(" ----- Fatal: exiting -----\n");
#define SIGNAL_ERROR do {\
	int errno_save = errno;\
	errno = errno_save;\
	perror(NULL);\
	exit(errno_save);\
} while (0)


// 	LOG_WARNING(" ----- Leaving function -----\n");
#define RETURN_ERROR do {\
	int errno_save = errno;\
	errno = errno_save;\
	perror(NULL);\
	errno = errno_save;\
	return errno_save;\
} while (0)

/** Returns the number of microseconds elapsed since EPOCH
 * @returns the number of microseconds elapsed since EPOCH
 */
inline static int64_t clockget()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t)tv.tv_sec*1000*1000 + tv.tv_usec;
}

#endif /* AH5_LOGGING_H__ */

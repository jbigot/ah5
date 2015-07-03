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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ah5.h>

#define DATA_WIDTH 1024
#define DATA_HEIGHT 1024
#define ACC(data, xx, yy) data[(((xx)+DATA_WIDTH)%DATA_WIDTH)+(((yy)+DATA_HEIGHT)%DATA_HEIGHT)*DATA_WIDTH]

void data_init(double *data)
{
	int xx, yy;
	for (yy=0; yy<DATA_HEIGHT; ++yy) {
		for (xx=0; xx<DATA_WIDTH; ++xx) {
			ACC(data, xx, yy) = sin(1.*xx/DATA_WIDTH)*sin(1.*yy/DATA_HEIGHT);
		}
	}
}

void data_compute(double *data_in, double *data_out)
{
	int xx, yy;
	for (yy=0; yy<DATA_HEIGHT; ++yy) {
		for (xx=0; xx<DATA_WIDTH; ++xx) {
			ACC(data_out, xx, yy) =
					.5 * ACC(data_in, xx, yy)
					+ .125 * ACC(data_in, xx-1, yy)
					+ .125 * ACC(data_in, xx+1, yy)
					+ .125 * ACC(data_in, xx, yy-1)
					+ .125 * ACC(data_in, xx, yy+1)
			;
		}
	}
}

int main(int argc, char** argv)
{
	ah5_t ah5_inst;
	double *data, *data_next, *data_swap;
	int ii;
	char fname[15];
	hsize_t zsize[2] = {0, 0};
	hsize_t bounds[2] = { DATA_WIDTH, DATA_HEIGHT };


	ah5_init(&ah5_inst);
	data = malloc(DATA_WIDTH*DATA_HEIGHT*sizeof(double));
	data_init(data);
	data_next = malloc(DATA_WIDTH*DATA_HEIGHT*sizeof(double));

	for (ii=0; ii<100; ++ii) {
		if (ii%10 == 0) {
			sprintf(fname, "data.%d.h5", ii);
			ah5_start(ah5_inst, fname);
			ah5_write(ah5_inst, data, "data", H5T_NATIVE_DOUBLE, 2, bounds, zsize, bounds );
			ah5_finish(ah5_inst);
		}
		data_compute(data, data_next);
		data_swap = data;
		data = data_next;
		data_next = data_swap;
	}
	sprintf(fname, "data.%d.h5", ii);
	ah5_start(ah5_inst, fname);
	ah5_write(ah5_inst, data, "data", H5T_NATIVE_DOUBLE, 2, bounds, zsize, bounds );
	ah5_finish(ah5_inst);

	ah5_finalize(ah5_inst);
	free(data);
	free(data_next);
	return 0;
}
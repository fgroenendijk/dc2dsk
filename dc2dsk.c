/*

dc2dsk 0.1
Because Power Macs (and even Linux lovers) should enjoy Floppy Emu too.

Based on DiskCopy2Dsk by Stephen Chamberlin (C) 2013.
(C)2014 Cameron Kaiser. All rights reserved.
ckaiser@floodgap.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	size_t datasize, paddingsize;
	unsigned char *metadata;

	if (argc > 1) {
		fprintf(stderr,
"usage: %s\n\n"
"pass a DiskCopy image on standard input;\n"
"a dsk image will exit on standard output.\n"
		, argv[0]);
		return 1;
	}

/* The meat:

   length is a big-endian 32-bit word at offset 0x40-0x43
   magic words at 0x52 and 0x53 should be 0x01 and 0x00 respectively
   disk image starts at 0x54 */

	metadata = (unsigned char *)malloc(0x54);
	if(fread(metadata, sizeof(unsigned char), 0x54, stdin) != 0x54) {
		perror("failed to read metadata");
		return 1;
	}
	if (metadata[0x52] != 0x01 || metadata[0x53] != 0x00) {
		fprintf(stderr,
			"this doesn't seem to be a DiskCopy 4.2 image\n");
		return 1;
	}

	/* I'm doing you x86 suckers a solid. I could have just treated
	   this as a big-endian integer and it would work just fine on my
	   G5. So pay it forward, mmkay? */
	datasize = metadata[0x40] * 16777216 +
		metadata[0x41] * 65536 +
		metadata[0x42] * 256 +
		metadata[0x43];
	fprintf(stderr, "data size: %i\n", datasize);
	if (datasize > 1440*1024) {
		fprintf(stderr, "this image size is too large to process\n");
		return 1;
	}

	/* Some Disk Copy images are shorter than the datasize that
	   Floppy Emu supports, so we'll just politely pad them out.
	   They work just fine. */
	paddingsize =	(datasize <= 400*1024) ?
		(400*1024 - datasize) :
			(datasize <= 800*1024) ?
		(800*1024 - datasize) :
			(datasize <= 1440*1024) ?
		(1440*1024 - datasize) : 0;
	if (paddingsize)
		fprintf(stderr, "warning: needs %i bytes of padding\n",
			paddingsize);

	while(datasize--) {
		if(feof(stdin)) {
			fprintf(stderr,
				"unexpected end of file! (%i bytes left)\n",
				datasize);
			return 1;
		}
		if(!fprintf(stdout, "%c", fgetc(stdin))) {
			perror("failed during output");
			return 1;
		}
	}
	if (paddingsize) {
		while(paddingsize--) {
			if(fputc(0, stdout)) {
				perror("failed during padding");
				return 1;
			}
		}
	}
	fprintf(stderr, "ok!\n");
	return 0;
}

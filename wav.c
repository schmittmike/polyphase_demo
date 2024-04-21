#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wav.h"

#define DEBUG 1

/* mallocs the wav sample pointers
 * returns total bytes read from data section of wav header
 *
 * this function is not general purpose and is purpose written for a specific
 * wav file, other files with different header information might not work */
int load_wav_file(const char *filename, struct Wav *data)
{
	int i;
	int read = 0;
	uint8_t *raw;	// stage data before convert endianness / split channel
	uint8_t buf[5]; // place to read header data into

	/* open .wav file and start reading header information */
	FILE *w = fopen(filename, "rb");
	if (w == NULL) {
		perror(filename);
		exit(1);
	}

	printf("[INFO] reading %s\n", filename);

        read += fread(buf, sizeof(uint8_t), 4, w); // "RIFF"
        read += fread(buf, sizeof(uint8_t), 4, w); // file size
        read += fread(buf, sizeof(uint8_t), 4, w); // "WAVE"
        read += fread(buf, sizeof(uint8_t), 4, w); // "fmt\0"

        read += fread(buf, sizeof(uint8_t), 4, w); // (17-20) format length (16)
        read += fread(buf, sizeof(uint8_t), 2, w); // (21-22) format type
        read += fread(buf, sizeof(uint8_t), 2, w); // (23-24) channel count

	/* get sample rate, stored little endian */
        read += fread(buf, sizeof(uint8_t), 4, w); // (25-28) sample rate
	data->sample_rate = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	printf("\tsample rate: %d\n", data->sample_rate);

        read += fread(buf, sizeof(uint8_t), 4, w); // (29-32) total bytes/sec
        read += fread(buf, sizeof(uint8_t), 2, w); // (33-34) bps*channels

	/* get bits per sample, stored little endian */
        read += fread(buf, sizeof(uint8_t), 2, w); // (35-36) bits per sample
	data->bits_per_sample = buf[0] | (buf[1] << 8);
#if DEBUG
	printf("\tbits per sample: %d\n", data->bits_per_sample);
#endif

	/* this is extra stuff that happens to be in my .wav file
	 * i don't feel like actually parsing the file anymore, so im just going
	 * to read out the rest of the header until i'm at the data */
        read += fread(buf, sizeof(uint8_t), 4, w); // (37-40) "LIST"
#if DEBUG
	printf("\t%c%c%c%c\n", buf[0], buf[1], buf[2], buf[3]);
#endif

        read += fread(buf, sizeof(uint8_t), 4, w);
        read += fread(buf, sizeof(uint8_t), 4, w); // "INFO"
        read += fread(buf, sizeof(uint8_t), 4, w); // "ISFT"
        read += fread(buf, sizeof(uint8_t), 4, w);
        read += fread(buf, sizeof(uint8_t), 4, w); // "Lavf"
        read += fread(buf, sizeof(uint8_t), 4, w);
        read += fread(buf, sizeof(uint8_t), 4, w);
        read += fread(buf, sizeof(uint8_t), 2, w);
        read += fread(buf, sizeof(uint8_t), 4, w); // "data"
#if DEBUG
	printf("\t%c%c%c%c\n", buf[0], buf[1], buf[2], buf[3]);
#endif
	data->header_size = read;

        read += fread(buf, sizeof(uint8_t), 4, w); // data size in bytes
	data->data_size = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
#if DEBUG
       	printf("\tdata section size: %d\n", data->data_size);
#endif

	/* data size / 2 channels / 2 bytes per sample */
	data->N = data->data_size / 4;
	printf("\tsamples : %d\n\n", data->N);

	/* the rest of my file is unsigned 16 bit data in 2 channels */
	raw = malloc(data->data_size*sizeof(uint8_t));
	if (raw == NULL) {
		perror("can't hold wav");
		exit(1);
	}

	/* get every sample into raw array, process each sample */
	read = fread(raw, sizeof(uint8_t), data->data_size, w);
	data->rsamp = malloc(data->N*sizeof(int16_t));
	data->lsamp = malloc(data->N*sizeof(int16_t));

	/* read samples and correct for endianness */
	for (i = 0; i < data->N; i++) {
		data->rsamp[i] = (raw[4*i + 0] + (raw[4*i + 1] << 8));
		data->lsamp[i] = (raw[4*i + 2] + (raw[4*i + 3] << 8));
	}

	free(raw);
	return read;
}


/* store data as wav file at filename.
 *
 * this function is hard coded for stereo 16-bit samples*/
int store_wav_file(const char *filename, struct Wav *data)
{
	int i;
	int written;
	uint32_t value;
	char buf[44];
	uint8_t *raw;

	/* open .wav file for writing */
	FILE *w = fopen(filename, "w+b");
	if (w == NULL) {
		perror(filename);
		exit(1);
	}

	printf("[INFO] saving samples to %s\n", filename);

	for (i = 0; i<44; i++) buf[i] = 0;

	/* 0-3 file mark */
	strncpy(&buf[0], "RIFF", 4);

	/* 4-7 overall size */
	value = 44 + data->data_size;
#if DEBUG
	printf("\ttotal file size: %d\n", value);
#endif
	buf[4] = (value & 0x000000ff) >> 0;
	buf[5] = (value & 0x0000ff00) >> 8;
	buf[6] = (value & 0x00ff0000) >> 16;
	buf[7] = (value & 0xff000000) >> 24;

	/* 8-15 file mark */
	strncpy(&buf[8], "WAVEfmt ", 8);

	/* 17-20 fmt section size */
	buf[16] = 16;

	/* 21-22 format type */
	buf[20] = 1;

	/* 23-24 channels */
	buf[22] = 2;

	/* 25-28 sample rate */
	value = data->sample_rate;
#if DEBUG
	printf("\tsample rate: %d\n", value);
#endif
	buf[24] = (value & 0x000000ff) >> 0;
	buf[25] = (value & 0x0000ff00) >> 8;
	buf[26] = (value & 0x00ff0000) >> 16;
	buf[27] = (value & 0xff000000) >> 24;

	/* 29-32 samplerate * 2bits per channel * channels / 8bitsperbyte */
	value = (data->sample_rate*16*2) / 8;
#if DEBUG
	printf("\tbytes per second: %d\n", value);
#endif
	buf[28] = (value & 0x000000ff) >> 0;
	buf[29] = (value & 0x0000ff00) >> 8;
	buf[30] = (value & 0x00ff0000) >> 16;
	buf[31] = (value & 0xff000000) >> 24;

	/* 33-34 16-bit stereo mode */
	buf[32] = 4;

	/* 35-36 bits per sample*/
	buf[34] = 16;

	/* 37-40 data header marker */
	strncpy(&buf[36], "data", 4);

	/* 41-44 total bytes in data section */
	value = data->data_size;
#if DEBUG
	printf("\tsample rate: %d\n", value);
#endif
	buf[40] = (value & 0x000000ff) >> 0;
	buf[41] = (value & 0x0000ff00) >> 8;
	buf[42] = (value & 0x00ff0000) >> 16;
	buf[43] = (value & 0xff000000) >> 24;

	written = fwrite(buf, sizeof(uint8_t), 44, w);

	raw = malloc(data->N*2*2*sizeof(uint8_t));
	if (raw == NULL) {
		perror("can't store wav");
		exit(1);
	}
	for (i = 0; i<data->N; i++) {
		value = data->rsamp[i];
		raw[4*i+0] = ((value & 0x00ff) >> 0);
		raw[4*i+1] = ((value & 0xff00) >> 8);

		value = data->lsamp[i];
		raw[4*i+2] = ((value & 0x00ff) >> 0);
		raw[4*i+3] = ((value & 0xff00) >> 8);
	}
	written = fwrite(raw, sizeof(uint8_t), data->N*2*2, w);
#if DEBUG
	printf("\tbytes written: %d\n", written);
#endif
	free(raw);
	return written;
}


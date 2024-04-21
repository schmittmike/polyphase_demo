#ifndef WAV_H
#define WAV_H 1

struct Wav {
	int N;			//sample count
	int sample_rate;
	int bits_per_sample;
	int header_size;	// size of metadata in bytes
	int data_size;		// size of data section in bytes
	int16_t *rsamp;		// right channel
	int16_t *lsamp;		// left channel
};

int load_wav_file(const char *filename, struct Wav *data);
int store_wav_file(const char *filename, struct Wav *data);

#endif

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wav.h"
#include "bandpass.h"

#define DEBUG 0
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof((X)[0]))

struct linear_convolve_arguments {
	double *signal;
	size_t signal_len;
	double *kernel;
	size_t kernel_len;
	double *result;
};

void polyphase_fir_2channel_wav(struct Wav *input, struct Wav *result,
		double *coefs, int n_coefs, int n_taps);

void *linear_convolve(void *p);


/* https://stackoverflow.com/questions/8424170/1d-linear-convolution-in-ansi-c-code
 * sample linear convolution code found here*/
void *linear_convolve(void *p)
{
	/* recast pthreads argument */
	struct linear_convolve_arguments *args = (struct linear_convolve_arguments *)p;

	size_t n;

	//double *signal = args->signal;
	//size_t signal_len = args->signal_len;
	//double *kernel = args->kernel;
	//size_t kernel_len = args->kernel_len;
	//double *result = args->result;


	for (n = 0; n < args->signal_len + args->kernel_len - 1; n++)
	{
		size_t kmin, kmax, k;

		args->result[n] = 0;

		/* calculate the overlap of the two signals */
		kmin = (n >= args->kernel_len - 1) ? n - (args->kernel_len - 1) : 0;
		kmax = (n < args->signal_len - 1) ? n : args->signal_len - 1;

		for (k = kmin; k <= kmax; k++)
		{
			args->result[n] += args->signal[k] * args->kernel[n - k];
		}
	}
}


/* filters/decimates input data
 * number of fir coefficients must divide equally between number of taps used
 *
 * also counts on the result Wav having its lsamp and rsamp not malloc-ed */
void polyphase_fir_2channel_wav(struct Wav *input, struct Wav *result,
		double *coefs, int n_coefs, int n_taps)
{
	int i, j;
	int M, N;
	double **x, **h, **r;

	struct linear_convolve_arguments *args = malloc(M*sizeof(struct linear_convolve_arguments));
	pthread_t *tid = calloc(M, sizeof(pthread_t));
	void *thread_return;

	M = n_taps;
	//N = 600000;
	N = input->N;

	/* make sure that the sample count is divisible into each tap */
	N = N - N%M;

	h = malloc(M*sizeof(double *));
	//
	//TODO: add second channel

	/* carefully split input streams into M taps:
	 * the way the polyphase filter derivation works determines the way that
	 * the taps are filled.
	 *
	 * there's for sure a much much cleaner way to do this but the index
	 * list is the easiest i came up with*/

	int *indices = malloc(M*sizeof(int));
	int *x_fill = malloc(M*sizeof(int));

	indices[M-1] = 1;
	/* cover case of M=1 */
	indices[0] = 0;
	x_fill[0] = 0;

	j = 1;
	for (i = M-1; i >= 1; i--) {
		indices[i] = j++;
		x_fill[i] = 1;
	}

#if DEBUG
	printf("indices: ");
	for (i = 0; i < M; i++) {
		printf("%d ", indices[i]);
	}
	printf("\n");
	printf("x_fill: ");
	for (i = 0; i < M; i++) {
		printf("%d ", x_fill[i]);
	}
	printf("\n");
#endif

	/* allocate each tap's portion of the data stream */
	x = malloc(M*sizeof(double *));
	for (i = 0; i<M; i++) {
		x[i] = malloc(ceil( (N+(M+1))/M )*sizeof(double));
	}

	/* fill the x arrays according to x_fill and indices */
	for (i = 0; i<N; i++) {
		x[ indices[i%M] ][ x_fill[i%M]++ ] = (double)input->rsamp[i];
	}

	/* allocate space for the split coefficients */
	for (i = 0; i<M; i++) {
		h[i] = malloc((n_coefs/M)*sizeof(double));
	}

	/* split fir coefficients into M taps */
	for (i = 0; i<M; i++) {
		for (j = 0; j < n_coefs/M; j++) {
			h[i][j] = coefs[i+(j*M)];
		}
	}

#if (DEBUG == 2)
	printf("x: ");
	for (i = 0; i < M; i++) {
		printf("\nx[%d]: ", i);
		for (j = 0; j < N/M; j++) {
			printf("%f ", x[i][j]);
		}
	}
	printf("\n");
	for (i = 0; i < M; i++) {
		printf("\nh[%d]: ", i);
		for (j = 0; j < M*2; j++) {
			printf("%f ", h[i][j]);
		}
	}
	printf("\n");
#endif

	/* allocate result arrays for each tap */
	r = malloc(M*sizeof(double *));
	for (i = 0; i<M; i++) {
		/* result of 1d convolution is len signal+kernel+1 */
		r[i] = malloc((N/M + n_coefs/M + 1)*sizeof(double));
		//r = malloc((N + n_coefs + 1)*sizeof(double));
	}

	/* convolve! */
	for (i = 0; i < M; i++) {
		args[i].signal = x[i];
		args[i].signal_len = N/M;
		args[i].kernel = h[i];
		args[i].kernel_len = n_coefs/M;
		args[i].result = r[i];
	}
	for (i = 0; i < M; i++) {
		printf("starting tap %d...\n", i);
		pthread_create(&tid[i], NULL, linear_convolve, &args[i]);
	}
	for (i = 0; i < M; i++) {
		pthread_join(tid[i], &thread_return);
	}

	for (i = 1; i < M; i++) {
		for (j = 0; j<N/M; j++) {
			r[0][j] += r[i][j];
		}
	}

#if DEBUG
	for (i = 0; i<5; i++) {
		printf("%lf\n", r[0][i]);
	}
#endif

	/* sum results of each convolution */
	result->rsamp = malloc((N/M)*sizeof(uint16_t));
	result->lsamp = malloc((N/M)*sizeof(uint16_t));
	result->sample_rate = input->sample_rate/M;
	result->N = (N/M);
	result->data_size = (N/M)*2*2;

	for (i = 0; i<N/M; i++) {
		result->rsamp[i] = (uint16_t)r[0][i];
		result->lsamp[i] = (uint16_t)r[0][i];
	}

	/* save samples to result and save new sample-rate */

	/* TODO: more complete frees? */
	for (i = 0; i<M; i++) {
		free(x[i]);
		free(h[i]);
		free(r[i]);
	}
	free(x);
	free(h);
	free(r);

	return;
}


int main(int argc, char **argv)
{
	struct Wav mega;
	struct Wav out;
	int taps = 1;

	/* TODO: i/o filenames in args, input validation */
	if (argc == 2) {
		taps = atoi(argv[1]);
	} else {
		printf("[INFO] usage: %s <n_taps=1>\n", argv[0]);
	}

	load_wav_file("/home/sch/Music/megalomania.wav", &mega);

	printf("[INFO] using %d taps...\n", taps);
	polyphase_fir_2channel_wav(&mega, &out, bandpass_coefs, BANDPASS_N_COEFS, taps);

	store_wav_file("test_mega.wav", &out);

	return 0;
}

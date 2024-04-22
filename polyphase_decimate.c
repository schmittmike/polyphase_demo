#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wav.h"
#include "bandpass.h"

#define DEBUG 0
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof((X)[0]))

void polyphase_fir_2channel_wav(struct Wav *input, struct Wav *result,
		double *coefs, int n_coefs, int n_taps);

void linear_convolve(const double signal[/* signal_len */], size_t signal_len,
		     const double kernel[/* kernel_len */], size_t kernel_len,
		     double result[/* signal_len + kernel_len - 1 */]);


/* https://stackoverflow.com/questions/8424170/1d-linear-convolution-in-ansi-c-code
 * sample linear convolution code found here*/
void linear_convolve(const double signal[/* signal_len */], size_t signal_len,
		     const double kernel[/* kernel_len */], size_t kernel_len,
		     double result[/* signal_len + kernel_len - 1 */])
{
	size_t n;

	for (n = 0; n < signal_len + kernel_len - 1; n++)
	{
		size_t kmin, kmax, k;

		result[n] = 0;

		/* calculate the overlap of the two signals */
		kmin = (n >= kernel_len - 1) ? n - (kernel_len - 1) : 0;
		kmax = (n < signal_len - 1) ? n : signal_len - 1;

		for (k = kmin; k <= kmax; k++)
		{
			result[n] += signal[k] * kernel[n - k];
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
	M = n_taps;
	N = 600000;
	//N = input->N;
	double **x, **h, **r;

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
	h = malloc(n_coefs*sizeof(double*));
	for (i = 0; i < M; i++) {
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

	/* convolve! */

	r = malloc(M*sizeof(double *));
	for (i = 0; i<M; i++) {
		/* result of 1d convolution is len signal+kernel+1 */
		r[i] = malloc((N/M + n_coefs/M + 1)*sizeof(double));
		//r = malloc((N + n_coefs + 1)*sizeof(double));
	}
	for (i = 0; i < M; i++) {
		printf("starting tap %d...\n", i);
		linear_convolve(x[i], N/M, h[i], n_coefs/M, r[i]);
	}
	for (i = 1; i < M; i++) {
		for (j = 0; j<N/M; j++) {
			r[0][j] += r[i][j];
		}
	}

#if DEBUG
	for (i = 0; i<16; i++) {
		printf("%lf\n", r[i]);
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


int main(void)
{
	struct Wav mega;
	struct Wav out;

	load_wav_file("/home/sch/Music/megalomania.wav", &mega);

	polyphase_fir_2channel_wav(&mega, &out, bandpass_coefs, BANDPASS_N_COEFS, 6);

	store_wav_file("test_mega.wav", &out);

	return 0;
}

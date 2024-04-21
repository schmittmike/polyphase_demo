#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "wav.h"
#include "bandpass.h"

#define ELEMENT_COUNT(X) (sizeof(X) / sizeof((X)[0]))

void polyphase_fir_2channel_wav(struct Wav *input, struct Wav *result,
		double *coefs, int n_coefs);

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
		double *coefs, int n_coefs)
{
	int i;
	int M, N;
	N = 10000;
	double *x, *h;
	/* result of 1d convolution is len signal+kernel+1 */
	double *r = malloc((N + n_coefs + 1)*sizeof(double));

	// FOR NOW: 1 tap 1 ch hardcode:

	/* carefully split input streams into M taps */
	x = malloc(N*sizeof(double));
	for (i = 0; i<N; i++) {
		x[i] = (double)input->rsamp[i];
	}

	/* split fir coefficients into M taps */
	h = malloc(n_coefs*sizeof(double));
	for (i = 0; i<n_coefs; i++) {
		h[i] = coefs[i];
	}

	/* convolve! */
	linear_convolve(x, N, h, n_coefs, r);

	/* sum results of each convolution */
	for (i = 0; i<16; i++) {
		printf("r[%d]: %lf\n", i, r[i]);
	}

	/* save samples to result and save new sample-rate */
	free(x);
	free(h);
	free(r);

	return;
}


int main(void)
{
	int i;
	struct Wav mega;
	struct Wav out;

	load_wav_file("/home/sch/Music/megalomania.wav", &mega);

	polyphase_fir_2channel_wav(&mega, &out, bandpass_coefs, BANDPASS_N_COEFS);

	store_wav_file("test_mega.wav", &mega);

	return 0;
}

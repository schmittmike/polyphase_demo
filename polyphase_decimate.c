#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "wav.h"

#define ELEMENT_COUNT(X) (sizeof(X) / sizeof((X)[0]))


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

		kmin = (n >= kernel_len - 1) ? n - (kernel_len - 1) : 0;
		kmax = (n < signal_len - 1) ? n : signal_len - 1;

		for (k = kmin; k <= kmax; k++)
		{
			result[n] += signal[k] * kernel[n - k];
		}
	}
}


int main(void)
{
	int i;

	struct Wav mega;
	load_wav_file("/home/sch/Music/megalomania.wav", &mega);

	for (i = 0; i<10; i++) {
		printf("l: %d r: %d \n", mega.rsamp[i], mega.lsamp[i]);
	}

	return 0;
}

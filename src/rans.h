#ifndef RANS_H
#define RANS_H

#include <stdint.h>
#include <vector>

namespace crt {

class Rans
{
public:
	Rans(int _n_symbols): n_symbols(_n_symbols) {
		freqs     = new uint32_t[n_symbols];
		cum_freqs = new uint32_t[n_symbols+1];
	}
	~Rans() {
		delete []freqs;
		delete []cum_freqs;
	}
	
	void getProbabilities(unsigned char *data, int size) {
		count_freqs(data, size);
		normalize_freqs(1<<prob_bits);
	}
	
	unsigned char *compress(unsigned char *data, int input_size, int &output_size);
	void decompress(unsigned char *data, int input_size, unsigned char *output, int output_size);

	int n_symbols;
	int prob_bits = 14;
	
	uint32_t *freqs;
	uint32_t *cum_freqs;
	
	void count_freqs(uint8_t const* in, uint32_t nbytes);
	void calc_cum_freqs();
	void normalize_freqs(uint32_t target_total);
};

}
#endif // RANS_H

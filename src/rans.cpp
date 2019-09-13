#include "rans.h"
#include "rans_byte.h"
#include <string.h>
#include <assert.h>

#include <iostream>
using namespace std;

using namespace crt;


void Rans::count_freqs(uint8_t const* in, uint32_t nbytes) {
	for (int i=0; i < n_symbols; i++)
		freqs[i] = 0;
	
	for (uint32_t i=0; i < nbytes; i++)
		freqs[in[i]]++;
	
	uint32_t last = 0;
	for(int i = 1; i < n_symbols; i++)
		if(freqs[i] != 0)
			last = i;
	//n_symbols = last+1;
}

void Rans::calc_cum_freqs() {
	cum_freqs[0] = 0;
	for (int i=0; i < n_symbols; i++)
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void Rans::normalize_freqs(uint32_t target_total)
{
	assert(target_total >= n_symbols);
	
	calc_cum_freqs();
	uint32_t cur_total = cum_freqs[n_symbols];
	
	// resample distribution based on cumulative freqs
	for (int i = 1; i <= n_symbols; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;
	
	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for (int i=0; i < n_symbols; i++) {
		if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
			// symbol i was set to zero freq
			
			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for (int j=0; j < n_symbols; j++) {
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);
			
			// and steal from it!
			if (best_steal < i) {
				for (int j = best_steal + 1; j <= i; j++)
					cum_freqs[j]--;
			} else {
				assert(best_steal > i);
				for (int j = i + 1; j <= best_steal; j++)
					cum_freqs[j]++;
			}
		}
	}
	
	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[n_symbols] == target_total);
	for (int i=0; i < n_symbols; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);
		
		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

unsigned char *Rans::compress(unsigned char *data, int input_size, int &output_size) {
	/*
	cout << "Data begin: ";
	for(int i = 0; i < 100 && i < input_size; i++)
		cout << (int)data[i] << " ";
	cout << endl;
	
	cout << "Freqs :";
	for(int i =0; i < n_symbols; i++)
		cout << freqs[i] << " ";
	cout << endl;
	
	
	cout << "Cum Freqs :";
	for(int i =0; i < n_symbols; i++)
		cout << cum_freqs[i] << " ";
	cout << endl; */
	
	//optimization for single value data!
	/*if(probabilities.size() == 1) {
		output_size = 0;
		return NULL;
	}*/
	RansEncSymbol esyms[n_symbols];
	for (int i=0; i < n_symbols; i++)
		RansEncSymbolInit(&esyms[i], cum_freqs[i], freqs[i], prob_bits);

	RansState rans;
	RansEncInit(&rans);
	
	int max_size = input_size*2;
	unsigned char *output = new unsigned char[max_size]; //use entropy here!
	
	uint8_t* ptr = output + max_size; // *end* of output buffer
	for (size_t i = input_size; i > 0; i--) { // NB: working in reverse!
		int s = data[i-1];
		RansEncPutSymbol(&rans, &ptr, &esyms[s]);
	}
	RansEncFlush(&rans, &ptr);
	
	output_size = (output + max_size) - ptr;
	memmove(output, ptr, output_size);
	
	return output;
}


void Rans::decompress(unsigned char *data, int input_size, unsigned char *output, int output_size) {

	uint8_t cum2sym[1<<prob_bits];
	for (int s = 0; s < n_symbols; s++)
		for (uint32_t i = cum_freqs[s]; i < cum_freqs[s+1]; i++)
			cum2sym[i] = s;
	
	RansDecSymbol dsyms[n_symbols];
	for (int i=0; i < n_symbols; i++)
		RansDecSymbolInit(&dsyms[i], cum_freqs[i], freqs[i]);
	
	//optimization for single value data!
	/*if(probabilities.size() == 1) {
		memset(output, probabilities[0].symbol, output_size);
		return;
	}*/
	
	RansState rans;
	uint8_t* ptr = data;
	RansDecInit(&rans, &ptr);
	
	for (size_t i=0; i < output_size; i++) {
		uint32_t s = cum2sym[RansDecGet(&rans, prob_bits)];
		output[i] = (uint8_t) s;
		RansDecAdvanceSymbol(&rans, &ptr, &dsyms[s], prob_bits);
	}	
}


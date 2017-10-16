/*
Corto

Copyright(C) 2017 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  You should have received 
a copy of the GNU General Public License along with Corto.
If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <deque>
#include <algorithm>
#include <iostream>
#include "tunstall.h"

using namespace std;
using namespace crt;

struct TSymbol {
	int offset;
	int length;
	uint32_t probability;
	TSymbol() {}
	TSymbol(uint32_t p, int o, int l): offset(o), length(l), probability(p)  {}
};

/* Left as an example of how to compress to a stream

int Tunstall::compress(Stream &stream, unsigned char *data, int size) {

	Tunstall t;
	t.getProbabilities(data, size);

	t.createDecodingTables();
	t.createEncodingTables();

	int compressed_size;
	unsigned char *compressed_data = t.compress(data, size, compressed_size);

	stream.write<uchar>(t.probabilities.size());
	stream.writeArray<uchar>(t.probabilities.size()*2, (uchar *)t.probabilities.data());


	stream.write<int>(size);
	stream.write<int>(compressed_size);
	stream.writeArray<unsigned char>(compressed_size, compressed_data);
	delete []compressed_data;
	//return compressed_size;
	return 1 + t.probabilities.size()*2 + 4 + 4 + compressed_size;
}

void Tunstall::decompress(Stream &stream, std::vector<unsigned char> &data) {

	Tunstall t;
	int nsymbols = stream.readUint8();
	uchar *probs = stream.readArray<uchar>(nsymbols*2);
	t.probabilities.resize(nsymbols);
	memcpy(t.probabilities.data(), probs, nsymbols*2);

	t.createDecodingTables();

	int size = stream.read<int>();
	data.resize(size);
	int compressed_size = stream.read<int>();
	unsigned char *compressed_data = stream.readArray<unsigned char>(compressed_size);

	if(size)
		t.decompress(compressed_data, compressed_size, data.data(), size);
} */


void Tunstall::getProbabilities(unsigned char *data, int size) {

//#define DEBUG_ENTROPY
#ifdef DEBUG_ENTROPY
	double e = 0;
#endif
	probabilities.clear();

	std::vector<int> probs(256, 0);
	for(int i = 0; i < size; i++)
		probs[data[i]]++;
//	int max = 0;
	for(size_t i = 0; i < probs.size(); i++) {
		if(probs[i] > 0) {
//			max = i;
			//TODO scaling probabilities to MAX would result in better accuracy (compression)?


#ifdef DEBUG_ENTROPY
			double p = probs[i]/(double)size;
			e -= p * log2(p);
			cout << (char)(i + 65) << " P: " << p << endl;
#endif
			probabilities.push_back(Symbol(i, probs[i]*255/size));
		}
	}

	std::sort(probabilities.begin(), probabilities.end(),
			  [](const Symbol &a, const Symbol &b)->bool { return a.probability > b.probability; });
#ifdef DEBUG_ENTROPY
	cout << "Entropy: " << e << " theorical compression: " << e*size/8 << endl;
#endif
}

void Tunstall::setProbabilities(float *probs, int n_symbols) {
	probabilities.clear();
	for(int i = 0; i < n_symbols; i++) {
		if(probs[i] <= 0) continue;
		probabilities.push_back(Symbol(i, probs[i]*255));
	}
}

void Tunstall::createDecodingTables2() {
	uint32_t n_symbols = probabilities.size();
	if(n_symbols <= 1) return;

	uint32_t dictionary_size = 1<<wordsize;
	//std::vector<TSymbol> queues(2*dictionary_size);
	vector<uint32_t> queues(2*dictionary_size);
	index.resize(2*dictionary_size);
	lengths.resize(2*dictionary_size);

	size_t end = 0;
	vector<unsigned char> &buffer = table;
	assert(wordsize == 8);
	buffer.resize(8192);
	uint32_t pos = 0;    //keep track of buffer lenght/
	vector<uint32_t> starts(n_symbols);

	uint32_t n_words = 0;

	uint32_t count = 2;
	uint32_t p0 = probabilities[0].probability<<8;
	uint32_t p1 = probabilities[1].probability<<8;
	uint32_t prob = (p0*p0)>>16;
	uint32_t max_count = (dictionary_size - 1)/(n_symbols - 1);
	while(prob > p1 && count < max_count) {
		prob = (prob*p0) >> 16;
		count++;
	}

	if(count >= 16) { //Very low entropy results in large tables > 8K.
		//  AAAA...A first word
		//  AAAA...B all shorter A...B can be compacted on the last one.
		//  AAAA...C and all shorter A .. C

		buffer[pos++] = probabilities[0].symbol;
		for(uint32_t k = 1; k < n_symbols; k++) {
			for(uint32_t i = 0; i < count-1; i++)
				buffer[pos++] = probabilities[0].symbol;
			buffer[pos++] = probabilities[k].symbol;
		}
		starts[0] = (count-1)*n_symbols;
		for(uint32_t k = 1; k < n_symbols; k++)
			starts[k] = k;

		for(uint32_t col = 0; col < count; col++) {
			for(uint32_t row = 1; row < n_symbols; row++) {
				uint32_t dest = row + col*n_symbols;
				uint32_t &probability = queues[dest];
				if(col == 0)
					probability = (probabilities[row].probability<<8);
				else
					probability = (prob * (probabilities[row].probability<<8)) >> 16;
				index[dest] = row*count - col;
				lengths[dest] = col+1;
			}
			if(col == 0)
				prob = p0;
			else
				prob = (prob*p0) >> 16;
		}

		uint32_t first = (count-1)*n_symbols;
		//TSymbol &first = queues[];
		queues[first] = prob;
		index[first] = 0;
		lengths[first] = count;
		n_words = 1 + count*(n_symbols - 1);
		end = count*n_symbols;
		assert(n_words == pos);

	} else {
		n_words = n_symbols;
		//initialize adding all symbols to queues
		for(uint32_t i = 0; i < n_symbols; i++) {
			starts[i] = i;
			queues[end] = (uint32_t)(probabilities[i].probability<<8);
			index[end] = pos;
			lengths[end++] = 1;
			buffer[pos++] = probabilities[i].symbol;
		}
	}

	while(n_words < dictionary_size) {
		//find highest probability word
		uint32_t best = 0;
		uint32_t max_prob = 0;
		for(uint32_t i = 0; i < n_symbols; i++) {
			uint32_t p = queues[starts[i]];
			if(p > max_prob) {
				best = i;
				max_prob = p;
			}
		}
		uint32_t symbol = starts[best];
		uint32_t probability = queues[symbol];
		uint32_t offset = index[symbol];
		uint32_t length = lengths[symbol];
		uint32_t r = 0;
		for(; r < n_symbols; r++) {
			uint32_t p = probabilities[r].probability;
			queues[end] = ( (probability * (unsigned int)(p<<8) )>>16);
			index[end] = pos;
			lengths[end++] = length + 1;

			assert(pos + length < buffer.size());
			memcpy(&buffer[pos], &buffer[offset], length);
			pos += length;
			buffer[pos++] = probabilities[r].symbol;

			if(n_words + r == dictionary_size -1)
				break;
		}
		if(r == n_symbols)
			starts[best] += n_symbols; //remove word only if all other rows expanded.
		n_words += n_symbols -1;
	}

	//compact index and lengths
	size_t word = 0;
	for(size_t i = 0, row = 0; i < end; i++, row++) {
		if(row >= n_symbols)
			row = 0;
		if(starts[row] > i)
			continue;

		index[word] = index[i];
		lengths[word] = lengths[i];
		word++;
	}
	index.resize(dictionary_size);
	lengths.resize(dictionary_size);
}

void Tunstall::createDecodingTables() {
	int n_symbols = probabilities.size();
	if(n_symbols <= 1) return;

	vector<deque<TSymbol> > queues(n_symbols);
	vector<unsigned char> buffer;

	//initialize adding all symbols to queues
	for(int i = 0; i < n_symbols; i++) {
		TSymbol s;
		s.probability = (uint32_t)(probabilities[i].probability<<8);
		s.offset = buffer.size();
		s.length = 1;

		queues[i].push_back(s);
		buffer.push_back(probabilities[i].symbol);
	}
	int dictionary_size = 1<<wordsize;
	int n_words = n_symbols;
	int table_length = n_symbols;
	while(n_words < dictionary_size - n_symbols +1) {
		//find highest probability word
		int best = 0;
		float max_prob = 0;
		for(int i = 0; i < n_symbols; i++) {
			float p = queues[i].front().probability ;
			if(p > max_prob) {
				best = i;
				max_prob = p;
			}
		}

		TSymbol symbol = queues[best].front();
		//split word.
		int pos = buffer.size();
		buffer.resize(pos + n_symbols*(symbol.length + 1));
		for(int i = 0; i < n_symbols; i++) {
			uint32_t p = probabilities[i].probability;
			TSymbol s;
			//TODO check performances with +1 and without.
			s.probability = ( ( symbol.probability * (unsigned int)(p<<8) )>>16); //probabilities[i].probability*symbol.probability;
			s.offset = pos;
			s.length = symbol.length + 1;

			memcpy(&buffer[pos], &buffer[symbol.offset], symbol.length);
			pos += symbol.length;
			buffer[pos++] = probabilities[i].symbol;
			queues[i].push_back(s);
		}
		table_length += (n_symbols-1)*(symbol.length + 1) +1;
		n_words += n_symbols -1;
		queues[best].pop_front();
	}
	index.clear();
	lengths.clear();
	table.clear();

	//build table and index
	index.resize(n_words);
	lengths.resize(n_words);
	table.resize(table_length);
	int word = 0;
	int pos = 0;
	for(size_t i = 0; i < queues.size(); i++) {
		deque<TSymbol> &queue = queues[i];
		for(size_t k = 0; k < queue.size(); k++) {
			TSymbol &s = queue[k];
			index[word] = pos;
			lengths[word] = s.length;
			word++;
			memcpy(&table[pos], &buffer[s.offset], s.length);
			pos += s.length;
		}
	}
	assert((int)index.size() <= dictionary_size);
}

void Tunstall::createEncodingTables() {

	int n_symbols = probabilities.size();
	if(n_symbols <= 1) return; //not much to compress
	//we need to reverse the table and index
	int lookup_table_size = 1;
	for(int i = 0; i < lookup_size; i++)
		lookup_table_size *= n_symbols;

	remap.resize(256, 0);
	for(int i = 0; i < n_symbols; i++) {
		Symbol &s = probabilities[i];
		remap[s.symbol] = i;
	}

	if(probabilities[0].probability > rle_limit) return;


	offsets.clear();
	offsets.resize(lookup_table_size, 0xffffff); //this is enough for quite large tables.
	for(size_t i = 0; i < index.size(); i++) {
		int low, high;
		int offset = 0; //keeps track of how far we are in the word.
		int table_offset = 0;
		while(1) {
			wordCode(&table[index[i] + offset], lengths[i] - offset, low, high);
			if(lengths[i] - offset <= lookup_size) {
				for(int k = low; k < high; k++) {
					offsets[table_offset + k] = i;
				}
				break;
			}

			//word is too long
			//check if some other word already did this:
			int w = offsets[table_offset + low];
			//if w = 0xfffff (another incomplete word with same starting was found
			// if w >= 0 (another complete word) we extend the table still pointing to w,
			//    but will be overwritten by longer words.
			if(w >= 0) { //add
				offsets[table_offset + low] = -offsets.size();
				offsets.resize(offsets.size() + lookup_table_size, w);
			}
			table_offset = -offsets[table_offset + low];
			offset += lookup_size;
		}
	}
}

unsigned char *Tunstall::compress(unsigned char *data, int input_size, int &output_size) {
	if(probabilities.size() == 1) {
		output_size = 0;
		return NULL;
	}

	unsigned char *output = new unsigned char[input_size*2]; //use entropy here!

	assert(wordsize <= 16);
	output_size = 0;
	int input_offset = 0;
	int word_offset = 0;
	int offset = 0;
	while(input_offset < input_size) {
		//read lookup_size symbols and compute code:
		int d = input_size - input_offset;
		if(d > lookup_size)
			d = lookup_size;
		int low, high;
		wordCode(data + input_offset, d, low, high);
		offset = offsets[-offset + low];
		assert(offset != 0xffffff);
		if(offset >= 0) { //ready to ouput a symbol
			output[output_size++] = offset;
			input_offset += lengths[offset] - word_offset;
			offset = 0;
			word_offset = 0;
		} else {
			word_offset += lookup_size;
			input_offset += lookup_size;
		}
	}
	//end of stream can be tricky:
	//we could have a partial read (need to encode half a word)
	if(offset < 0) {
		while(offset < 0)
			offset = offsets[-offset];
		output[output_size++] = offset;
	}
	assert(output_size <= input_size*2);
#ifdef DEBUG_ENTROPY
	cout << "Compressed to: E: " << ((float)output_size*8.0f)/input_size << " tot: " << output_size << endl;
#endif
	return output;
}

void Tunstall::decompress(unsigned char *data, int input_size, unsigned char *output, int output_size) {
	unsigned char *end_output = output + output_size;
	unsigned char *end_data = data + input_size -1;
	if(probabilities.size() == 1) {
		memset(output, probabilities[0].symbol, output_size);
		return;
	}
	//index.push_back(index.back() + lengths.back()); //TODO WHY?
	while(data < end_data) {
		int symbol = *data++;
		int start = index[symbol];
		int length = lengths[symbol];
		//int length = index[symbol+1] - start;
		memcpy(output, &table[start], length);
		output += length;
	}

	//last symbol might override so we check.
	int symbol = *data;
	int start = index[symbol];
	int length = (end_output - output);
	memcpy(output, &table[start], length);
}

int Tunstall::decompress(unsigned char *data, unsigned char *output, int output_size) {
	unsigned char *end_output = output + output_size;
	unsigned char *start = data;
	if(probabilities.size() == 1) {
		memset(output, probabilities[0].symbol, output_size);
		return 0;
	}
	exit(0);
	while(1) {
		int symbol = *data++;
		assert(symbol < (int)index.size());
		int start = index[symbol];
		int length = lengths[symbol];
		if(output + length >= end_output) {
			length = (end_output - output);
			memcpy(output, &table[start], length);
			break;
		} else {
			memcpy(output, &table[start], length);
			output += length;
		}
	}
	return data - start;
}


float Tunstall::entropy() {
	float e = 0;
	for(size_t i = 0; i < probabilities.size(); i++) {
		float p = probabilities[i].probability/255.0f;
		e += p*log(p)/log(2);
	}
	return -e;
}

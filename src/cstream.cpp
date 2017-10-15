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

#include "cstream.h"

#include "tunstall.h"
#ifdef ENTROPY_TESTS
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"
#include <zlib.h>
#endif

using namespace crt;
using namespace std;

int crt::ilog2(uint64_t p) {
	int k = 0;
	while ( p>>=1 ) { ++k; }
	return k;
}

namespace crt {


}

//TODO uniform notation length first, pointer after everywhere
int OutStream::compress(uint32_t size, uchar *data) {
	switch(entropy) {
	case NONE:
		write<uint32_t>(size);
		writeArray<uchar>(size, data);
		return sizeof(size) + size;

	case TUNSTALL: return tunstall_compress(data, size);
#ifdef ENTROPY_TESTS
	case ZLIB:     return zlib_compress(data, size);
	case LZ4:     return lz4_compress(data, size);
#endif
	default:
		throw "Unknown entropy";
	}
	return -1;
}

//TODO uniform notation length first, pointer after
void InStream::decompress(vector<uchar> &data) {
	switch(entropy) {
	case NONE: {
		uint32_t size = readUint32();
		data.resize(size);
		uchar *c = readArray<uchar>(size);
		memcpy(data.data(), c, size);
		break;
	}
	case TUNSTALL: tunstall_decompress(data); break;
#ifdef ENTROPY_TESTS
	case ZLIB:     zlib_decompress(data); break;
	case LZ4:     lz4_decompress(data); break;
#endif
	default:
		throw "Unknown entropy";
	}
}

int OutStream::tunstall_compress(uchar *data, int size) {
	Tunstall t;
	t.getProbabilities(data, size);

	t.createDecodingTables2();
	t.createEncodingTables();

	int compressed_size;
	unsigned char *compressed_data = t.compress(data, size, compressed_size);

	write<uchar>(t.probabilities.size());
	writeArray<uchar>(t.probabilities.size()*2, (uchar *)t.probabilities.data());


	write<int>(size);
	write<int>(compressed_size);
	writeArray<unsigned char>(compressed_size, compressed_data);
	delete []compressed_data;
	//return compressed_size;
	return 1 + t.probabilities.size()*2 + 4 + 4 + compressed_size;
}

void InStream::tunstall_decompress(vector<uchar> &data) {
	Tunstall t;
	int nsymbols = readUint8();
	uchar *probs = readArray<uchar>(nsymbols*2);
	t.probabilities.resize(nsymbols);
	memcpy(t.probabilities.data(), probs, nsymbols*2);

	t.createDecodingTables2();


	int size = readUint32();
	data.resize(size);
	int compressed_size = readUint32();
	unsigned char *compressed_data = readArray<unsigned char>(compressed_size);

	if(size)
		t.decompress(compressed_data, compressed_size, data.data(), size);
}

#ifdef ENTROPY_TESTS

int OutStream::zlib_compress(uchar *data, int size) {

	Bytef *compressed_data = new Bytef[compressBound(size)];
	uLongf compressed_size;
	compress2(compressed_data, &compressed_size, (const Bytef *)data, size, 9);

	write<int>(size);
	write<int>(compressed_size);
	writeArray<unsigned char>(compressed_size, compressed_data);
	delete []compressed_data;
	//return compressed_size;
	return 4 + 4 + compressed_size;
}

void InStream::zlib_decompress(vector<uchar> &data) {
	uLongf size = read<int>();
	data.resize(size);
	uLong compressed_size = read<int>();
	if(!size) return;

	unsigned char *compressed_data = readArray<unsigned char>(compressed_size);
	uncompress((Bytef *)data.data(), &size, (Bytef *)compressed_data, compressed_size);
}

int OutStream::lz4_compress(uchar *data, int size) {

	int compressed_size = LZ4_compressBound(size);
	uchar *compressed_data = new uchar[compressed_size];
	compressed_size = LZ4_compress_HC((const char *)data, (char *)compressed_data, size, compressed_size, 9);

	write<int>(size);
	write<int>(compressed_size);
	writeArray<uchar>(compressed_size, compressed_data);
	delete []compressed_data;
	//return compressed_size;
	return 4 + 4 + compressed_size;
}

void InStream::lz4_decompress(vector<uchar> &data) {
	int size = read<int>();
	data.resize(size);
	int compressed_size = read<int>();
	if(!size) return;

	uchar *compressed_data = readArray<uchar>(compressed_size);
	LZ4_decompress_safe((const char *)compressed_data, (char *)data.data(), compressed_size, size);
}
#endif


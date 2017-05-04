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

#ifndef CRT_CSTREAM_H
#define CRT_CSTREAM_H

#include <string.h>
#include <iostream>

#include "bitstream.h"

typedef unsigned char uchar;

using namespace std;

namespace crt {

int ilog2(uint64_t p);

class Stream {
protected:

	uchar *buffer;
	uchar *pos; //for reading.
	int allocated;
	int stopwatch; //used to measure stream partial size.

public:
	enum Entropy { NONE = 0, TUNSTALL = 1, HUFFMAN = 2, ZLIB = 3, LZ4 = 4 };
	Entropy entropy;

	Stream(): buffer(NULL), pos(NULL), allocated(0), stopwatch(0), entropy(TUNSTALL) {}

	Stream(int _size, uchar *_buffer) {
		init(_size, _buffer);
	}

	~Stream() {
		if(allocated)
			delete []buffer;
	}

	int size() { return pos - buffer; }
	uchar *data() { return buffer; }
	void restart() { stopwatch = size(); }
	int elapsed() {
		int e = size() - stopwatch; stopwatch = size();
		return e;
	}

	int  compress(uint32_t size, uchar *data);
	void decompress(std::vector<uchar> &data);
	int  tunstall_compress(unsigned char *data, int size);
	void tunstall_decompress(std::vector<uchar> &data);

#ifdef ENTROPY_TESTS
	int  zlib_compress(uchar *data, int size);
	void zlib_decompress(std::vector<uchar> &data);
	int  lz4_compress(uchar *data, int size);
	void lz4_decompress(std::vector<uchar> &data);
#endif

	void reserve(int reserved) {
		allocated = reserved;
		stopwatch = 0;
		pos = buffer = new uchar[allocated];
	}

	void init(int /*_size*/, uchar *_buffer) {
		buffer = _buffer;
		pos = buffer;
		allocated = 0;
	}

	void rewind() { pos = buffer; }

	template<class T> void write(T c) {
		grow(sizeof(T));
		*(T *)pos = c;
		pos += sizeof(T);
	}

	template<class T> void writeArray(int s, T *c) {
		int bytes = s*sizeof(T);
		push(c, bytes);
	}

	void writeString(const char *str) {
		int bytes = strlen(str)+1;
		write<uint16_t>(bytes);
		push(str, bytes);
	}

	void write(BitStream &stream) {
		stream.flush();
		//padding to 32 bit is needed for javascript reading (which uses int words.), mem needs to be aligned.
		write<int>((int)stream.size);

		int pad = (pos - buffer) & 0x3;
		if(pad != 0)
			pad = 4 - pad;
		grow(pad);
		pos += pad;
		push(stream.buffer, stream.size*sizeof(uint32_t));
	}


	template<class T> T read() {
		T c;
		c = *(T *)pos;
		pos += sizeof(T);
		return c;
	}

	template<class T> T *readArray(int s) {
		int bytes = s*sizeof(T);
		T *buffer = (T *)pos;
		pos += bytes;
		return buffer;
	}

	char *readString() {
		int bytes = read<uint16_t>();
		return readArray<char>(bytes);
	}


	void read(BitStream &stream) {
		int s = read<int>();
		//padding to 32 bit is needed for javascript reading (which uses int words.), mem needs to be aligned.
		int pad = (pos - buffer) & 0x3;
		if(pad != 0)
			pos += 4 - pad;
		stream.init(s, (uint32_t *)pos);
		pos += s*sizeof(uint32_t);
	}

	void grow(int s) {
		if(allocated == 0)
			reserve(1024);
		int size = pos - buffer;
		if(size + s > allocated) { //needs more spac
			int new_size = allocated*2;
			while(new_size < size + s)
				new_size *= 2;
			uchar *b = new uchar[new_size];
			memcpy(b, buffer, allocated);
			delete []buffer;
			buffer = b;
			pos = buffer + size;
			allocated = new_size;
		}
	}

	void push(const void *b, int s) {
		grow(s);
		memcpy(pos, b, s);
		pos += s;
	}


	static int needed(int a) {
		if(a == 0) return 0;
		if(a == -1) return 1;
		if(a < 0) a = -a - 1;
		int n = 2;
		while(a >>= 1) n++;
		return n;
	}



	template <class T> void encodeValues(uint32_t size, T *values, int N) {
		BitStream bitstream(size);
		//Storing bitstream before logs, allows in decompression to allocate only 1 logs array and reuse it.
		std::vector<std::vector<uchar> > clogs((size_t)N);

		for(int c = 0; c < N; c++) {
			auto &logs = clogs[c];
			logs.resize(size);
			for(uint32_t i = 0; i < size; i++) {
				int val = values[i*N + c];

				if(val == 0) {
					logs[i] = 0;
					continue;
				}
				int ret = ilog2(abs(val)) + 1;  //0 -> 0, [1,-1] -> 1 [-2,-3,2,3] -> 2
				logs[i] = ret;
				int middle = (1<<ret)>>1;
				if(val < 0) val = -val -middle;
				bitstream.write(val, ret);
			}
		}

		write(bitstream);
		for(int c = 0; c < N; c++)
			compress(clogs[c].size(), &*clogs[c].begin());
	}

	template <class T> int decodeValues(T *values, int N) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;

		for(int c = 0; c < N; c++) {
			decompress(logs);
			for(uint32_t i = 0; i < logs.size(); i++) {
				uchar &diff = logs[i];
				if(diff == 0) {
					values[i*N + c] = 0;
					continue;
				}

				int val = bitstream.read(diff);
				int middle = 1<<(diff-1);
				if(val < middle)
					val = -val -middle;
				values[i*N + c] = val;
			}
		}
		return logs.size();
	}


	template <class T> void encodeArray(uint32_t size, T *values, int N) {
		BitStream bitstream(size);
		std::vector<uchar> logs(size);

		for(uint32_t i = 0; i < size; i++) {
			T *p = values + i*N;
			int diff = needed(p[0]);
			for(int c = 1; c < N; c++) {
				int d = needed(p[c]);
				if(diff < d) diff = d;
			}
			logs[i] = diff;
			if(diff == 0) continue;

			int max = 1<<(diff-1);
			for(int c = 0; c < N; c++)
				bitstream.write(p[c] + max, diff);
		}

		write(bitstream);
		compress(logs.size(), &*logs.begin());
	}

	template <class T> int decodeArray(T *values, int N) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;
		decompress(logs);

		for(uint32_t i =0; i < logs.size(); i++) {
			T *p = values + i*N;
			uchar &diff = logs[i];
			if(diff == 0) {
				for(int c = 0; c < N; c++)
					p[c] = 0;
				continue;
			}
			//making a single read is 2/3 faster
			//uint64_t &max = bmax[diff];
			const uint64_t max = (1<<diff)>>1;
			if(0 && diff < 22) {
				//uint64_t &mask = bmask[diff]; //using table is 4% faster
				const uint64_t mask = (1<<diff)-1;
				uint64_t bits = bitstream.read(N*diff);
				for(uint32_t c = N-1; c > 0; c--) {
					p[c] = (bits & mask) - max;
					bits >>= diff;
				}
				p[0] = bits - max;
			} else {
				for(int c = 0; c < N; c++)
					p[c] = bitstream.read(diff) - max;
			}
		}
		return logs.size();
	}
};

} //namespace
#endif // CRT_CSTREAM_H

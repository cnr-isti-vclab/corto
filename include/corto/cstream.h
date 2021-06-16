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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "bitstream.h"

typedef unsigned char uchar;

using namespace std;

namespace crt {

int ilog2(uint64_t p);

class Stream {
public:
	enum Entropy { NONE = 0, TUNSTALL = 1, HUFFMAN = 2, ZLIB = 3, LZ4 = 4 };
	Entropy entropy;
	Stream(): entropy(TUNSTALL) {}
};

class OutStream: public Stream {
protected:
	std::vector<uchar> buffer;
	size_t stopwatch; //used to measure stream partial size.

public:
	OutStream(size_t r = 0): stopwatch(0) { buffer.reserve(r); }
	uint32_t size() { return buffer.size(); }
	uchar *data() { return buffer.data(); }
	void reserve(size_t r) { buffer.reserve(r); }
	void restart() { stopwatch = buffer.size(); }
	uint32_t elapsed() {
		size_t e = size() - stopwatch; stopwatch = size();
		return (uint32_t)e;
	}
	int  compress(uint32_t size, uchar *data);
	int  tunstall_compress(unsigned char *data, int size);

#ifdef ENTROPY_TESTS
	int  zlib_compress(uchar *data, int size);
	int  lz4_compress(uchar *data, int size);
#endif

	template<class T> void write(T c) {
		uchar *pos = grow(sizeof(T));
		*(T *)pos = c;
	}
	template<class T> void writeArray(int count, T *c) {
		push(c, count*sizeof(T));
	}

	void writeString(const char *str) {
		uint16_t bytes = (uint16_t)(strlen(str)+1);
		write<uint16_t>(bytes);
		push(str, bytes);
	}

	void write(BitStream &stream) {
		stream.flush();
		write<int>((int)stream.size);

		//padding to 32 bit is needed for javascript reading (which uses int words.), mem needs to be aligned.
		int pad = size() & 0x3;
		if(pad != 0)
			pad = 4 - pad;
		grow(pad);
		push(stream.buffer, stream.size*sizeof(uint32_t));
	}

	uchar *grow(size_t s) {
		size_t len = buffer.size();
		buffer.resize(len + s);
		//padding to 32 bit is needed for javascript reading (which uses int words.), mem needs to be aligned.
		assert((((uintptr_t)buffer.data()) & 0x3) == 0);
		return buffer.data() + len;
	}

	void push(const void *b, size_t s) {
		uchar *pos = grow(s);
		memcpy(pos, b, s);
	}


	static int needed(int a) {
		if(a == 0) return 0;
		if(a == -1) return 1;
		if(a < 0) a = -a - 1;
		int n = 2;
		while(a >>= 1) n++;
		return n;
	}

	//encode differences of vectors (assuming no correlation between components)
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
				logs[i] = (uchar)ret;
				int middle = (1<<ret)>>1;
				if(val < 0) val = -val -middle;
				bitstream.write(val, ret);
			}
		}

		write(bitstream);
		for(int c = 0; c < N; c++)
			compress((uint32_t)clogs[c].size(), clogs[c].data());
	}
	//encode differences of vectors (assuming correlation between components)
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
		compress(logs.size(), logs.data());
	}
	
	//encode DIFFS
	template <class T> void encodeDiffs(uint32_t size, T *values) {
		BitStream bitstream(size);
		std::vector<uchar> logs(size);
		for(uint32_t i = 0; i < size; i++) {
			T val = values[i];
			if(val == 0) {
				logs[i] = 0;
				continue;
			}
			int ret = ilog2(abs(val)) + 1;  //0 -> 0, [1,-1] -> 1 [-2,-3,2,3] -> 2
			logs[i] = (uchar)ret;
			
			int middle = (1<<ret)>>1;
			if(val < 0) val = -val -middle;
			bitstream.write(val, ret);
		}
		write(bitstream);
		compress(logs.size(), logs.data());
	}
	
	//encode POSITIVE values
	template <class T> void encodeIndices(uint32_t size, T *values) {
		BitStream bitstream(size);
		std::vector<uchar> logs(size);
		for(uint32_t i = 0; i < size; i++) {
			T val = values[i] + 1;
			if(val == 1) {
				logs[i] = 0;
				continue;
			}
			
			int ret = logs[i] = ilog2(val);
			bitstream.write(val -(1<<ret), ret);
		}
		write(bitstream);
		compress(logs.size(), logs.data());
	}
};

class InStream: public Stream {
protected:
	const uchar *buffer;
	const uchar *pos; //for reading.

public:

	InStream(): buffer(NULL), pos(NULL) {}
	InStream(int _size, uchar *_buffer) {
		init(_size, _buffer);
	}

	void decompress(std::vector<uchar> &data);
	void tunstall_decompress(std::vector<uchar> &data);

#ifdef ENTROPY_TESTS
	int  zlib_compress(uchar *data, int size);
	int  lz4_compress(uchar *data, int size);
#endif

	void init(int /*_size*/, const uchar *_buffer) {
		buffer = _buffer; //I'm not lying, I won't touch it.
		pos = buffer;
	}

	void rewind() { pos = buffer; }

/*	template<class T> T read() {
		T c;
		c = *(T *)pos;
		pos += sizeof(T);
		return c;
	} */

	template<class T> T *readArray(uint32_t s) {
		T *buffer = (T *)pos;
		pos += s*sizeof(T);
		return buffer;
	}

	uint8_t readUint8() {
		return *pos++;
	}

	uint16_t readUint16() {
		uint16_t c;
		c = pos[1];
		c<<=8;
		c += pos[0];
		pos += 2;
		return c;
	}

	uint32_t readUint32() {
		uint32_t c;
		c = pos[3];
		c<<=8;
		c += pos[2];
		c<<=8;
		c += pos[1];
		c<<=8;
		c += pos[0];
		pos += 4;
		return c;
	}

	float readFloat() {
		uint32_t c = readUint32();
		return *(float *)&c;
	}

	char *readString() {
		uint16_t bytes = readUint16();
		return readArray<char>(bytes);
	}


	void read(BitStream &stream) {
		int s = readUint32();
		//padding to 32 bit is needed for javascript reading (which uses int words.), mem needs to be aligned.
		int pad = (pos - buffer) & 0x3;
		if(pad != 0)
			pos += 4 - pad;
		stream.init(s, (uint32_t *)pos);
		pos += s*sizeof(uint32_t);
	}


	template <class T> int decodeValues(T *values, int N) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;

		for(int c = 0; c < N; c++) {
			decompress(logs);
			if(!values) continue;

			for(uint32_t i = 0; i < logs.size(); i++) {
				uchar &diff = logs[i];
				if(diff == 0) {
					values[i*N + c] = 0;
					continue;
				}

				int val = (int)bitstream.read(diff);
				int middle = 1<<(diff-1);
				if(val < middle)
					val = -val -middle;
				values[i*N + c] = (T)val;
			}
		}
		return logs.size();
	}




	template <class T> uint32_t decodeArray(T *values, int N) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;
		decompress(logs);

		if(!values) //just skip and return number of readed
			return (uint32_t)logs.size();

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
					p[c] = (T)((bits & mask) - max);
					bits >>= diff;
				}
				p[0] = (T)(bits - max);
			} else {
				for(int c = 0; c < N; c++)
					p[c] = (T)(bitstream.read(diff) - max);
			}
		}
		return (uint32_t)logs.size();
	}
	
	//decode POSITIVE values
	template <class T> uint32_t decodeIndices(T *values) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;
		decompress(logs);
		
		if(!values)
			return (uint32_t)logs.size();
		
		for(uint32_t i =0; i < logs.size(); i++) {
			T &p = values[i];
			uchar &ret = logs[i];
			if(ret == 0) {
				p = 0;
				continue;
			}
			p = (1<<ret) + (T)bitstream.read(ret) -1;
		}
		return (uint32_t)logs.size();
	}
	
	//decode DIFFERENCES
	template <class T> uint32_t decodeDiffs(T *values) {
		BitStream bitstream;
		read(bitstream);

		std::vector<uchar> logs;
		decompress(logs);
		
		if(!values)
			return (uint32_t)logs.size();
		
		for(uint32_t i =0; i < logs.size(); i++) {
			
			uchar &diff = logs[i];
			if(diff == 0) {
				values[i] = 0;
				continue;
			}

/*
 *  Is this faster (and correct)?
			int max = (1<<diff)>>1;
			values[i] = (int)bitstream.read(diff) - ((1<<(diff)>>1);
*/
					
			int val = (int)bitstream.read(diff);
			int middle = 1<<(diff-1);
			if(val < middle)
				val = -val -middle;
			values[i] = (T)val; 
		}
		return (uint32_t)logs.size();
	}
};

} //namespace
#endif // CRT_CSTREAM_H

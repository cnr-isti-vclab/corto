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

#ifndef CRT_BITSTREAM_H
#define CRT_BITSTREAM_H

#include <vector>
#include <stdint.h>

namespace crt {

class BitStream {
public:
	BitStream(): size(0), buffer(0), allocated(0), pos(0), buff(0), bits(0) {}
	BitStream(int reserved); // in uint32_t units
	BitStream(int size, uint32_t *buffer); //for reading


	~BitStream();
	void init(int size, uint32_t *buffer); //in uint32_t units for reading
	void reserve(int size); //for writing

	void write(uint32_t bits, int n);
	uint32_t read(int numbits); //faster
	uint32_t writtenBits();


	void flush();
	int size; //in uint32
	uint32_t *buffer;
private:
	void push_back(uint32_t w);
	int allocated;

	uint32_t *pos;
	uint32_t buff;
	int bits;
};

}//namespace
#endif // CRT_BITSTREAM_H

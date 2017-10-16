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

#include <string.h>
#include <assert.h>
#include "bitstream.h"

// allows a specific bit to be masked from a number
#include <iostream>
using namespace std;
using namespace crt;
// usage: bmask[k] has the rightmost k bits == 1, other bits 0
//
static uint32_t bmask[] = {
	0x00,     0x01,       0x03,        0x07,        0x0f,       0x01f,       0x03f,       0x07f,
	0xff,     0x01ff,     0x03ff,      0x07ff,      0x0fff,     0x01fff,     0x03fff,     0x07fff,
	0xffff,   0x01ffff,   0x03ffff,    0x07ffff,    0x0fffff,   0x01fffff,   0x03fffff,   0x07fffff,
        0xffffff, 0x01ffffff, 0x03ffffff,  0x07ffffff,  0x0fffffff, 0x01fffffff, 0x03fffffff, 0x7fffffff };

/*	0xffffffff,       0x01ffffffff,       0x03ffffffff,        0x07ffffffff,        0x0fffffffff,       0x01fffffffff,       0x03fffffffff,       0x07fffffffff,
	0xffffffffff,     0x01ffffffffff,     0x03ffffffffff,      0x07ffffffffff,      0x0fffffffffff,     0x01fffffffffff,     0x03fffffffffff,     0x07fffffffffff,
	0xffffffffffff,   0x01ffffffffffff,   0x03ffffffffffff,    0x07ffffffffffff,    0x0fffffffffffff,   0x01fffffffffffff,   0x03fffffffffffff,   0x07fffffffffffff,
	0xffffffffffffff, 0x01ffffffffffffff, 0x03ffffffffffffff,  0x07ffffffffffffff,  0x0fffffffffffffff, 0x01fffffffffffffff, 0x03fffffffffffffff, 0x07fffffffffffffff,

        0xffffffffffffffff }; */

#define BITS_PER_WORD 32


//TODO is it faster using bmask or using ~((1L<<d)-1)?

BitStream::BitStream(int reserved) { //for write
	reserve(reserved);


}

BitStream::BitStream(int _size, uint32_t *_buffer) { //for read
	init(_size, _buffer);

}

BitStream::~BitStream() {
	if(allocated)
		delete []buffer;
}

void BitStream::init(int _size, uint32_t *_buffer) { //in uint32_t units for reading
	buffer = _buffer;
	size = _size;
	allocated = 0;
	buff = 0;
	bits = 0;
	pos = buffer;
}

void BitStream::reserve(int reserved) { //in uint32_t units for reading
	allocated = reserved;
	pos = buffer = new uint32_t[allocated];
	size = 0;
	buff = 0;
	bits = BITS_PER_WORD;
	pos = buffer;
}

uint32_t BitStream::writtenBits() {
	return (size+1)*BITS_PER_WORD - bits;
}

//TODO change name but the bmask is useless
void BitStream::write(uint32_t value, int numbits) {
	if(!allocated) reserve(256);
	if (numbits >= bits) {
		buff = (buff << bits) | (value >> (numbits - bits));
		push_back(buff);
		value &= bmask[numbits - bits];
		numbits -= bits;
		bits = BITS_PER_WORD;
		buff = 0;
	}

	if (numbits > 0) {
		buff = (buff << numbits) | value;
		bits -= numbits;
	}
}

uint32_t BitStream::read(int numbits) {
	if(numbits > bits) {
		bits = numbits - bits;
		uint32_t result = (buff << bits); //looks the same.
		bits = 32 - bits;

		buff = *pos++;
		result |= (buff >> bits);
		buff = (buff & ((1<<bits)-1)); //slighting faster than mask.
		return result;

	} else {
		bits -= numbits;
		uint32_t result = (buff >> bits);
		//buff -= result << bits; //same as below.
		buff = (buff & ((1<<bits)-1)); //slighting faster than mask,
		return result;
	}
}


void BitStream::flush() {
	if (bits != BITS_PER_WORD) {
		push_back((buff << bits));
		buff = 0;
		bits = BITS_PER_WORD;
	}
}

void BitStream::push_back(uint32_t w) {
	if(size >= allocated) {
				uint32_t *b = new uint32_t[allocated*2];
				memcpy(b, buffer, allocated*sizeof(uint32_t));
		delete []buffer;
		buffer = b;
		allocated *= 2;
	}
	buffer[size++] = w;
}

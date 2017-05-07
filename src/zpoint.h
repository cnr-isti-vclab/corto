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

#ifndef CRT_ZPOINT_H
#define CRT_ZPOINT_H

#include <stdint.h>
#include "point.h"

namespace crt {

class ZPoint {
public:
	uint64_t bits; //enough for 21 bit quantization.
	uint32_t pos;

	ZPoint(uint64_t b = 0): bits(b), pos(-1) {}
	ZPoint(uint64_t x, uint64_t y, uint64_t z, int levels, int i): bits(0), pos(i) {
		uint64_t l = 1;
		for (int i = 0; i < levels; i++)
			bits |= (x & l << i)<< (2*i) | (y & l << i) << (2*i + 1) | (z & l << i) << (2*i+2);
	}

	uint64_t morton2(uint64_t x)
	{
		x = x & 0x55555555ull;
		x = (x | (x >> 1)) & 0x33333333ull;
		x = (x | (x >> 2)) & 0x0F0F0F0Full;
		x = (x | (x >> 4)) & 0x00FF00FFull;
		x = (x | (x >> 8)) & 0x0000FFFFull;
		return x;
	}

	uint64_t morton3(uint64_t x)
	{
		//1001 0010  0100 1001  0010 0100  1001 0010  0100 1001  0010 0100  1001 0010  0100 1001 => 249
		//1001001001001001001001001001001001001001001001001001001001001001
		//a  b  c  d  e  f  g  h  i  l  m  n  o  p  q  r  s  t  u  v  x  y
		// a  b  c  d  e  f  g  h  i  l  m  n  o  p  q  r  s  t  u  v  x
		//  a  b  c  d  e  f  g  h  i  l  m  n  o  p  q  r  s  t  u  v  x
		//
		//0011000011000011000011000011000011000011000011000011000011000011
		x = x & 0x9249249249249249ull;
		x = (x | (x >> 2))  & 0x30c30c30c30c30c3ull;
		x = (x | (x >> 4))  & 0xf00f00f00f00f00full;
		x = (x | (x >> 8))  & 0x00ff0000ff0000ffull;
		x = (x | (x >> 16)) & 0xffff00000000ffffull;
		x = (x | (x >> 32)) & 0x00000000ffffffffull;
		return x;
	}

	Point3f toPoint(const Point3i &min, float step) {
		int x = (int)morton3(bits);
		int y = (int)morton3(bits>>1);
		int z = (int)morton3(bits>>2);

		Point3f p(x + min[0], y + min[1], z + min[2]);
		p *= step;
		return p;
	}
	Point3f toPoint(float step) {
		uint32_t x = morton3(bits);
		uint32_t y = morton3(bits>>1);
		uint32_t z = morton3(bits>>2);

		return Point3f(x*step, y*step, z*step);
	}


#ifdef WIN32
#define ONE64 ((__int64)1)
#else
#define ONE64 1LLU
#endif
	void clearBit(int i) {
		bits &= ~(ONE64 << i);
	}
	void setBit(int i) {
		bits |= (ONE64 << i);
	}

	void setBit(int i, uint64_t val) {
		bits &= ~(ONE64 << i);
		bits |= (val << i);
	}
	bool testBit(int i) {
		return (bits & (ONE64<<i)) != 0;
	}
	bool operator==(const ZPoint &zp) const {
		return bits == zp.bits;
	}
	bool operator!=(const ZPoint &zp) const {
		return bits != zp.bits;
	}
	bool operator<(const ZPoint &zp) const {
		return bits > zp.bits;
	}
	int difference(const ZPoint &p) const {
		uint64_t diff = bits ^ p.bits;
		return log2(diff);
	}
	static int log2(uint64_t p) {
		int k = 0;
		while ( p>>=1 ) { ++k; }
		return k;
	}
};

}//namespace
#endif // CRT_ZPOINT_H

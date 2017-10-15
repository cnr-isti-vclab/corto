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

#ifndef CRT_COLOR_ATTRIBUTE_H
#define CRT_COLOR_ATTRIBUTE_H

#include "vertex_attribute.h"
#include "point.h"

namespace crt {

class ColorAttr: public GenericAttr<uchar> {
public:
	int qc[4];
	ColorAttr(): GenericAttr<uchar>(4) {
		qc[0] = qc[1] = qc[2] = 4;
		qc[3] = 8;
	}
	virtual int codec() { return COLOR_CODEC; }

	void setQ(int r_bits, int g_bits, int b_bits, int a_bits) {
		qc[0] = 1<<(8 - r_bits);
		qc[1] = 1<<(8 - g_bits);
		qc[2] = 1<<(8 - b_bits);
		qc[3] = 1<<(8 - a_bits);
	}

	virtual void quantize(uint32_t nvert, const char *buffer);
	virtual void dequantize(uint32_t nvert);

	virtual void encode(uint32_t nvert, OutStream &stream) {
		stream.restart();
		for(int c = 0; c < 4; c++)
			stream.write<uchar>((uchar)qc[c]);

		stream.encodeValues<char>(nvert, (char *)diffs.data(), N);
		size = stream.elapsed();
	}
	virtual void decode(uint32_t /*nvert*/, InStream &stream) {
		for(int c = 0; c < 4; c++)
			qc[c] = stream.readUint8();
		stream.decodeValues<uchar>((uchar *)buffer, N);
	}
};

} //namespace

#endif // CRT_COLOR_ATTRIBUTE_H

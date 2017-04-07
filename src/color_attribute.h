/*
Nexus

Copyright(C) 2012 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (http://www.gnu.org/licenses/gpl.txt)
for more details.
*/
#ifndef NX_COLOR_ATTRIBUTE_H
#define NX_COLOR_ATTRIBUTE_H

#include "vertex_attribute.h"
#include "point.h"

namespace nx {

class ColorAttr: public GenericAttr<uchar> {
public:
	int qc[4];
	ColorAttr(): GenericAttr<uchar>(4) {
		qc[0] = qc[1] = qc[2] = 4;
		qc[3] = 8;
	}
	virtual int codec() { return COLOR_CODEC; }

	void setQ(int lumabits, int chromabits, int alphabits) {
		qc[0] = 1<<(8 - lumabits);
		qc[1] = qc[2] = 1<<(8 - chromabits);
		qc[3] = 1<<(8-alphabits);
	}

        virtual void quantize(uint32_t nvert, char *buffer);
	virtual void dequantize(uint32_t nvert);

	virtual void encode(uint32_t nvert, Stream &stream) {
		stream.restart();
		for(int c = 0; c < 4; c++)
			stream.write<uchar>(qc[c]);

		stream.encodeValues<char>(nvert, (char *)&*diffs.begin(), N);
		size = stream.elapsed();
	}
	virtual void decode(uint32_t /*nvert*/, Stream &stream) {
		for(int c = 0; c < 4; c++)
			qc[c] = stream.read<uchar>();
		stream.decodeValues<uchar>((uchar *)buffer, N);
	}
};

} //namespace

#endif // NX_COLOR_ATTRIBUTE_H

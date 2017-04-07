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
#ifndef NX_INDEX_ATTRIBUTE_H
#define NX_INDEX_ATTRIBUTE_H

#include "cstream.h"

namespace nx {

enum Clers { VERTEX = 0, LEFT = 1, RIGHT = 2, END = 3, BOUNDARY = 4, DELAY = 5, SPLIT = 6};

struct Quad {
	uint32_t t, a, b, c;
	Quad() {}
	Quad(uint32_t _t, uint32_t v0, uint32_t v1, uint32_t v2): t(_t), a(v0), b(v1), c(v2) {}
};

struct Face {
	uint32_t a, b, c;
	Face() {}
	Face(uint32_t v0, uint32_t v1, uint32_t v2): a(v0), b(v1), c(v2) {}
};

class IndexAttribute {
public:
	uint32_t *faces32;
	uint16_t *faces16;
	std::vector<uint32_t> faces;
	std::vector<Face> prediction;

	std::vector<uint32_t> groups;
	std::vector<uchar> clers;
	BitStream bitstream;
	uint32_t max_front; //max size reached by front.
	uint32_t size;

	IndexAttribute(): faces32(nullptr), faces16(nullptr), max_front(0) {}
	void encode(Stream &stream) {
		stream.write<uint32_t>(max_front);
		stream.write<uint32_t>(groups.size());
		for(uint32_t &g: groups)
			stream.write<uint32_t>(g);

		stream.restart();
		stream.compress(clers.size(), &*clers.begin());
		stream.write(bitstream);
		size = stream.elapsed();

	}

	void decode(Stream &stream) {

		max_front = stream.read<uint32_t>();
		groups.resize(stream.read<uint32_t>());
		for(uint32_t &g: groups)
			g = stream.read<uint32_t>();

		stream.decompress(clers);
		stream.read(bitstream);
	}
};

} //namespace

#endif // NX_INDEX_ATTRIBUTE_H

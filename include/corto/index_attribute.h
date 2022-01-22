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

#ifndef CRT_INDEX_ATTRIBUTE_H
#define CRT_INDEX_ATTRIBUTE_H

#include "cstream.h"

namespace crt {

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

struct Group {
	uint32_t end; //1+ last face
	std::map<std::string, std::string> properties;

	Group() {}
	Group(uint32_t e): end(e) {}
};

class IndexAttribute {
public:
	uint32_t *faces32;
	uint16_t *faces16;
	std::vector<uint32_t> faces;
	std::vector<Face> prediction;

	std::vector<Group> groups;
	std::vector<uchar> clers;
	BitStream bitstream;
	uint32_t max_front; //max size reached by front.
	uint32_t size;

	IndexAttribute(): faces32(nullptr), faces16(nullptr), max_front(0) {}
	void encode(OutStream &stream) {
		stream.write<uint32_t>(max_front);

		stream.restart();
		stream.compress((int)clers.size(), clers.data());
		stream.write(bitstream);
		size = (uint32_t)stream.elapsed();
	}

	void encodeGroups(OutStream &stream) {
		stream.write<uint32_t>((uint32_t)groups.size());
		for(Group &g: groups) {
			stream.write<uint32_t>(g.end);
			stream.write<uchar>((uchar)g.properties.size());
			for(auto it: g.properties) {
				stream.writeString(it.first.c_str());
				stream.writeString(it.second.c_str());
			}
		}
	}

	void decode(InStream &stream) {
		max_front = stream.readUint32();
		stream.decompress(clers);
		stream.read(bitstream);
	}

	void decodeGroups(InStream &stream) {
		groups.resize(stream.readUint32());
		for(Group &g: groups) {
			g.end = stream.readUint32();
			uchar size = stream.readUint8();
			for(uint32_t i = 0; i < size; i++) {
				const char *key = stream.readString();
				g.properties[key] = stream.readString();
			}
		}
	}
};

} //namespace

#endif // CRT_INDEX_ATTRIBUTE_H

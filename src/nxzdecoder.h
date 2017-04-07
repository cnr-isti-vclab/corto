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
#ifndef NXZ_DECODER_H
#define NXZ_DECODER_H

#include <vector>
#include <map>
#include <algorithm>

#include "cstream.h"
#include "tunstall.h"
#include "zpoint.h"
#include "index_attribute.h"
#include "vertex_attribute.h"
#include "color_attribute.h"
#include "normal_attribute.h"

namespace nx {



class NxzDecoder {
public:
	uint32_t nvert, nface;

	std::map<std::string, VertexAttribute *> data;
	IndexAttribute index;

	NxzDecoder(int len, uchar *input);
	~NxzDecoder();

	bool hasAttr(const char *name) { return data.count(name); }

	bool setPositions(float *buffer) { return setAttribute("position", (char *)buffer, VertexAttribute::FLOAT); }
	bool setNormals(float *buffer)   { return setAttribute("normal", (char *)buffer, VertexAttribute::FLOAT); }
	bool setNormals(int16_t *buffer) { return setAttribute("normal", (char *)buffer, VertexAttribute::INT16); }
	bool setColors(uchar *buffer)    { return setAttribute("color", (char *)buffer, VertexAttribute::UINT8); }
	bool setUvs(float *buffer)       { return setAttribute("uv", (char *)buffer, VertexAttribute::FLOAT); }

	bool setAttribute(const char *name, char *buffer, VertexAttribute::Format format);
	bool setAttribute(const char *name, char *buffer, VertexAttribute *attr);

	void setIndex(uint32_t *buffer) { index.faces32 = buffer; }
	void setIndex(uint16_t *buffer) { index.faces16 = buffer; }

	void decode();

private:
	Stream stream;

	int vertex_count; //keep tracks of current decoding vertex

	void decodePointCloud();
	void decodeMesh();
	void decodeFaces(uint32_t start, uint32_t end, uint32_t &cler);
};


} //namespace
#endif // NXZ_DECODER_H

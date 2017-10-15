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

#include <assert.h>
#include "normal_attribute.h"

using namespace crt;

template <class T> void markBoundary(uint32_t nvert, uint32_t nface, T *index, std::vector<int32_t> &boundary) {
	boundary.clear();
	boundary.resize(nvert, 0);

	T *end = index + nface*3;
	for(T *f = index; f < end; f += 3) {
		boundary[f[0]] ^= (int)f[1];
		boundary[f[0]] ^= (int)f[2];
		boundary[f[1]] ^= (int)f[2];
		boundary[f[1]] ^= (int)f[0];
		boundary[f[2]] ^= (int)f[0];
		boundary[f[2]] ^= (int)f[1];
	}
}


template <class T> void estimateNormals(uint32_t nvert, Point3i *coords, uint32_t nface, T *index, std::vector<Point3f> &estimated) {
	estimated.clear();
	estimated.resize(nvert, Point3f(0, 0, 0));
	T *end = index + nface*3;
	for(T *f = index; f < end; f += 3) {
		Point3i &p0 = coords[f[0]]; //overflow!
		Point3i &p1 = coords[f[1]];
		Point3i &p2 = coords[f[2]];
		Point3f v0(p0[0], p0[1], p0[2]);
		Point3f v1(p1[0], p1[1], p1[2]);
		Point3f v2(p2[0], p2[1], p2[2]);
		Point3f n = (( v1 - v0) ^ (v2 - v0));
		estimated[f[0]] += n;
		estimated[f[1]] += n;
		estimated[f[2]] += n;
	}
}

void NormalAttr::quantize(uint32_t nvert, const char *buffer) {
	uint32_t n = 2*nvert;

	values.resize(n);
	diffs.resize(n);

	Point2i *normals = (Point2i *)values.data();
	switch(format) {
	case FLOAT:
		for(uint32_t i = 0; i < nvert; i++) {
			normals[i] = toOcta(((const Point3f *)buffer)[i], (int)q);
		}
		break;
	case INT32:
		for(uint32_t i = 0; i < nvert; i++)
			normals[i] =  toOcta(((const Point3i *)buffer)[i], (int)q);
		break;
	case INT16:
	{
		Point3<int16_t> *s = (Point3<int16_t> *)buffer;
		for(uint32_t i = 0; i < nvert; i++)
			normals[i] = toOcta(Point3i(s[i][0], s[i][1], s[i][2]), (int)q);
		break;
	}
	case INT8:
	{
		Point3<int8_t> *s = (Point3<int8_t> *)buffer;
		for(uint32_t i = 0; i < nvert; i++)
			normals[i] = toOcta(Point3i(s[i][0], s[i][1], s[i][2]), (int)q);
		break;
	}
	default: throw "Unsigned types not supported for normals";
	}
	Point2i min(values[0], values[1]);
	Point2i max(min);
	for(uint32_t i = 1; i < nvert; i++) {
		min.setMin(normals[i]);
		max.setMax(normals[i]);
	}
	max -= min;
	bits = std::max(ilog2(max[0]-1), ilog2(max[1]-1)) + 1;
}


void NormalAttr::preDelta(uint32_t nvert,  uint32_t nface, std::map<std::string, VertexAttribute *> &attrs, IndexAttribute &index) {
	if(prediction == DIFF)
		return;

	if(attrs.find("position") == attrs.end())
		throw "No position attribute found. Use DIFF normal strategy instead.";

	GenericAttr<int> *coord = dynamic_cast<GenericAttr<int> *>(attrs["position"]);
	if(!coord)
		throw "Position attr has been overloaded, Use DIFF normal strategy instead.";

	uint32_t *start = index.faces.data();
	//estimate normals using vertices and faces existing.
	std::vector<Point3f> estimated;
	estimateNormals<uint32_t>(nvert, (Point3i *)coord->values.data(), nface, start, estimated);

	if(prediction == BORDER)
		markBoundary<uint32_t>(nvert, nface, start, boundary); //mark boundary points on original vertices.

	Point2i *v= (Point2i *)values.data();
	for(uint32_t i = 0; i < nvert; i++) {
		Point2i n = toOcta(estimated[i], (int)q);
		v[i] -= n;
	}
}

void NormalAttr::deltaEncode(std::vector<Quad> &context) {

	if(prediction == DIFF) {
		diffs[0] = values[context[0].t*2];
		diffs[1] = values[context[0].t*2+1];

		for(uint32_t i = 1; i < context.size(); i++) {
			Quad &quad = context[i];
			diffs[i*2 + 0] = values[quad.t*2 + 0] - values[quad.a*2 + 0];
			diffs[i*2 + 1] = values[quad.t*2 + 1] - values[quad.a*2 + 1];
		}
		diffs.resize(context.size()*2); //unreferenced vertices

	} else  {//just reorder diffs, for border story only boundary diffs
		uint32_t count = 0;

		for(uint32_t i = 0; i < context.size(); i++) {
			Quad &quad = context[i];
			if(prediction != BORDER || boundary[quad.t] != 0) { //boundary mark is in old index.
				diffs[count*2 + 0] = values[quad.t*2 + 0];
				diffs[count*2 + 1] = values[quad.t*2 + 1];
				count++;
			}
		}
		diffs.resize(count*2); //unreferenced vertices and borders
	}
}

void NormalAttr::encode(uint32_t /*nvert*/, OutStream &stream) {
	stream.write<uchar>(prediction);
	stream.restart();
	stream.encodeArray<int32_t>(diffs.size()/2, diffs.data(), 2);
	size = stream.elapsed();
}

void NormalAttr::decode(uint32_t nvert, InStream &stream) {
	prediction = stream.readUint8();
	diffs.resize(nvert*2);
	uint32_t readed = stream.decodeArray<int32_t>(diffs.data(), 2);

	if(prediction == BORDER)
		diffs.resize(readed*2);
}

void NormalAttr::deltaDecode(uint32_t nvert, std::vector<Face> &context) {
	if(!buffer) return;

	if(prediction != DIFF)
		return;

	if(context.size()) {
		for(uint32_t i = 1; i < context.size(); i++) {
			Face &f = context[i];
			for(int c = 0; c < 2; c++) {
				int &d = diffs[i*2 + c];
				d += diffs[f.a*2 + c];
			}

		}
	} else { //point clouds assuming values are already sorted by proximity.
		for(uint32_t i = 2; i < nvert*2; i++) {
			int &d = diffs[i];
			d += diffs[i-2];
		}
	}
}

void NormalAttr::postDelta(uint32_t nvert, uint32_t nface,
						   std::map<std::string, VertexAttribute *> &attrs,
						   IndexAttribute &index) {
	if(!buffer) return;

	//for border and estimate we need the position already deltadecoded but before dequantized
	if(prediction == DIFF)
		return;

	if(attrs.find("position") == attrs.end())
		throw "No position attribute found. Use DIFF normal strategy instead.";

	GenericAttr<int> *coord = dynamic_cast<GenericAttr<int> *>(attrs["position"]);
	if(!coord)
		throw "Position attr has been overloaded, Use DIFF normal strategy instead.";

	vector<Point3f> estimated(nvert, Point3f(0, 0, 0));
	if(index.faces32)
		estimateNormals<uint32_t>(nvert, (Point3i *)coord->buffer, nface, index.faces32, estimated);
	else
		estimateNormals<uint16_t>(nvert, (Point3i *)coord->buffer, nface, index.faces16, estimated);

	if(prediction == BORDER) {
		if(index.faces32)
			markBoundary<uint32_t>(nvert, nface, index.faces32, boundary);
		else
			markBoundary<uint16_t>(nvert, nface, index.faces16, boundary);
	}

	switch(format) {
	case FLOAT:
		computeNormals((Point3f *)buffer, estimated);
		break;
	case INT16:
		computeNormals((Point3s *)buffer, estimated);
		break;
	default: throw "Format not supported for normal attribute (float, int16 only)";
	}
}

void NormalAttr::dequantize(uint32_t nvert) {
	if(!buffer) return;

	if(prediction != DIFF)
		return;

	switch(format) {
	case FLOAT:
		for(uint32_t i = 0; i < nvert; i++)
			((Point3f *)buffer)[i] = toSphere(Point2i(diffs[i*2], diffs[i*2 + 1]), (int)q);
		break;
	case INT16:
		for(uint32_t i = 0; i < nvert; i++)
			((Point3s *)buffer)[i] = toSphere(Point2s(diffs[i*2], diffs[i*2 + 1]), (int)q);
		break;
	default: throw "Format not supported for normal attribute (float, int32 or int16 only)";
	}
}

void NormalAttr::computeNormals(Point3s *normals, std::vector<Point3f> &estimated) {
	uint32_t nvert = estimated.size();

	Point2i *diffp = (Point2i *)diffs.data();
	int count = 0; //here for the border.
	for(unsigned int i = 0; i < nvert; i++) {
		Point3f &e = estimated[i];
		Point3s &n = normals[i];

		if(prediction == ESTIMATED || boundary[i]) {
			Point2i &d = diffp[count++];
			Point2i qn = toOcta(e, (int)q);
			n = toSphere(Point2s(qn[0] + d[0], qn[1] + d[1]), (int)q);
		} else {//no correction
			float len = e.norm();
			if(len < 0.00001f)
				e = Point3f(0, 0, 1);
			else {
				len = 32767.0f/len;
				for(int k = 0; k < 3; k++)
					n[k] = (int16_t)(e[k]*len);
			}
		}
	}
}

void NormalAttr::computeNormals(Point3f *normals, std::vector<Point3f> &estimated) {
	uint32_t nvert = estimated.size();

	Point2i *diffp = (Point2i *)diffs.data();
	int count = 0; //here for the border.
	for(unsigned int i = 0; i < nvert; i++) {
		Point3f &e = estimated[i];
		Point3f &n = normals[i];

		if(prediction == ESTIMATED || boundary[i]) {
			Point2i &d = diffp[count++];
			Point2i qn = toOcta(e, (int)q);
			n = toSphere(qn + d, (int)q);
		} else {//no correction
			n = Point3f(e[0], e[1], e[2]);
			n /= n.norm();
		}
	}
}

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

#ifndef CRT_VERTEX_ATTRIBUTE_H
#define CRT_VERTEX_ATTRIBUTE_H

#include <map>
#include <string>
 #include <algorithm>
#include "cstream.h"
#include "index_attribute.h"

namespace crt {

class VertexAttribute {
public:
	enum Format { UINT32 = 0, INT32, UINT16, INT16, UINT8, INT8, FLOAT, DOUBLE };
	enum Strategy { PARALLEL = 0x1, CORRELATED = 0x2 };
	enum CODEC { GENERIC_CODEC = 1, NORMAL_CODEC = 2, COLOR_CODEC = 3, CUSTOM_CODEC = 100 };

	char *buffer;     //output data buffer, input is not needed
	int N;            //number of components
	float q;          //quantization step
	int strategy;

	Format format;    //input or output format
	uint32_t size;    //compressed size (for stats and other nefarious purpouses)
	int bits;    //quantization in bits;

	VertexAttribute(): buffer(nullptr), N(0), q(0.0f), strategy(0), format(INT32), size(0) {}
	virtual ~VertexAttribute(){}

	virtual int codec() = 0; //identifies attribute class.

	//quantize and store as values
	virtual void quantize(uint32_t nvert, const char *buffer) = 0;
	//used by attributes which leverage other attributes (normals, for example)
	virtual void preDelta(uint32_t /*nvert*/, uint32_t /*nface*/, std::map<std::string, VertexAttribute *> &/*attrs*/, IndexAttribute &/*index*/) {}
	//use parallelogram prediction or just diff from v0
	virtual void deltaEncode(std::vector<Quad> &context) = 0;
	//compress diffs and write to stream
	virtual void encode(uint32_t nvert, OutStream &stream) = 0;

	//read quantized data from streams
	virtual void decode(uint32_t nvert, InStream &stream) = 0;
	//use parallelogram prediction to recover values
	virtual void deltaDecode(uint32_t nvert, std::vector<Face> &faces) = 0;
	//use other attributes to estimate (normals for example)
	virtual void postDelta(uint32_t /*nvert*/, uint32_t /*nface*/, std::map<std::string, VertexAttribute *> &/*attrs*/, IndexAttribute &/*index*/) {}
	//reverse quantization operations
	virtual void dequantize(uint32_t nvert) = 0;
};


//T should be int, short or char (for colors for example)
template <class T> class GenericAttr: public VertexAttribute {
public:
	std::vector<T> values, diffs;

	GenericAttr(int dim) { N = dim; }
	virtual ~GenericAttr(){}
	virtual int codec() { return GENERIC_CODEC; }

	virtual void quantize(uint32_t nvert, const char *buffer) {
		uint32_t n = N*nvert;

		values.resize(n);
		diffs.resize(n);
		switch(format) {
		case INT32:
			for(uint32_t i = 0; i < n; i++)
				values[i] = ((const int32_t *)buffer)[i]/q;
			break;
		case INT16:
			for(uint32_t i = 0; i < n; i++)
				values[i] = ((const int16_t *)buffer)[i]/q;
			break;
		case INT8:
			for(uint32_t i = 0; i < n; i++)
				values[i] = ((const int16_t *)buffer)[i]/q;
			break;
		case FLOAT:
			for(uint32_t i = 0; i < n; i++)
				values[i] = ((const float *)buffer)[i]/q;
			break;
		case DOUBLE:
			for(uint32_t i = 0; i < n; i++)
				values[i] = ((const double *)buffer)[i]/q;
			break;
		default: throw "Unsupported format.";
		}
		bits = 0;
		for(int k = 0; k < N; k++) {
			int min = values[k];
			int max = min;
			for(uint32_t i = k; i < n; i +=N) {
				if(min > values[i]) min = values[i];
				if(max < values[i]) max = values[i];
			}
			max -= min;
			bits = std::max(bits, ilog2(max-1) + 1);
//			cout << "max: " << max << " " << " bits: " << bits << endl;

		}
	}

	virtual void deltaEncode(std::vector<Quad> &context) {
		for(int c = 0; c < N; c++)
			diffs[c] = values[context[0].t*N + c];
		for(uint32_t i = 1; i < context.size(); i++) {
			Quad &q = context[i];
			if(q.a != q.b && (strategy & PARALLEL)) {
				for(int c = 0; c < N; c++)
					diffs[i*N + c] = values[q.t*N + c] - (values[q.a*N + c] + values[q.b*N + c] - values[q.c*N + c]);
			} else {
				for(int c = 0; c < N; c++)
					diffs[i*N + c] = values[q.t*N + c] - values[q.a*N + c];
			}
		}
		diffs.resize(context.size()*N); //unreferenced vertices
	}

	virtual void encode(uint32_t nvert, OutStream &stream) {
		stream.restart();
		if(strategy & CORRELATED)
			stream.encodeArray<T>(nvert, diffs.data(), N);
		else
			stream.encodeValues<T>(nvert, diffs.data(), N);

		size = stream.elapsed();
	}

	virtual void decode(uint32_t /*nvert */, InStream &stream) {
		if(strategy & CORRELATED)
			stream.decodeArray<T>((T *)buffer, N);
		else
			stream.decodeValues<T>((T *)buffer, N);
	}

	virtual void deltaDecode(uint32_t nvert, std::vector<Face> &context) {
		if(!buffer) return;

		T *values = (T *)buffer;

		if(strategy & PARALLEL) { //parallelogram prediction
			for(uint32_t i = 1; i < context.size(); i++) {
				Face &f = context[i];
				for(int c = 0; c < N; c++)
					values[i*N + c] += values[f.a*N + c] + values[f.b*N + c] - values[f.c*N + c];
			}
		} else if(context.size()) {  //mesh but not parallelogram prediction
			for(uint32_t i = 1; i < context.size(); i++) {
				Face &f = context[i];
				for(int c = 0; c < N; c++)
					values[i*N + c] += values[f.a*N + c];
			}
		} else {       //point clouds assuming values are already sorted by proximity.
			for(uint32_t i = N; i < nvert*N; i++) {
				values[i] += values[i - N];
			}
		}
	}

	virtual void dequantize(uint32_t nvert) {
		if(!buffer) return;

		T *coords = (T *)buffer;
		uint32_t n = N*nvert;
		switch(format) {
		case FLOAT:
			for(uint32_t i = 0; i < n; i++)
				((float *)buffer)[i] = coords[i]*q;
			break;

		case INT16:
			for(uint32_t i = 0; i < n; i++)
				((uint16_t *)buffer)[i] *= q;
			break;

		case INT32:
			for(uint32_t i = 0; i < n; i++)
				((uint32_t *)buffer)[i] *= q;
			break;

		case INT8:
			for(uint32_t i = 0; i < n; i++)
				((char *)buffer)[i] *= q;
			break;

		case DOUBLE:
			for(uint32_t i = 0; i < n; i++)
				((double *)buffer)[i] = coords[i]*q;
			break;

		case UINT16:
			for(uint32_t i = 0; i < n; i++)
				((uint16_t *)buffer)[i] *= q;
			break;

		case UINT32:
			for(uint32_t i = 0; i < n; i++)
				((uint32_t *)buffer)[i] *= q;
			break;

		case UINT8:
			for(uint32_t i = 0; i < n; i++)
				((char *)buffer)[i] *= q;
			break;
		}
	}
};

}

#endif // CRT_VERTEX_ATTRIBUTE_H

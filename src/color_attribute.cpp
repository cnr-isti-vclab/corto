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

#include "color_attribute.h"
using namespace crt;


void ColorAttr::quantize(uint32_t nvert, const char *buffer) {
	uint32_t n = N*nvert;

	values.resize(n);
	diffs.resize(n);
	switch(format) {
	case UINT8:
	{
		const uint8_t *c = (const uint8_t *)buffer;
		uint8_t *v = values.data();		
		Color4b y;
		for(uint32_t i = 0; i < nvert; i++) {
			for(int k = 0; k < N; k++)
				y[k] = c[k]/qc[k];
			y = y.toYCC();
			for(int k = 0; k < N; k++)
				v[k] = y[k];
			c += N;
			v += N;
		}
	}
		break;

	case FLOAT:
	{
		Color4b y;
		y[3] = 255;
		const float *c = (const float *)buffer;
		uint8_t *v = values.data();
		for(uint32_t i = 0; i < nvert; i++) {
			for(int k = 0; k < N; k++)
				y[k] = ((int)(c[k]*255.0f))/qc[k];
			y = y.toYCC();
			for(int k = 0; k < N; k++)
				v[k] = y[k];
			c += N;
			v += N;
		}
	}
		break;

	default: throw "Unsupported color input format.";
	}
	bits = 0;
}

void ColorAttr::dequantize(uint32_t nvert) {
	if(!buffer) return;

	switch(format) {
	case UINT8:
	{
		uint8_t *c = (uint8_t *)buffer;
		Color4b color;
		color[3] = 255;
		for(uint32_t i = 0; i < nvert; i++) {
			for(int k = 0; k < N; k++)
				color[k] = c[k];
			color = color.toRGB();
			for(int k = 0; k < N; k++)
				c[k] *= qc[k];
			c += out_components;
		}
		break;
	}
	case FLOAT:
	{
		std::vector<Color4b> colors(nvert);
		memcpy(colors.data(), buffer, nvert*sizeof(Color4b));
		float *c = (float *)buffer;
		
		for(uint32_t i = 0; i < nvert; i++) {
			Color4b &rgb = colors[i];
			rgb = rgb.toRGB();
			for(int k = 0; k < out_components; k++)
				c[k] = (c[k]*qc[k])/255.0f;
			c += out_components;
		}
		break;
	}
	default: throw "Unsupported color output format.";
	}
}

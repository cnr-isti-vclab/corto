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
		const Color4b *colors = (const Color4b *)buffer;
		for(uint32_t i = 0; i < nvert; i++) {
			Color4b c = colors[i];
			Color4b &y = *(Color4b *)&(values[i*N]);
			for(int k = 0; k < 4; k++)
				y[k] = c[k]/qc[k];
			y = y.toYCC();
		}
	}
		break;

	case FLOAT:
	{
		const float *colors = (const float *)buffer;
		for(uint32_t i = 0; i < nvert; i++) {
			Color4b &y = *(Color4b *)&(values[i*N]);
			for(int k = 0; k < 4; k++)
				y[k] = ((int)(colors[i*N + k]*255.0f))/qc[k];
			y = y.toYCC();
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
		Color4b *colors = (Color4b *)buffer;
		for(uint32_t i = 0; i < nvert; i++) {
			Color4b &c = colors[i];
			c = c.toRGB();
			for(int k = 0; k < 4; k++)
				c[k] *= qc[k];
		}
		break;
	}
	case FLOAT:
	{
		std::vector<Color4b> colors(nvert);
		memcpy(colors.data(), buffer, nvert*sizeof(Color4b));
		for(uint32_t i = 0; i < nvert; i++) {
			Color4b &c = colors[i];
			c = c.toRGB();
			for(int k = 0; k < 4; k++)
				((float *)buffer)[i*4 +k] = (c[k]*qc[k])/255.0f;
		}
		break;
	}
	default: throw "Unsupported color output format.";
	}
}

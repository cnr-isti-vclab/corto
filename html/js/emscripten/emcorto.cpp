#include <emscripten/bind.h>

#include <emscripten.h>


#include "../../../include/corto/decoder.h"


using namespace crt;
using namespace emscripten;

extern "C" {

Decoder *EMSCRIPTEN_KEEPALIVE newDecoder(int n, const uchar *buffer) {
	return new Decoder(n, buffer);
}

int EMSCRIPTEN_KEEPALIVE ngroups(Decoder *decoder) {
	return decoder->index.groups.size();
}

void EMSCRIPTEN_KEEPALIVE groups(Decoder *decoder, int *groups) {
	int n = decoder->index.groups.size();
	for(int i = 0; i < n; i++) {
		groups[i] = decoder->index.groups[i].end;
	}
}

const char* EMSCRIPTEN_KEEPALIVE getPropName(Decoder* decoder, int group, int prop) {
	int n = decoder->index.groups.size();
	if (group < 0 || group >= n)
		return nullptr;
	int p = decoder->index.groups[group].properties.size();
	if (prop < 0 || prop >= p)
		return nullptr;
	
	int i=0;
	for (auto& property : decoder->index.groups[group].properties) {
		if (i == prop)
			return property.first.c_str();
		i++;
	}
	
	return nullptr;
}

const char* EMSCRIPTEN_KEEPALIVE getPropValue(Decoder* decoder, int group, int prop) {
	int n = decoder->index.groups.size();
	if (group < 0 || group >= n)
		return nullptr;
	int p = decoder->index.groups[group].properties.size();
	if (prop < 0 || prop >= p)
		return nullptr;
	
	int i=0;
	for (auto& property : decoder->index.groups[group].properties) {
		if (i == prop)
			return property.second.c_str();
		i++;
	}
	
	return nullptr;
}



int EMSCRIPTEN_KEEPALIVE groupEnd(Decoder* decoder, int group) {
	int n = decoder->index.groups.size();
	if (group < 0 || group >= n)
		return 0;
	return decoder->index.groups[group].end;
}

int EMSCRIPTEN_KEEPALIVE nprops(Decoder* decoder, int group) {
	return decoder->index.groups[group].properties.size();
}

int EMSCRIPTEN_KEEPALIVE nvert(Decoder *decoder) {
	return decoder->nvert;
}

int EMSCRIPTEN_KEEPALIVE nface(Decoder *decoder) {
	return decoder->nface;
}

bool EMSCRIPTEN_KEEPALIVE hasAttr(Decoder *decoder, const char *attr) {
	return decoder->hasAttr(attr);
}

bool EMSCRIPTEN_KEEPALIVE hasNormal(Decoder *decoder) {
	return decoder->hasAttr("normal");
}

bool EMSCRIPTEN_KEEPALIVE hasColor(Decoder *decoder) {
	return decoder->hasAttr("color");
}

bool EMSCRIPTEN_KEEPALIVE hasUv(Decoder *decoder) {
	return decoder->hasAttr("uv");
}

void EMSCRIPTEN_KEEPALIVE setPositions(Decoder *decoder, float *buffer) {
	decoder->setPositions(buffer);
}

void EMSCRIPTEN_KEEPALIVE setNormals32(Decoder *decoder, float *buffer) {
	decoder->setNormals(buffer);
}

void EMSCRIPTEN_KEEPALIVE setNormals16(Decoder *decoder, int16_t *buffer) {
	decoder->setNormals(buffer);
}

void EMSCRIPTEN_KEEPALIVE setColors(Decoder *decoder, uchar *buffer, int components = 4) {
	decoder->setColors(buffer, components);
}

void EMSCRIPTEN_KEEPALIVE setUvs(Decoder *decoder, float *buffer) {
	decoder->setUvs(buffer);
}

void EMSCRIPTEN_KEEPALIVE setIndex16(Decoder *decoder, uint16_t *buffer) {
	decoder->setIndex(buffer);
}

void EMSCRIPTEN_KEEPALIVE setIndex32(Decoder *decoder, uint32_t *buffer) {
	decoder->setIndex(buffer);
}

void EMSCRIPTEN_KEEPALIVE decode(Decoder *decoder) {
	decoder->decode();
}

void EMSCRIPTEN_KEEPALIVE deleteDecoder(Decoder *decoder) {
	delete decoder;
}

}


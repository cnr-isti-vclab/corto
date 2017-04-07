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
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

#include "nxzencoder.h"
#include "nxzdecoder.h"

#include "tinyply.h"
#include "meshloader.h"
#include "timer.h"

using namespace nx;
using namespace std;
using namespace tinyply;

int main(int argc, char *argv[]) {
	if(argc != 2) {
		cerr << "Usage: " << argv[0] << " [file.ply]\n";
		return 0;
	}

	nx::MeshLoader loader;
	bool ok = loader.load(argv[1]);
	if(!ok) {
		cerr << "Failed loading model: " << argv[1] << endl;
		return 1;
	}

	bool pointcloud = (loader.nface == 0);
	//if(force_pointcloud)
	//	nface = 0;
	nx::Timer timer;

	nx::NxzEncoder encoder(loader.nvert, loader.nface, nx::Stream::TUNSTALL);
	if(pointcloud)
		encoder.addPositions(&*loader.coords.begin());
	else
		encoder.addPositions(&*loader.coords.begin(), &*loader.index.begin());
	//TODO add suppor for wedge and face attributes adding simplex attribute
	if(loader.norms.size())
		encoder.addNormals(&*loader.norms.begin(), 10, nx::NormalAttr::BORDER);

	if(loader.colors.size())
		encoder.addColors(&*loader.colors.begin());

	if(loader.uvs.size())
		encoder.addUvs(&*loader.uvs.begin(), 1.0f/1024.0f);

	if(loader.radiuses.size())
		encoder.addAttribute("radius", (char *)&*loader.radiuses.begin(), nx::VertexAttribute::FLOAT, 1, 1.0f);
	encoder.encode();


	cout << "Encoding time: " << timer.elapsed() << "ms" << endl;

	//encoder might actually change these number (unreferenced vertices, degenerate faces, duplicated coords in point clouds)
	uint32_t nvert = encoder.nvert;
	uint32_t nface = encoder.nface;

	cout << "Nvert: " << nvert << " Nface: " << nface << endl;
	cout << "Compressed to: " << encoder.stream.size() << endl;
	cout << "Ratio: " << 100.0f*encoder.stream.size()/(nvert*12 + nface*12) << "%" << endl;
	cout << "Bpv: " << 8.0f*encoder.stream.size()/nvert << endl << endl;

	cout << "Header: " << encoder.header_size << " bpv: " << (float)encoder.header_size/nvert << endl;

	nx::VertexAttribute *coord = encoder.data["position"];
	cout << "Coord bpv; " << 8.0f*coord->size/nvert << endl;
	cout << "Coord q: " << coord->q << " bits: " << coord->bits << endl << endl;

	nx::VertexAttribute *norm = encoder.data["normal"];
	if(norm) {
		cout << "Normal bpv; " << 8.0f*norm->size/nvert << endl;
		cout << "Normal q: " << norm->q << " bits: " << norm->bits << endl << endl;
	}

	nx::ColorAttr *color = dynamic_cast<nx::ColorAttr *>(encoder.data["color"]);
	if(color) {
		cout << "Color bpv; " << 8.0f*color->size/nvert << endl;
		cout << "Color q: " << color->qc[0] << " " << color->qc[1] << " " << color->qc[2] << " " << color->qc[3] << " bits: " << color->bits << endl << endl;
	}

	nx::GenericAttr<int> *uv = dynamic_cast<nx::GenericAttr<int> *>(encoder.data["uv"]);
	if(uv) {
		cout << "Uv bpv; " << 8.0f*uv->size/nvert << endl;
		cout << "Uv q: " << uv->q << " bits: " << uv->bits << endl << endl;
	}


	nx::GenericAttr<int> *radius = dynamic_cast<nx::GenericAttr<int> *>(encoder.data["radius"]);
	if(radius) {
		cout << "Radius  bpv; " << 8.0f*radius->size/nvert << endl;
		cout << "Radius q: " << radius->q << " bits: " << radius->bits << endl << endl;
	}

	cout << "Face bpv; " << 8.0f*encoder.index.size/nvert << endl;



	timer.start();

	nx::NxzDecoder decoder(encoder.stream.size(), encoder.stream.data());
	assert(decoder.nface == nface);
	assert(decoder.nvert = nvert);

	nx::MeshLoader out;
	out.nvert = encoder.nvert;
	out.nface = encoder.nface;
	out.coords.resize(nvert*3);
	decoder.setPositions(&*out.coords.begin());
	if(decoder.data.count("normal")) {
		out.norms.resize(nvert*3);
		decoder.setNormals(&*out.norms.begin());
	}
	if(decoder.data.count("color")) {
		out.colors.resize(nvert*4);
		decoder.setColors(&*out.colors.begin());
	}
	if(decoder.data.count("uv")) {
		out.uvs.resize(nvert*2);
		decoder.setUvs(&*out.uvs.begin());
	}
	if(decoder.data.count("radius")) {
		out.radiuses.resize(nvert);
		decoder.setAttribute("radius", (char *)&*out.radiuses.begin(), nx::VertexAttribute::FLOAT);
	}
	if(decoder.nface) {
		out.index.resize(nface*3);
		decoder.setIndex(&*out.index.begin());
	}
	decoder.decode();

	int delta = timer.elapsed();
	if(nface) {
		float mfaces = nface/1000000.0f;
		cout << "TOT M faces: " << mfaces << " in: " << delta << "ms or " << 1000*mfaces/delta << " MT/s" << endl;
	} else {
		float mverts = nvert/1000000.0f;
		cout << "TOT M verts: " << mverts << " in: " << delta << "ms or " << 1000*mverts/delta << " MT/s" << endl;
	}

	FILE *file = fopen("test.nxz", "w");
	if(!file) {
		cerr << "Couldl not open file: " << "test.nxz" << endl;
		return 1;
	}
	size_t count = encoder.stream.size();
	size_t written = fwrite ( encoder.stream.data(), 1, count, file);
	if(written != count) {
		cerr << "Failed saving file: " << "test.nxz" << endl;
		return 1;
	}
	std::vector<std::string> comments;
	out.savePly("test.ply", comments);

	return 0;
}


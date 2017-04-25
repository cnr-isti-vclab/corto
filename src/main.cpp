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

#include <unistd.h>
#include <assert.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

#include "encoder.h"
#include "decoder.h"

#include "tinyply.h"
#include "meshloader.h"
#include "timer.h"

using namespace crt;
using namespace std;
using namespace tinyply;

void usage() {
	cerr <<
R"use(Usage: corto [OPTIONS] <FILE>

FILE is the path to a .ply or a .obj 3D model.
  -o <output>: filename of the .crt compressed file.
			   if not specified the extension of the input file will be replaced.
  -e <key=value>: add an exif property, or more than one.
  -p : treat the input as a point cloud."
  -v <bits>: vertex bits quantization. If not specified an euristic is used
  -n <bits>: normal bits quantization. Default 10.
  -c <bits>: color bits quantization. Default 6.
  -u <bits>: texture coordinate bits. Default 10.
  -q <step>: quantization step unit (float) instead of bits for vertex coordinates
  -N <prediction>: normal prediction can be:
	  delta: use difference from previous normal (fastest)
	  estimated: use difference from compute normals (cheaper)
	  border: store difference only for boundary vertices (cheapest, inaccurate)
  -P <file.ply>: decompress and save as .ply for debugging purpouses
)use";
}

static bool endsWith(const std::string& str, const std::string& suffix) {
	return str.size() >= suffix.size() && !str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

int main(int argc, char *argv[]) {

	string input;
	string output;
	string plyfile;
	bool pointcloud = false;
	bool add_normals = false;
	float vertex_q = 0.0f;
	int vertex_bits = 0;
	int norm_bits = 10;
	int r_bits = 6;
	int g_bits = 7;
	int b_bits = 6;
	int a_bits = 5;

	int uv_bits = 12;

	string normal_prediction;
	std::map<std::string, std::string> exif;

	int c;
	while((c = getopt(argc, argv, "pAo:v:n:c:u:q:N:e:P:")) != -1) {
		switch(c) {
		case 'o': output = optarg;  break;  //output filename
		case 'p': pointcloud = true; break; //force pointcloud
		case 'v': vertex_bits = atoi(optarg); break;
		case 'n': norm_bits   = atoi(optarg); break;
		case 'c': r_bits = g_bits = a_bits = b_bits  = atoi(optarg); break;
		case 'r': r_bits = atoi(optarg); break;
		case 'g': g_bits = atoi(optarg); break;
		case 'b': b_bits = atoi(optarg); break;
		case 'a': a_bits = atoi(optarg); break;

		case 'u': uv_bits     = atoi(optarg); break;
		case 'q': vertex_q    = atof(optarg); break;
		case 'A': add_normals = true; break;
		case 'N': normal_prediction = optarg; break;
		case 'P': plyfile = optarg; break; //save ply for debugging purpouses
		case 'e': {
			std::string opt(optarg);
			size_t pos = opt.find('=');
			if(pos == string::npos || pos == 0 || pos == opt.size()-1) {
				cerr << "Expecting key=value or \"key=another value\" for exif arguments" << endl;
				return 1;
			}
			std::string key = opt.substr(0, pos);
			std::string value = opt.substr(pos+1, string::npos);
			exif[key] = value;
			break;
		}
		break;
		case '?': usage(); return 0; break;
		default:
			cerr << "Unknown option: " << (char)c << endl;
			usage();
			return 1;
		}
	}
	if(optind == argc) {
		cerr << "Missing filename" << endl;
		usage();
		return 1;
	}

	if(optind != argc-1) {
		cerr << "Too many arguments\n";
		usage();
		return 1;
	}
	input = argv[optind];

	//options for obj: join by material (discard group info).
	//exif pairs: -exif key=value //write and override what would put inside (mtllib for example).

	crt::MeshLoader loader;
	loader.add_normals = add_normals;
	bool ok = loader.load(input);
	if(!ok) {
		cerr << "Failed loading model: " << argv[1] << endl;
		return 1;
	}

	pointcloud = (loader.nface == 0 || pointcloud);
	crt::NormalAttr::Prediction prediction = crt::NormalAttr::BORDER;
	if(!normal_prediction.empty()) {
		if(normal_prediction == "delta")
			prediction = crt::NormalAttr::DIFF;
		else if(normal_prediction == "border")
			prediction = crt::NormalAttr::BORDER;
		else if(normal_prediction == "estimated")
			prediction = crt::NormalAttr::ESTIMATED;
		else {
			cerr << "Unknown normal prediction: " << prediction << " expecting: delta, border or estimated" << endl;
			return 1;
		}
	}

	crt::Timer timer;

	crt::Encoder encoder(loader.nvert, loader.nface, crt::Stream::TUNSTALL);

	encoder.exif = loader.exif;
	//add and override exif properties
	for(auto it: exif)
		encoder.exif[it.first] = it.second;

	for(auto &g: loader.groups)
		encoder.addGroup(g.end, g.properties);

	if(pointcloud) {
		if(vertex_bits)
			encoder.addPositionsBits(&*loader.coords.begin(), vertex_bits);
		else
			encoder.addPositions(&*loader.coords.begin(), vertex_q);

	} else {
		if(vertex_bits)
			encoder.addPositionsBits(&*loader.coords.begin(), &*loader.index.begin(), vertex_bits);
		else
			encoder.addPositions(&*loader.coords.begin(), &*loader.index.begin(), vertex_q);
	}
	//TODO add suppor for wedge and face attributes adding simplex attribute
	if(loader.norms.size() && norm_bits > 0)
		encoder.addNormals(&*loader.norms.begin(), norm_bits, prediction);

	if(loader.colors.size() && r_bits > 0)
		encoder.addColors(&*loader.colors.begin(), r_bits, g_bits, b_bits, a_bits);

	if(loader.uvs.size() && uv_bits > 0)
		encoder.addUvs(&*loader.uvs.begin(), pow(2, -uv_bits));

	if(loader.radiuses.size())
		encoder.addAttribute("radius", (char *)&*loader.radiuses.begin(), crt::VertexAttribute::FLOAT, 1, 1.0f);
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

	crt::VertexAttribute *coord = encoder.data["position"];
	cout << "Coord bpv; " << 8.0f*coord->size/nvert << endl;
	cout << "Coord q: " << coord->q << " bits: " << coord->bits << endl << endl;

	crt::VertexAttribute *norm = encoder.data["normal"];
	if(norm) {
		cout << "Normal bpv; " << 8.0f*norm->size/nvert << endl;
		cout << "Normal q: " << norm->q << " bits: " << norm->bits << endl << endl;
	}

	crt::ColorAttr *color = dynamic_cast<crt::ColorAttr *>(encoder.data["color"]);
	if(color) {
		cout << "Color bpv; " << 8.0f*color->size/nvert << endl;
		cout << "Color q: " << color->qc[0] << " " << color->qc[1] << " " << color->qc[2] << " " << color->qc[3] << endl;
	}

	crt::GenericAttr<int> *uv = dynamic_cast<crt::GenericAttr<int> *>(encoder.data["uv"]);
	if(uv) {
		cout << "Uv bpv; " << 8.0f*uv->size/nvert << endl;
		cout << "Uv q: " << uv->q << " bits: " << uv->bits << endl << endl;
	}


	crt::GenericAttr<int> *radius = dynamic_cast<crt::GenericAttr<int> *>(encoder.data["radius"]);
	if(radius) {
		cout << "Radius  bpv; " << 8.0f*radius->size/nvert << endl;
		cout << "Radius q: " << radius->q << " bits: " << radius->bits << endl << endl;
	}

	cout << "Face bpv; " << 8.0f*encoder.index.size/nvert << endl;



	timer.start();

	crt::Decoder decoder(encoder.stream.size(), encoder.stream.data());
	assert(decoder.nface == nface);
	assert(decoder.nvert = nvert);

	crt::MeshLoader out;
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
		decoder.setAttribute("radius", (char *)&*out.radiuses.begin(), crt::VertexAttribute::FLOAT);
	}
	if(decoder.nface) {
		out.index.resize(nface*3);
		decoder.setIndex(&*out.index.begin());
	}
	decoder.decode();

	int delta = timer.elapsed();
	if(nface) {
		float mfaces = nface/1000000.0f;
		cout << "TOT M faces: " << mfaces << " in: " << delta << "ms, " << 1000*mfaces/delta << " MT/s" << endl;
	} else {
		float mverts = nvert/1000000.0f;
		cout << "TOT M verts: " << mverts << " in: " << delta << "ms, " << 1000*mverts/delta << " MT/s" << endl;
	}

	if(output.empty()) {
		size_t lastindex = input.find_last_of(".");
		output = input.substr(0, lastindex);
	}
	if(!endsWith(output, ".crt"))
		output += ".crt";

	FILE *file = fopen(output.c_str(), "w");
	if(!file) {
		cerr << "Couldl not open file: " << output << endl;
		return 1;
	}
	size_t count = encoder.stream.size();
	size_t written = fwrite ( encoder.stream.data(), 1, count, file);
	if(written != count) {
		cerr << "Failed saving file: " << "test.crt" << endl;
		return 1;
	}
	std::vector<std::string> comments;
	if(!plyfile.empty())
		out.savePly(plyfile, comments);

	return 0;
}


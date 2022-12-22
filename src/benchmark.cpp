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
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

#include "encoder.h"
#include "decoder.h"

#include "tinyply.h"
#include "meshloader.h"
#include "timer.h"
#include <lz4hc.h>

#include <unordered_map>
#include <sstream>

#ifndef _WIN32
#include <unistd.h>
#else
#include <stdio.h>
#include <string.h>
int opterr = 1, optind = 1, optopt, optreset;
const char* optarg;
int getopt(int nargc, char* const nargv[], const char* ostr);
#endif

#define ENABLE_CONSOLE_LOG	1

using namespace crt;
using namespace std;
using namespace tinyply;

struct CompressionStat
{
	float EncodingTime;
	float DecodingTime;
	uint32_t NTriangles;
	float Ratio;
};

struct CompressionSettings
{
	crt::Stream::Entropy Entropy;
	int CompressionLevel;
	bool PointCloud = false;
	bool AddNormals = true;
	string Group;
	string NormalPrediction;
	std::map<std::string, std::string> Exif;
	float VertexQ = 0.0f;
	int VertexBits = 0;
	int NormBits = 10;
	int RBits = 6;
	int GBits = 7;
	int BBits = 6;
	int ABits = 5;
	int UVBits = 12;

};

struct CompressionResult
{
	uint32_t ColorComponents = 3;
	Encoder* CompressionEncoder;

	uint32_t NVerts;
	uint32_t NFaces;
	uint32_t EncodedSize;
	uint32_t UnencodedSize;
	
	float Ratio;
	float HeaderSizeBpv;
	float Bpv;
	float EncodingTime;

	int Error;

	CompressionResult() = default;
	CompressionResult(int err) { Error = err; }
};

struct DecompressionResult
{
	float DecodingTime;

	int Error;

	DecompressionResult(int err) { Error = err; }
	DecompressionResult() = default;
};


static bool endsWith(const std::string& str, const std::string& suffix) {
	return str.size() >= suffix.size() && !str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

static ifstream::pos_type filesize(const char* filename)
{
	ifstream in(filename, ifstream::ate | ifstream::binary);
	return in.tellg();
}

static string CrtEntropyToString(crt::Stream::Entropy e)
{
	switch (e)
	{
	case crt::Stream::TUNSTALL:return "TUNSTALL";
	case crt::Stream::LZ4:return "LZ4";
	}
	return "";
}


void usage()
{
	cerr <<
	R"use(Usage: cortobenchmark [OPTIONS] <FILE>

	FILE is the path to a file containing paths to .ply or .obj models to use as a benchmark
	  -i <iterations>: number of compression / decompression iterations per model
	  -s <step>: step to transition from a compression level to the next
	)use";
}

static vector<string> GetModelPaths(string inputPath)
{
	vector<string> ret;
	ifstream inputFile(inputPath);
	string path;

	while (inputFile >> path)
		ret.push_back(path);

	return ret;
}

CompressionResult CompressModel(string path, CompressionSettings& settings)
{
	Timer loadingTime;
	CompressionResult ret;
	// Load mesh
	static crt::MeshLoader loader;
	static string prevPath;

	if (path.compare(prevPath) != 0)
	{
		prevPath = path;
		loader = crt::MeshLoader();
		loader.add_normals = settings.AddNormals;
		bool ok = loader.load(path, settings.Group);
		if (!ok) {
			cerr << "Failed loading model: " << path << endl;
			return 1;
		}
	}

	crt::NormalAttr::Prediction prediction = crt::NormalAttr::BORDER;
	crt::Timer timer;

	// Encode
	crt::Encoder* encoder = new Encoder(loader.nvert, loader.nface, settings.Entropy);
	ret.CompressionEncoder = encoder;

	encoder->setCompressionLevel(settings.CompressionLevel);
	encoder->exif = loader.exif;
	//add and override exif properties
	for (auto it : settings.Exif)
		encoder->exif[it.first] = it.second;

	for (auto& g : loader.groups)
		encoder->addGroup(g.end, g.properties);

	if (settings.PointCloud) {
		if (settings.VertexBits)
			encoder->addPositionsBits(loader.coords.data(), settings.VertexBits);
		else
			encoder->addPositions(loader.coords.data(), settings.VertexQ);

	}
	else {
		if (settings.VertexBits)
			encoder->addPositionsBits(loader.coords.data(), loader.index.data(), settings.VertexBits);
		else
			encoder->addPositions(loader.coords.data(), loader.index.data(), settings.VertexQ);
	}
	//TODO add suppor for wedge and face attributes adding simplex attribute
	if (loader.norms.size() && settings.NormBits > 0)
		encoder->addNormals(loader.norms.data(), settings.NormBits, prediction);

	if (loader.colors.size() && settings.RBits > 0) {
		if (loader.nColorsComponents == 3) {
			encoder->addColors3(loader.colors.data(), settings.RBits, settings.GBits, settings.BBits);
		}
		else
			encoder->addColors(loader.colors.data(), settings.RBits, settings.GBits, settings.BBits, settings.ABits);
	}

	if (loader.uvs.size() && settings.UVBits > 0)
		encoder->addUvs(loader.uvs.data(), pow(2, -settings.UVBits));

	if (loader.radiuses.size())
		encoder->addAttribute("radius", (char*)loader.radiuses.data(), crt::VertexAttribute::FLOAT, 1, 1.0f);
	encoder->encode();

	// Setup result
	ret.ColorComponents = loader.nColorsComponents;
	ret.EncodingTime = timer.elapsed();
	ret.Error = 0;
	ret.NVerts = encoder->nvert;
	ret.NFaces = encoder->nface;
	ret.EncodedSize = encoder->stream.size();
	ret.HeaderSizeBpv = (float)encoder->header_size / ret.NVerts;
	ret.Bpv = 8.0f * encoder->stream.size() / ret.NVerts;
	ret.Ratio = 100.0f * encoder->stream.size() / (ret.NVerts * 12 + ret.NFaces * 12);

	return ret;
}

DecompressionResult DecompressModel(string path, CompressionResult& res)
{
	DecompressionResult ret;
	Timer timer;
	timer.start();

	// Decode
	crt::Decoder decoder(res.CompressionEncoder->stream.size(), res.CompressionEncoder->stream.data());
	assert(decoder.nface == res.NFaces);
	assert(decoder.nvert == res.NVerts);

	crt::MeshLoader out;
	out.nvert = res.CompressionEncoder->nvert;
	out.nface = res.CompressionEncoder->nface;
	out.coords.resize(res.NVerts * 3);
	decoder.setPositions(out.coords.data());
	if (decoder.data.count("normal")) {
		out.norms.resize(res.NVerts * 3);
		decoder.setNormals(out.norms.data());
	}
	if (decoder.data.count("color")) {
		out.colors.resize(res.NVerts * res.ColorComponents);
		out.nColorsComponents = res.ColorComponents;
		decoder.setColors(out.colors.data(), res.ColorComponents);
	}
	if (decoder.data.count("uv")) {
		out.uvs.resize(res.NVerts * 2);
		decoder.setUvs(out.uvs.data());
	}
	if (decoder.data.count("radius")) {
		out.radiuses.resize(res.NVerts);
		decoder.setAttribute("radius", (char*)out.radiuses.data(), crt::VertexAttribute::FLOAT);
	}
	if (decoder.nface) {
		out.index.resize(res.NFaces * 3);
		decoder.setIndex(out.index.data());
	}
	decoder.decode();

	int delta = timer.elapsed();
	float DecodingTime;

	ret.DecodingTime = delta;
	ret.Error = 0;

	return ret;
}

int ParseOptions(int argc, char** argv, string& inputPath, uint32_t& nIterations, uint32_t& step)
{
	int c;
	nIterations = 10;
	step = 1;

	while ((c = getopt(argc, argv, "i::s::")) != -1) {
		switch (c) {
		case 'i': 
		{
			int arg = atoi(optarg);
			nIterations = arg > 0 ? arg : 10;  
			break;  // n iterations
		}
		case 's':
		{
			int arg = atoi(optarg);
			step = arg > 0 ? arg : 1;
			break; // compression level step
		}
		case '?': usage(); return -1;
		default:
			cerr << "Unknown option: " << (char)c << endl;
			usage();
			return -2;
		}
	}
	if (optind == argc) {
		cerr << "Missing filename" << endl;
		usage();
		return -3;
	}

	if (optind != argc - 1) {
#ifdef _WIN32
		cerr << "Too many arguments or argument before other options\n";
#else
		cerr << "Too many arguments\n";
#endif
		usage();
		return -4;
	}

	inputPath = argv[optind];
	return 0;
}

int main(int argc, char *argv[]) {
	string inputPath;
	uint32_t nIterations, compressionLevelIncrease;
	if (ParseOptions(argc, argv, inputPath, nIterations, compressionLevelIncrease) != 0)
		return -1;

	string csvContent;
	ofstream results("results.csv");
	unordered_map<crt::Stream::Entropy, std::vector<int>> compressionLevels = {
		{crt::Stream::LZ4, {LZ4HC_CLEVEL_MIN-1, LZ4HC_CLEVEL_MAX}},
		{crt::Stream::TUNSTALL, {0,0}}
	};

	std::vector<std::string> modelPaths = GetModelPaths(inputPath);
	std::vector<crt::Stream::Entropy> toCompare = { crt::Stream::TUNSTALL, crt::Stream::LZ4 };

	// Generate CSV header
	csvContent = "Model,";
	for (uint32_t i = 0; i < toCompare.size(); i++)
	{
		std::stringstream ss;
		string entropyString = CrtEntropyToString(toCompare[i]);

		for (uint32_t c = compressionLevels[toCompare[i]][0]; c <= compressionLevels[toCompare[i]][1]; c+=compressionLevelIncrease)
		{
			ss << entropyString << " Lev" << c << " enc,";
			ss << entropyString << " Lev" << c << " dec,";
			ss << entropyString << " Lev" << c << " ratio,";
			ss << entropyString << " Lev" << c << " enc MT/s,";
			ss << entropyString << " Lev" << c << " dec MT/s,";
		}
		
		csvContent += ss.str();
	}
	csvContent += "\n";

	for (uint32_t m = 0; m < modelPaths.size(); m++)
	{
		string modelResults = modelPaths[m] + ",";
#if ENABLE_CONSOLE_LOG
		cout << "CURRENT MODEL: " << modelPaths[m] << endl;
#endif
		for (uint32_t e = 0; e < toCompare.size(); e++)
		{
#if ENABLE_CONSOLE_LOG
			cout << "CURRENT ENTROPY: " << CrtEntropyToString(toCompare[e]) << endl;
#endif
			int minLevel = compressionLevels[toCompare[e]][0], maxLevel = compressionLevels[toCompare[e]][1];
			// Compression levels
			for (int l = minLevel; l <= maxLevel; l += compressionLevelIncrease)
			{
#if ENABLE_CONSOLE_LOG
				cout << "COMPRESSION LEVEL: " << l << endl;
#endif
				float encodingTime = 0, decodingTime = 0;
				CompressionSettings settings;

				settings.CompressionLevel = l;
				settings.Entropy = toCompare[e];

				CompressionResult compResult;
				DecompressionResult decompResult;

				for (uint32_t i = 0; i < nIterations; i++)
				{
					compResult = CompressModel(modelPaths[m], settings);
					if (compResult.Error != 0)
					{
						cout << "A compression error occurred, saving partial results and aborting benchmark." << endl;
						results << csvContent;
						results.close();
						return -1;
					}
					
					decompResult = DecompressModel(modelPaths[m], compResult);
					if (decompResult.Error != 0)
					{
						cout << "A decompression error occurred, saving partial results and aborting benchmark." << endl;
						results << csvContent;
						results.close();
						return -1;
					}

					encodingTime += compResult.EncodingTime;
					decodingTime += decompResult.DecodingTime;

					delete compResult.CompressionEncoder;
				}

				compResult.UnencodedSize = filesize((modelPaths[m]).c_str());
				compResult.EncodingTime = (float)encodingTime / nIterations;
				decompResult.DecodingTime = (float)decodingTime / nIterations;

#if ENABLE_CONSOLE_LOG
				cout << "Encoding time: " << compResult.EncodingTime << endl 
					 << "Decoding time: " << decompResult.DecodingTime << endl;
#endif

				CompressionStat stats = { compResult.EncodingTime, decompResult.DecodingTime, compResult.Ratio };

				stringstream ss;
				ss << compResult.EncodingTime << "," << decompResult.DecodingTime << "," << compResult.Ratio << "," <<
					((compResult.NVerts / 3) / (max(compResult.EncodingTime, 0.1f) * 1000.0f)) << "," << 
					((compResult.NVerts / 3)/ (max(decompResult.DecodingTime, 0.1f) * 1000.0f)) << ",";
				modelResults += ss.str();
			}
		}
		modelResults += "\n";
		csvContent += modelResults;
	}

	results << csvContent;
	results.close();

	return 0;
}


#ifdef _WIN32

int getopt(int nargc, char* const nargv[], const char* ostr) {
	static const char* place = "";        // option letter processing
	const char* oli;                      // option letter list index

	if (optreset || !*place) {             // update scanning pointer
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = "";
			return -1;
		}

		if (place[1] && *++place == '-') { // found "--"
			++optind;
			place = "";
			return -1;
		}
	}                                       // option letter okay?

	if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, optopt))) {
		// if the user didn't specify '-' as an option,  assume it means -1.
		if (optopt == (int)'-')
			return (-1);
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			cout << "illegal option -- " << optopt << "\n";
		return ('?');
	}

	if (*++oli != ':') {                    // don't need argument
		optarg = NULL;
		if (!*place)
			++optind;

	}
	else {                                // need an argument
		if (*place)                         // no white space
			optarg = place;
		else if (nargc <= ++optind) {       // no arg
			place = "";
			if (*ostr == ':')
				return (':');
			if (opterr)
				cout << "option requires an argument -- " << optopt << "\n";
			return (':');
		}
		else                              // white space
			optarg = nargv[optind];
		place = "";
		++optind;
	}
	return optopt;                          // dump back option letter
}

#endif

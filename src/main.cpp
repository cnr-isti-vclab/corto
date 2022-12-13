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

/* CRASH
*		name		idx		Opt?	NonOpt?		TUNSTALL?	ZSTD?
	
	-	gargo		8
	-	laurana		9
	-	mba			10		YES					YES			YES
	-	moebius		11		/		YES			YES			/
	-	sphere		17		/		YES			YES			YES

*/

#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

#include "encoder.h"
#include "decoder.h"

#include "tinyply.h"
#include "tinydir.h"
#include "meshloader.h"
#include "timer.h"

#define ENABLE_CONSOLE_LOG	1


using namespace crt;
using namespace std;
using namespace tinyply;

struct MeshData
{
	uint32_t ColorComponents;
};

struct CompressionSettings
{
	crt::Stream::Entropy Entropy;
	bool Optimize;
	int CompressionLevel;
	bool PointCloud = false;
	bool AddNormals = false;
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
	MeshData Mesh;
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

static vector<string> GetModelPaths(string root)
{
	std::vector<string> ret;
	tinydir_dir dir;
	tinydir_open(&dir, root.c_str());

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (file.is_dir && file.is_reg)
		{
			// Explore the dir
			vector<string> result = GetModelPaths(file.path);
			ret.insert(ret.end(), result.begin(), result.end());
		}
		else if (endsWith(file.path, ".ply") || endsWith(file.path, ".obj"))
			ret.push_back(file.name);
		
		tinydir_next(&dir);
	}

	tinydir_close(&dir);

	return ret;
}

CompressionResult CompressModel(string path, CompressionSettings& settings)
{
	CompressionResult ret;
	// Load mesh
	crt::MeshLoader loader;
	loader.add_normals = settings.AddNormals;
	bool ok = loader.load(path, settings.Group);
	if (!ok) {
		cerr << "Failed loading model: " << path << endl;
		return 1;
	}

	if (settings.PointCloud)
		loader.nface = 0;
	settings.PointCloud = (loader.nface == 0 || settings.PointCloud);
	crt::NormalAttr::Prediction prediction = crt::NormalAttr::BORDER;
	if (!settings.NormalPrediction.empty()) {
		if (settings.NormalPrediction == "delta")
			prediction = crt::NormalAttr::DIFF;
		else if (settings.NormalPrediction == "border")
			prediction = crt::NormalAttr::BORDER;
		else if (settings.NormalPrediction == "estimated")
			prediction = crt::NormalAttr::ESTIMATED;
		else {
			cerr << "Unknown normal prediction: " << prediction << " expecting: delta, border or estimated" << endl;
			return 1;
		}
	}

	crt::Timer timer;

	// Encode
	crt::Encoder* encoder = new Encoder(loader.nvert, loader.nface, settings.Entropy);
	ret.CompressionEncoder = encoder;

	encoder->setOptimizationMode(settings.Optimize);
	encoder->setZstdCompressionLevel(settings.CompressionLevel);
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
		out.colors.resize(res.NVerts * res.Mesh.ColorComponents);
		out.nColorsComponents = res.Mesh.ColorComponents;
		decoder.setColors(out.colors.data(), res.Mesh.ColorComponents);
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
	cout << endl << "Data set";
	decoder.decode();
	cout << endl;

	int delta = timer.elapsed();
	float DecodingTime;

	ret.DecodingTime = delta;
	ret.Error = 0;

	if (!endsWith(path, ".crt"))
		path += ".crt";

	/*FILE* file = fopen(("cortomodels/compressed/" + path).c_str(), "wb");
	if (!file) {
		cerr << "Could not open file: " << path << endl;
		return 1;
	}
	size_t count = res.CompressionEncoder->stream.size();
	size_t written = fwrite(res.CompressionEncoder->stream.data(), 1, count, file);
	if (written != count) {
		cerr << "Failed saving file: " << "test.crt" << endl;
		return 1;
	}
	std::vector<std::string> comments;
	if (!path.empty())
		out.savePly("cortomodels/decompressed/" + path + ".ply", comments);
		*/

	return ret;
}

void ResultMessage(CompressionSettings& settings, string modelPath, CompressionResult cr, DecompressionResult dr, ofstream& outFile)
{
	string encodingString = settings.Entropy == crt::Stream::Entropy::TUNSTALL ? "Tunstall" : "ZSTD";
	string acceleratedString = settings.Optimize ? "Accelerated" : "Unaccelerated";

	outFile << "MODEL: " << modelPath << "\t\t" << acceleratedString << " " << encodingString << " level " << settings.CompressionLevel << endl;
	outFile << "COMPRESSSION DATA:" << endl;
	outFile << "Uncompressed size:\t" << cr.UnencodedSize<< "\tEncoded size: " << cr.EncodedSize << endl;
	outFile << "Encoding time:\t" << cr.EncodingTime << "\tDecoding time: " << dr.DecodingTime<< endl;
	outFile << "Vertices:\t" << cr.NVerts << "\tCompression ratio: " << cr.Ratio << endl << endl;
}

int main(int argc, char *argv[]) {
	string input;
	string output;
	string plyfile;
	ofstream results("results.txt");

	int nIterations = 1, modelIncrease = 1;
	int minCompressionLevel = -5, maxCompressionLevel = 22, compressionLevelIncrease = 5;
	bool accelerateCompression[2] = { false, true };
	bool hasCompressionLevels[2] = { false, true };
	std::vector<std::string> modelPaths = GetModelPaths("cortomodels/models");
	crt::Stream::Entropy toCompare[2] = {crt::Stream::TUNSTALL, crt::Stream::ZSTD};

	// Entropy method
	for (uint32_t e = 0; e < 2; e++)
	{
#if ENABLE_CONSOLE_LOG
		cout << "TESTING " << (toCompare[e] == crt::Stream::TUNSTALL ? "TUNSTALL" : "ZSTD") << endl;
#endif
		// Compression acceleration
		for (uint32_t a = 0; a < 2; a++)
		{
#if ENABLE_CONSOLE_LOG
			cout << "Acceleration: " << accelerateCompression[a] << endl;
#endif
			for (uint32_t m = 0; m <modelPaths.size(); m += modelIncrease)
			{
#if ENABLE_CONSOLE_LOG
				cout << "Model: " << modelPaths[m] << endl;
#endif
				int minLevel = 0, maxLevel = 1;
				if (hasCompressionLevels[e])
				{
					minLevel = minCompressionLevel;
					maxLevel = maxCompressionLevel;
				}
				// Compression levels
				for (int l = minLevel; l < maxLevel; l += compressionLevelIncrease)
				{
#if ENABLE_CONSOLE_LOG
					cout << "Compression level: " << l << endl;
#endif
					float encodingTime = 0, decodingTime = 0;
					CompressionSettings settings;

					settings.CompressionLevel = l;
					settings.Optimize = accelerateCompression[a];
					settings.Entropy = toCompare[e];

					CompressionResult compResult;
					DecompressionResult decompResult;

					for (uint32_t i = 0; i < nIterations; i++)
					{
						cout << "Compressing...";
						compResult = CompressModel("cortomodels/models/" + modelPaths[m], settings);
						cout << "compressed." << endl;

						cout << "Decompressing...";
						decompResult = DecompressModel(modelPaths[m], compResult);
						cout << "decompressed." << endl;

						encodingTime += compResult.EncodingTime;
						decodingTime += decompResult.DecodingTime;						

						delete compResult.CompressionEncoder;
					}

					compResult.UnencodedSize = filesize(("cortomodels/models/" + modelPaths[m]).c_str());
					compResult.EncodingTime = (float)encodingTime / nIterations;
					decompResult.DecodingTime = (float)decodingTime / nIterations;

					ResultMessage(settings, modelPaths[m], compResult, decompResult, results);
				}
			}
		}
	}

	results.close();
	return 0;
}
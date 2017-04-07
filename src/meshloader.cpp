#include <assert.h>

#include "meshloader.h"
#include "tinyply.h"
#include "objload.h"
#include "point.h"

using namespace nx;
using namespace tinyply;
using namespace std;


static bool endsWith(const std::string& str, const std::string& suffix) {
	return str.size() >= suffix.size() && !str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool MeshLoader::load(const std::string &filename) {
	if(endsWith(filename, ".ply") || endsWith(filename, ".PLY"))
		return loadPly(filename);
	if(endsWith(filename, ".obj") || endsWith(filename, ".OBJ"))
		return loadObj(filename);
	return false;
}


bool MeshLoader::loadPly(const std::string &filename) {
	std::ifstream ss(filename, std::ios::binary);
	if(!ss.is_open())
		return false;
	PlyFile ply(ss);

	ply.request_properties_from_element("vertex", { "x", "y", "z" }, coords);
	ply.request_properties_from_element("vertex", { "nx", "ny", "nz" }, norms);
	ply.request_properties_from_element("vertex", { "red", "green", "blue", "alpha" }, colors);
	ply.request_properties_from_element("vertex", { "texture_u", "texture_v" }, uvs);
	ply.request_properties_from_element("vertex", { "radius" }, radiuses);

	ply.request_properties_from_element("face", { "vertex_indices" }, index, 3);
	ply.request_properties_from_element("face", { "texcoord" }, wedge_uvs, 6);
	ply.read(ss);

	nface = index.size()/3;
	nvert = coords.size()/3;
	if(wedge_uvs.size() || wedge_norms.size())
		splitWedges();

	for(uint32_t i = 0; i < index.size(); i++)
		assert(index[i] < coords.size()/3);

	comments = ply.comments;
	return true;
}

bool MeshLoader::loadObj(const std::string &filename) {

	obj::Model m = obj::loadModelFromFile(filename);

	swap(m.vertex, coords);
	swap(m.texCoord, uvs);
	swap(m.normal, norms);

	nvert = coords.size()/3;
	nface = 0;
	for(auto it: m.faces) {
		nface += it.second.size()/3;
		index.insert(index.end(), it.second.begin(), it.second.end());
		groups.push_back(nface);
	}

	for(uint32_t i = 0; i < index.size(); i++)
		assert(index[i] < coords.size()/3);

	return nvert > 0;
}

void MeshLoader::splitWedges() {
	std::vector<bool> visited(nvert, false);
	uvs.resize(nvert*2);
	std::multimap<uint32_t, uint32_t> duplicated;
	for(uint32_t i = 0; i < index.size(); i++) {
		uint32_t k = index[i];
		Point3f p = *(Point3f *)&coords[k*3];
		Point2f wuv = *(Point2f *)&wedge_uvs[i*2]; //wedge uv
		Point2f &uv = *(Point2f *)&uvs[k*2];
		if(!visited[k]) {
			visited[k] = true;
			uv = wuv;
			continue;
		}
		if(uv == wuv)
			continue;

		uint32_t found = 0xffffffff;
		auto d = duplicated.find(k);
		if(d != duplicated.end()) {
			auto range = duplicated.equal_range(k);
			for (auto i = range.first; i != range.second; ++i) {
				uint32_t j = i->second;
				if(*(Point2f *)&uvs[j*2] == wuv) {
					found = i->first;
					break;
				}
			}
		}
		if(found == 0xffffffff) {
			found = coords.size()/3;
			coords.push_back(p[0]);
			coords.push_back(p[1]);
			coords.push_back(p[2]);

			uvs.push_back(wuv[0]);
			uvs.push_back(wuv[1]);

			if(colors.size())
				for(int j = 0; j < 4; j++)
					colors.push_back(colors[k*4 + j]);

			//TODO do the same thing for all other attributes

			duplicated.insert(std::make_pair(k, found));
		}
		assert(found < coords.size()/3);
		index[i] = found;
	}
	nface = index.size()/3;
	nvert = coords.size()/3;
}

bool MeshLoader::savePly(const string &filename, std::vector<std::string> &comments) {
	std::filebuf fb;
	fb.open(filename, std::ios::out | std::ios::binary);
	if(!fb.is_open())
		return false;
	std::ostream outputStream(&fb);
	PlyFile out;
	out.comments = comments;

	out.add_properties_to_element("vertex", { "x", "y", "z" }, coords);
	if(norms.size())
		out.add_properties_to_element("vertex", { "nx", "ny", "nz" }, norms);
	if(colors.size())
		out.add_properties_to_element("vertex", { "red", "green", "blue", "alpha" }, colors);
	if(uvs.size()) {
	/*		for(int i = 0; i < reindex.size(); i++) {
			tex[i*2] = reuvs[reindex[i]*2];
			tex[i*2+1] = reuvs[reindex[i]*2+1];
	}*/
		out.add_properties_to_element("vertex", { "texture_u", "texture_v" }, uvs);
	}
	if(radiuses.size())
		out.add_properties_to_element("vertex", { "radius" }, radiuses);

	if(nface > 0)
		out.add_properties_to_element("face", { "vertex_indices" }, index, 3, PlyProperty::Type::UINT8);

	out.write(outputStream, true);
	fb.close();
	return true;
}

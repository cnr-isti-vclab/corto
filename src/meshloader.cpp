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

#include "meshloader.h"
#include "tinyply.h"
#include "objload.h"
#include "point.h"

using namespace crt;
using namespace tinyply;
using namespace std;


static bool endsWith(const std::string& str, const std::string& suffix) {
	return str.size() >= suffix.size() && !str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

static bool startsWith(const std::string& str, const std::string& prefix) {
	return str.size() >= prefix.size() && !str.compare(0, prefix.size(), prefix);
}

bool MeshLoader::load(const std::string &filename, const string &group) {
	if(endsWith(filename, ".ply") || endsWith(filename, ".PLY"))
		return loadPly(filename);
	if(endsWith(filename, ".obj") || endsWith(filename, ".OBJ"))
		return loadObj(filename, group);
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
	ply.request_properties_from_element("face", { "texnumber" }, tex_number, 1);

	ply.read(ss);

	nface = index.size()/3;
	nvert = coords.size()/3;
	if(colors.size())
		nColorsComponents = colors.size()/nvert;
	
	if(add_normals && !norms.size() && !wedge_norms.size())
		addNormals();

	if(wedge_uvs.size() || wedge_norms.size())
		splitWedges();

	for(uint32_t i = 0; i < index.size(); i++)
		assert(index[i] < coords.size()/3);

	//create groups:
	if(tex_number.size()) {
		uint32_t notexcount = 0;
		vector<uint32_t> count;
		//count each tex numner (including no texture ones.
		for(int t: tex_number) {
			if(t >= (int)count.size()) {
				count.resize(t+1, 0);
			}
			if(t < 0)
				notexcount++;
			else
				count[t]++;
		}
		if(notexcount)
			count.push_back(notexcount);

		//init each end with what is actually the start, we will increase it later.
		groups.resize(count.size(), 0);
		for(size_t i = 0; i < count.size()-1; i++)
			groups[i+1].end = groups[i].end + count[i];

		vector<uint32_t> tmp(index.size());
		for(size_t i = 0; i < tex_number.size(); i++) {
			int t = tex_number[i];
			if(t < 0) t = groups.size()-1;
			uint32_t &o = groups[t].end;
			tmp[o*3] = index[i*3];
			tmp[o*3+1] = index[i*3+1];
			tmp[o*3+2] = index[i*3+2];
			o++;
		}
		swap(tmp, index);
	} else {
		groups.push_back(Group(index.size()/3));
	}

	uint32_t texcount = 0;
	for(auto &str: ply.comments)
		if(startsWith(str, "TextureFile") && texcount < groups.size())
			groups[texcount++].properties["texture"] = str.substr(12, str.size());

	return true;
}

bool MeshLoader::loadObj(const std::string &filename, const std::string &groupname) {

	obj::IndexedModel m = obj::loadModelFromFile(filename);

	for(auto &mat: m.mtllibs)
		exif["mtllib"] = mat;
	swap(m.vertex, coords);
	swap(m.texCoord, uvs);
	swap(m.normal, norms);
	swap(m.faces, index);


	int keepgroup = -1;
	for(auto &block: m.blocks) {
		Group g(block.end);
		if(block.material.size())
			g.properties["material"] = block.material;
		if(block.groups.size()) {
			std::string str;
			for(auto &group: block.groups) {
				if(groupname.size() && group == groupname)
					keepgroup = groups.size();
				str.append(group);
				str.append(" ");
			}
			str.pop_back();
			g.properties["groups"] = str;
		}
		groups.push_back(g);
	}


	if(keepgroup != -1) { //remove all other groups
		int start = 0;
		if(keepgroup > 0)
			start = groups[keepgroup-1].end;
		Group g = groups[keepgroup];
		vector<int> reorder(coords.size(), -1);
		int facecount = 0;
		int vertcount = 0;
		vector<float> newcoords(coords.size());
		for(int i = start; i < g.end; i++) {
			for(int k = 0; k < 3; k++) {
				int v = index[i*3+k];
				if(reorder[v] == -1) {
					newcoords[vertcount*3] = coords[v*3];
					newcoords[vertcount*3+1] = coords[v*3+1];
					newcoords[vertcount*3+2] = coords[v*3+2];
					reorder[v] = vertcount++;
				}
				index[facecount*3 + k] = reorder[v];
			}
			facecount++;
		}
		newcoords.resize(vertcount*3);
		index.resize(facecount*3);
		swap(coords, newcoords);
		g.end = facecount;
		groups[0] = g;
		groups.resize(1);

	}

	nvert = coords.size()/3;
	nface = index.size()/3;

	for(uint32_t i = 0; i < index.size(); i++)
		assert(index[i] < nvert);

	return nvert > 0;
}

void MeshLoader::splitWedges() {
	if(wedge_uvs.size() == 0 && wedge_norms.size() == 0)
		return;

	std::vector<bool> visited(nvert, false);
	bool has_wedge_uvs = wedge_uvs.size();
	bool has_wedge_norms = wedge_norms.size();

	if(has_wedge_uvs)
	uvs.resize(nvert*2);
	if(has_wedge_norms)
		norms.resize(nvert*3);

	std::multimap<uint32_t, uint32_t> duplicated;
	for(uint32_t i = 0; i < index.size(); i++) {
		uint32_t k = index[i];
		Point3f p = *(Point3f *)&coords[k*3];

		bool split = false;

		if(has_wedge_uvs) {
			Point2f wuv = *(Point2f *)&wedge_uvs[i*2]; //wedge uv
			Point2f &uv = *(Point2f *)&uvs[k*2];
			if(!visited[k])
				uv = wuv;
			else
				split |= (uv != wuv);
		}
		if(has_wedge_norms) {
			Point2f wn = *(Point2f *)&wedge_norms[i*3]; //wedge uv
			Point2f &n = *(Point2f *)&norms[k*3];
			if(!visited[k])
				n = wn;
			else
				split |= (n != wn);
		}
		if(!visited[k]) {
			visited[k] = true;
			continue;
		}
		if(!split)
			continue;

		uint32_t found = 0xffffffff;
		auto d = duplicated.find(k);
		if(d != duplicated.end()) {
			auto range = duplicated.equal_range(k);
			for (auto it = range.first; it != range.second; ++it) {
				uint32_t j = it->second;
				if((!has_wedge_uvs || *(Point2f *)&uvs[j*2] == *(Point2f *)&wedge_uvs[i*2]) &&
				(!has_wedge_norms || *(Point2f *)&norms[j*3] == *(Point2f *)&wedge_norms[i*3]))
					found = j;
			}
		}
		if(found == 0xffffffff) {
			found = coords.size()/3;
			coords.push_back(p[0]);
			coords.push_back(p[1]);
			coords.push_back(p[2]);
			if(norms.size()) {
				norms.push_back(norms[k*3]);
				norms.push_back(norms[k*3+1]);
				norms.push_back(norms[k*3+2]);
			}

			if(has_wedge_uvs) {
				uvs.push_back(wedge_uvs[i*2 + 0]);
				uvs.push_back(wedge_uvs[i*2 + 1]);
			}

			if(has_wedge_norms) {
				norms.push_back(wedge_norms[i*3 + 0]);
				norms.push_back(wedge_norms[i*3 + 1]);
				norms.push_back(wedge_norms[i*3 + 2]);
			}

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
	
	if(colors.size()) {
		if(nColorsComponents == 4)
			out.add_properties_to_element("vertex", { "red", "green", "blue", "alpha" }, colors);
		else if(nColorsComponents == 3)
			out.add_properties_to_element("vertex", { "red", "green", "blue" }, colors);
	}
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

void MeshLoader::addNormals() {

	norms.resize(nvert*3, 0);

	uint32_t *end = index.data() + index.size();
	Point3f *coord = (Point3f *)coords.data();
	Point3f *norm = (Point3f *)norms.data();
	for(uint32_t *f = index.data(); f < end; f += 3) {
		assert(f[0]*3 < coords.size());
		Point3f &p0 = coord[f[0]];
		Point3f &p1 = coord[f[1]];
		Point3f &p2 = coord[f[2]];
		Point3f n = (( p1 - p0) ^ (p2 - p0));
		norm[f[0]] += n+Point3f(10, 10, 10);
		norm[f[1]] += n+Point3f(10, 10, 10);
		norm[f[2]] += n+Point3f(10, 10, 10);
	}
	for(uint32_t i = 0; i < nvert; i++) {
		Point3f &n = norm[i];
		float len = n.norm();
		if(len == 0)
			n = Point3f(0, 0, 1);
		else
			n /= len;
	}
}

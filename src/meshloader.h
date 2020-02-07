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

#ifndef CRT_MESHLOADER_H
#define CRT_MESHLOADER_H

#include <string>
#include <vector>
#include <map>

namespace crt {

class MeshLoader {
public:

	struct Group {
		std::map<std::string, std::string> properties;
		uint32_t end;
		Group(uint32_t e = 0): end(e) {}
	};

	MeshLoader(): add_normals(false) {}
	bool load(const std::string &filename, const std::string &group = "");
	bool loadPly(const std::string &filename);
	bool loadObj(const std::string &filename, const std::string &group);
	void splitWedges();
	bool savePly(const std::string &filename, std::vector<std::string> &comments);

	bool add_normals;    //add normals (if not present) before splitting groups.
	uint32_t nface;
	uint32_t nvert;
	std::vector<float> coords;
	std::vector<float> norms;
	std::vector<float> uvs;
	std::vector<float> radiuses;
	std::vector<uint8_t> colors;
	uint32_t nColorsComponents = 0;

	std::vector<uint32_t> index;

	std::vector<float> wedge_uvs;
	std::vector<float> wedge_norms;
	std::vector<int>   tex_number; //per face texture number

	std::vector<Group> groups;
	std::map<std::string, std::string> exif;

protected:
	void addNormals();
};

} //namespace
#endif // CRT_MESHLOADER_H

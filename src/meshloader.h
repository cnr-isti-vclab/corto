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

	bool load(const std::string &filename);
	bool loadPly(const std::string &filename);
	bool loadObj(const std::string &filename);
	void splitWedges();
	bool savePly(const std::string &filename, std::vector<std::string> &comments);


	uint32_t nface;
	uint32_t nvert;
	std::vector<float> coords;
	std::vector<float> norms;
	std::vector<float> uvs;
	std::vector<float> radiuses;
	std::vector<uint8_t> colors;

	std::vector<uint32_t> index;

	std::vector<float> wedge_uvs;
	std::vector<float> wedge_norms;
	std::vector<int>   tex_number; //per face texture number

	std::vector<Group> groups;
	std::map<std::string, std::string> exif;
};

} //namespace
#endif // CRT_MESHLOADER_H

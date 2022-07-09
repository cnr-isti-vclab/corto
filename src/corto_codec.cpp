#include "corto_codec.h"

namespace crt
{
    // TODO: Use a .json instead of Vector2
    Decoder* CreateDecoder(int length, unsigned char* data, Vector2* decoderInfo)
    {
        Decoder* decoder = new Decoder(length, data);
        
        // We can't manipulate the C++ data in C# so we have to send if that way
        Vector2& info = decoderInfo[0];
        info.x = decoder->nface;
        info.y = decoder->nvert;
        
        return decoder;
    }

    void DestroyDecoder(Decoder* decoder)
    {
        delete decoder;
    }

    // TODO : Create a set  of wrappers for the set attributes functions to support additional data
    int DecodeMesh(Decoder* decoder, Vector3* vertices, int* indices, Vector3* normals, Color* colors, Vector2* texcoord)
    {
        // Unity does not support point clouds for the moment
        if (decoder->nface == 0)
        {
            return -1;
        }
        else
        {
            decoder->setIndex((uint32_t*)(indices));
        }

        if (decoder->nvert > 0)
        {
            decoder->setPositions((float*)vertices);
        }

        if (decoder->data.count("normal") > 0)
        {
            decoder->setNormals((float*)normals);
        }

        if (decoder->data.count("color") > 0)
        {
            decoder->setAttribute("color", (char*)colors, VertexAttribute::FLOAT);
        }

        if (decoder->data.count("uv") > 0)
        {
            decoder->setUvs((float*)texcoord);
        }

        decoder->decode();

        return decoder->nface;
    }

    int GroupsSize(Decoder *decoder) {
		return decoder->index.groups.size();
	}
	Group *GetGroup(Decoder *decoder, int i) {
		return &(decoder->index.groups[i]);
	}
	int GroupEnd(Group *group) { return group->end; }
	int GroupPropertiesSize(Group *group) { return group->properties.size(); }
	void GetGroupProperty(Group *group, int i, char *key, char *value) {
		auto it = group->properties.begin();
		std::advance (it, i);
		memcpy(key, it->first.c_str(), it->first.size()+1);
		memcpy(value, it->second.c_str(), it->second.size()+1);
	}
	int GetMTLSize(Decoder *decoder) {
		if(decoder->exif.count("mtl"))
			return decoder->exif["mtl"].size() + 1;
	}
	void GetMTL(Decoder *decoder, char *mtl) {
		mtl[0] = 0;
		if(decoder->exif.count("mtl"))
			memcpy(mtl, decoder->exif["mtl"].c_str(), decoder->exif["mtl"].size() + 1);
	}

/*    void GetGroup(int index, int *end, int *nproperties, int *keys, int *values, char *buffer) {
		Group &group = decoder->index.groups[index];
		*end = group.end;
		*nproperties = group.properties.size();
		int count = 0;
		int offset = 0;
		for(auto &it : group.properties) {
			keys[count] = offset;

			int size = it->first.size() + 1;
			memcpy(buffer + offset, it->first.c_str(), size);
			offset += size;

			values[count] = offset;
			size = it->second.size() + 1;
			memcpy(buffer + offset, it->second.c_str(), size);
			offset += size;
		}
    }*/
}

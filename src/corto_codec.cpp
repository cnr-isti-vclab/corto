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
}
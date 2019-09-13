#ifndef CORTO_CODEC_H
#define CORTO_CODEC_H

#include "decoder.h"
#include <string.h>

// If compiling with Visual Studio
#if defined(_MSC_VER)
#define EXPORT_API __declspec(dllexport)
#else
// Other platforms don't need this
#define EXPORT_API
#endif

namespace crt
{
    extern "C"
    {
        struct Color
        {
            float r;
            float g;
            float b;
            float a;
        };

        struct Vector2
        {
            float x;
            float y;
        };

        struct Vector3
        {
            float x;
            float y;
            float z;
        };

        // TODO : Use .json instead of Vector2*
        Decoder EXPORT_API *CreateDecoder(int length, unsigned char* data, Vector2* decoderInfo);
        void EXPORT_API DestroyDecoder(Decoder* decoder);
        int EXPORT_API DecodeMesh(Decoder* decoder, Vector3* vertices, int* indices, Vector3* normals, Color* colors, Vector2* texcoord);
    }
}
#endif // !CORTO_UNITY_PLUGIN_H
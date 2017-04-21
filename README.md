<p><img width="144" src="http://vcg.isti.cnr.it/corto/img/logo.svg"></p>

**Corto** is a library for compression and decompression meshes and point clouds (C++ and Javascript).

## [Demo](http://vcg.isti.cnr.it/corto/examples.html)

## Features

Corto compression library supports point clouds and meshes with per vertex attributes: normals, colors,
texture coordinates and custom attributes.

The main focus is on decompression speed, see [performances](#performances), both for C++ lib and javascript.

[corto](#corto) is program to compress .ply and .obj models, [corto.js](#corto.js) a javascript library for WebGL applications and [CORTOLoader](#cortoloader, a threejs loader.

This work is based on the compression algorithm developed for the 
[Nexus](https://github.com/cnr-isti-vclab/nexus) project for multiresolution for creation
and visualization models.

## Performances


## Building 

make Makefile
cmake 
or qtcreator

## Tools

### corto

Usage: 

corto file.ply

(options for bit or quantization step)

### CORTOLoader
Threejs loader (mtl optional

### corto.js

### cortolib

Encoder...
addCoords(....)
addNormals
addAttribute

in place decoding provide buffers

Decoder
(get nvert nface groups)
setCoords



# Corto 
<svg height="50.574291" width="77.870079" xmlns="http://www.w3.org/2000/svg">
 <g transform="translate(-111.57769 -260.11672)">
  <path d="m127.58 289.55 41.52-.13s-3.83 7-6.94 8.52c-27.7 0-.1.01-27.7 0-4.46-3.33-6.88-8.39-6.88-8.39z" fill="#75501c"/>
  <g fill="none" stroke="#000" stroke-linejoin="round">
   <path d="m167.73 305.19h-38.17s-10.97-2.41-12.48-7.21h63.13c-1.52 4.8-12.48 7.21-12.48 7.21z"/>
   <path d="m134.45 297.94c-6.29-4.42-12.16-16.16-12.16-32.32h52.02c0 16.16-5.87 27.91-12.16 32.32"/>
   <path d="m174.1 270.88c4.39 0-1.99.01 3.43.01 4.66 0 6.31 3.88 6.41 5.87.11 2.2-1 4.84-3.32 6.11-2.32 1.27-6.27 2.63-10.15 3.59"/>
   <path d="m127.58 289.55 41.52-.13"/>
  </g>
  <text font-family="Calibri" font-size="17" x="130" y="286">corto</text>
 </g>
</svg>

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



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

Bunny mesh 34K vertices (12 bits precision) courtesy of [Stanford](http://graphics.stanford.edu/data/3Dscanrep/)

| Bunny           | Corto   | Draco  | OpenCTM |
| -------------   | ------:| ------:| -------:|
| Size            | 62.8KB | 63.6KB |   168KB |
| C++ decode      |  2.3ms |   25ms* |    18ms* |
| Js first run    |   27ms |  495ms |   113ms |
| Js 10th run     |  6.5ms |   30ms |    52ms |
| Js lib size     |   26KB |  673KB |    17KB |


Proserpina mesh 128K vertices (14 bits), textures (12 bits), normals (10 bits) courtesy of [egiptologo91 in Sketchfab](https://sketchfab.com/models/dd671b1fc15c481b8592284e155cd8cb)

| Proserpina      | Corto   | Draco  | OpenCTM |
| -------------   | -------:| ------:| -------:0
|                 |   982KB |  779KB |  1.13MB |
| C++ decode      |    23ms | 220ms* |  100ms* |
| Js first run    |   100ms | 1120ms |   420ms |
| JS 10th run     |    60ms |  490ms |   330ms |


Palmyra point clout 500K vertices (18 bits), textures (12 bits) courtesy of [robotgoat in Sketchfab](https://sketchfab.com/models/13227e6ed7c44bdd9af2870dc68ca63e)

| Palmyra         | Corto   | Draco  |
| -------------   | -------:| ------:|
|                 |   282KB |  301KB |
| C++ decode      |    28ms | 644ms* |
| Js first run    |    110ms | 2400ms |
| JS 10th run     |    65ms | 1600ms |


* C++ timing might depend on compilation flags

OpenCTM larger size is due mainly to simpler connectivity compression, Draco Javascript lib size is due to emscripten.

## Building 

Using g++

	make -f Makefile

Using qt:

	qmake corto.pro
	make

Using cmake (todo)

cmake ./
make


## Tools

### corto

This program creates the .crt compressed model.

Usage: 

	corto [options] file

	FILE is the path to a .ply or a .obj 3D model.
		-o <output>: filename of the .crt compressed file.
		             if not specified the extension of the input file will be replaced.
		-e <key=value>: add an exif property, or more than one.
		-p : treat the input as a point cloud."
		-v <bits>: vertex bits quantization. If not specified an euristic is used
		-n <bits>: normal bits quantization. Default 10.
		-c <bits>: color bits quantization. Default 6.
		-u <bits>: texture coordinate bits. Default 10.
		-q <step>: quantization step unit (float) instead of bits for vertex coordinates
		-N <prediction>: normal prediction can be:
		                 delta: use difference from previous normal (fastest)
		                 estimated: use difference from compute normals (cheaper)
		                 border: store difference only for boundary vertices (cheapest)
		-P <file.ply>: decompress and save as .ply for debugging purpouses

Material groups for obj (newmtl) and ply with texnumbers are preserved into the crt model.

### CORTOLoader

CORTOLoader.js is similar to [THREE.OBJLoader](https://threejs.org/docs/index.html#examples/loaders/OBJLoader) in functionality, and can easily replace it in three.js.

	var loader = new THREE.CORTOLoader({ path: path });  
	//materials autocreated, pass option loadMaterial: false otherwise.

	loader.load(model, function(mesh) {
		mesh.addEventListener("change", render);         //if textures could be needed for materials

		if(!mesh.geometry.attributes.normal)
			mesh.geometry.computeVertexNormals();

		scene.add(mesh); 
		render();
	});

See demo.html for details.

### corto.js

CortoDecoder decodes a .crt  model

	var request = new XMLHttpRequest();
	request.open('GET', 'bun_zipper.crt');
	request.responseType = 'arraybuffer';
	request.onload = function() {
		var decoder = new CortoDecoder(this.response);
		var model = decoder.decode();
		console.log(model.nvert, model.nface, model.groups);
		console.log('Index: ', model.index);
		console.log('Positions: ', model.position);
		console.log('Colors: ', model.color);
		console.log('Nornmals: ', model.normal);
		console.log('Tex coords: ', model.uv);
		//custom attributes can be encoded, see cortolib below for details.
	}
	request.send();

### cortolib

Encoder...
addCoords(....)
addNormals
addAttribute

in place decoding provide buffers

Decoder
(get nvert nface groups)
setCoords



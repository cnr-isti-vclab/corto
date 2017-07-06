<p><img width="128" src="http://pc-ponchio.isti.cnr.it/corto/img/logo1.svg"></p>

**Corto** is a library for compression and decompression meshes and point clouds (C++ and Javascript).

## [Demo](http://vcg.isti.cnr.it/corto/index.html)

## Features

Corto compression library supports point clouds and meshes with per vertex attributes: normals, colors,
texture coordinates and custom attributes.

The main focus of this work is on decompression speed, see [performances](#performances), both for C++ lib and javascript,
while still provide acceptable compression rates.

* [corto](#corto) is program to compress **.ply** and **.obj** models
* [libcorto](#libcorto) is a C++ compression/decompression library
* [corto.js](#corto_js) a javascript library for .crt decompression
* [CORTOLoader](#cortoloader), a threejs loader

This work is based on the compression algorithm developed for the 
[Nexus](https://github.com/cnr-isti-vclab/nexus) project for creation
and visualization of multiresolution models. See [Fast decompression for web-based view-dependent 3D rendering](http://vcg.isti.cnr.it/Publications/2015/PD15/).

Entropy compression is based on [Tunstall](#tunstall) coding, decompression require only table lookup and is very fast while
similar in compression ratio to Huffman where the number of symbols is small.


## Performances

Decompression timing and size for a few models tested on a somewhat dated pc. (Intel Core i5-3450 @ 3.10GHz).
Jvascript decompression time drops dramatically for repeadted run of complex algorithms, hence the 10th run result.

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
| -------------   | -------:| ------:| -------:|
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


* C++ timing might be somewhat affected by compilation flags
* OpenCTM larger size is due mainly to simpler connectivity compression, 
* Draco Javascript lib size is due to emscripten.

## Building 

The program and the library have no dependencies, g++ on linux and VisualStudio on Windows have been tested,
but it should work everywhere with slight modifications.

Using g++

	make -f Makefile.linux

Using qt:

	qmake corto.pro
	make

Using cmake:

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

### corto_js

CortoDecoder decodes a .crt as an arraybuffer and returns an objects with attributes (positions, index, colors etc).

	<script src="js/corto.js"/>
	<script>
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
	</script>

### libcorto

Interface is not entirely stable, no mayor change is expected.
See src/main.cpp for an extensive example.


	std::vector<float> coords;
	std::vector<uint32_t> index;
	std::vector<float> uv;
	std::vector<float> radius;
	
	//fill data arrays...
	
	crt::Encoder encoder(nvert, nface);
	
	//add attributes to be encoded
	encoder.addPositions(coords.data(), index.data(), vertex_quantization_step);
	encoder.addUvs(uvs.data(), pow(2, -uv_bits));
	
	//add custom attributes
	encoder.addAttribute("radius", (char *)radiuses.data(), crt::VertexAttribute::FLOAT, 1, 1.0f);
	
	const char *compressed_data = encoder.stream.data();
	const uint32_t compressed_size = encoder.stream.size();

Decoding

	crt::Decoder decoder(size, data);
	
	//allocate memory if needed
	coords.resize(decoder.nvert*3);
	index.resize(decoder.nface*3);
	
	//tell the decoder where to decompress data
	decoder.setPositions(coords.data());
	
	if(decoder.data.count("uv")) {
		out.uvs.resize(decoder.nvert*2);
		decoder.setUvs(out.uvs.data());
	}
	//actually decode
	decoder.decode();


### Tunstall

TODO.

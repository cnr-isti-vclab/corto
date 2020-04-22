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
* [corto.em.js](#corto_em) an emscripten library for .crt decompression (NEW! faster)
* [CORTOLoader](#cortoloader), a threejs loader

This work is based on the compression algorithm developed for the 
[Nexus](https://github.com/cnr-isti-vclab/nexus) project for creation
and visualization of multiresolution models. See [Fast decompression for web-based view-dependent 3D rendering](http://vcg.isti.cnr.it/Publications/2015/PD15/).

Entropy compression is based on [Tunstall](#tunstall) coding, decompression require only table lookup and is very fast while
similar in compression ratio to Huffman where the number of symbols is small.

The C++ code is released under GPL licence, the Javascript code under MIT licence.

## Performances

Decompression timing and size for a few models tested on a midlevel notebook. (Intel® Core™ i7-8550U CPU @ 1.80GHz × 8 ).
Things changed from the last time I made some timings: emscripten (and draco) improved a lot, corto is now available in emscritpem too.

Corto library has a compression level settings from 1 (lower compression, faster) to 10 (higher compression, slower decompression) with 7 being the default.

Bunny mesh 34K vertices (14 bits precision) courtesy of [Stanford](http://graphics.stanford.edu/data/3Dscanrep/)

| Bunny               | Corto  | Draco cl 1 | Draco cl 7 |
| -------------       | ------:|     ------:|  --------: |
| Size                | 95.8KB |    122.0KB |    82.3KBH |
| C++ decode          |    2ms |       7ms  |        9ms |
| Js Chrome           |  4.6ms |       10ms |       13ms |  
| Js Firefox          |    8ms |       12ms |       18ms |  
| Js lib size (gz)    |   28KB |      268KB |      268KB |  


Proserpina mesh 128K vertices (14 bits), textures (12 bits), normals (10 bits) courtesy of [egiptologo91 in Sketchfab](https://sketchfab.com/models/dd671b1fc15c481b8592284e155cd8cb)

| Proserpina      | Corto   | Draco cl 1  | Draco cl 7 |
| -------------   | -------:| ------:| -------:|
|                 |   872KB | 1080KB |   672MB |
| C++ decode      |    18ms |  62ms* |  170ms* |
| Js Chrome       |    31ms |   62ms |   280ms |
| JS Firefox      |    41ms |   88ms |   280ms |



The Nile - Vatican Museums point cloud 167K vertices (14bits), textures (12 bits), normals(10 bits) courtesy of [egiptologo91 in Sketchfab](https://sketchfab.com/3d-models/the-nile-de5ecd487d194890b2af093428aa5ca2)


| Nile           | Corto   | Draco cl 1 | Draco cl 7 |
| -------------   | -------:| ------:| ------------:|
|                 |   890KB |  1.92MB | 1.70MB |
| C++ decode      |     7ms | 43ms    | 81ms |
| Js Chrome       |    18ms | 104ms   | 118ms |
| JS Firefox      |    32ms | 123ms  |  128ms |


* C++ timing might be affected by compilation flags


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

### corto_em

CortoDecoder decodes a .crt as an arraybuffer and returns an objects with attributes (positions, index, colors etc).
The interface is slightly different. No decode object, just a decode function, with support for 16 bit indexes and normals.

	<script src="js/corto.js"/>
	<script>
	var request = new XMLHttpRequest();
	request.open('GET', 'bun_zipper.crt');
	request.responseType = 'arraybuffer';
	request.onload = function() {
		var shortIndex = false;
		var shortNormals = true;
		var model = CortoDecoder.decode(this.response, shortIndex, shortNormals);
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
See `src/main.cpp` for an extensive example.


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

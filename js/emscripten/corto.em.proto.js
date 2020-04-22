/*
Corto
Copyright (c) 2017-2020, Visual Computing Lab, ISTI - CNR
All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

onmessage = async function(job) {
	if(typeof(job.data) == "string") return;

	var buffer = job.data.buffer;
	if(!buffer) return;
	if(!CortoDecoder.instance)
		await CortoDecoder.ready;

	var model = CortoDecoder.decode(buffer, job.data.short_index, job.data.short_normals, job.data.rgba_colors ? 4 : 3);
	postMessage({ model: model, buffer: buffer, request: job.data.request});
};

var CortoDecoder = (function() {
	"use strict";
	var wasm_base = "BASE64";
/*	var wasm_simd = "";
	var detector = new Uint8Array([0,97,115,109,1,0,0,0,1,4,1,96,0,0,3,3,2,0,0,5,3,1,0,1,12,1,0,10,22,2,12,0,65,0,65,0,65,0,252,10,0,0,11,7,0,65,0,253,4,26,11]);

	var wasm = wasm_base;

	if (WebAssembly.validate(detector)) {
		wasm = wasm_simd;
		console.log("Warning: corto is using experimental SIMD support");
	}*/
	var instance, heap;

	var env = {
		emscripten_notify_memory_growth: function(index) {
			heap = new Uint8Array(instance.exports.memory.buffer);
		},
		proc_exit:  function(rval) { return 52; }, //WASI_ENOSYS
	};

	function unhex(data) {
		var bytes = new Uint8Array(data.length / 2);
		for (var i = 0; i < data.length; i += 2) {
			bytes[i / 2] = parseInt(data.substr(i, 2), 16);
		}
		return bytes.buffer;
	}

	var promise =
		WebAssembly.instantiate(unhex(wasm_base), { env:env, wasi_unstable:env })
		.then(function(result) {
			instance = result.instance;
			instance.exports._start();
			env.emscripten_notify_memory_growth(0);
		});

	function pad() {
		return;
		var s = instance.exports.sbrk(0);
		var t = s & 0x3;
		if(t)
			instance.exports.sbrk(4 -t);
	}

	function decode(source, shortIndex = false, shortNormal = false, colorComponents = 4) {
		if(!source.length)
			source = new Uint8Array(source);
		var len = source.length;
		var exports = instance.exports;
		var sbrk = exports.malloc; //yes I am a criminal.
		var free = exports.free;


//copy source to heap. we could use directly source, but that is good only for the first em call.


		//We could use the heap instead of malloc, don't know cost (not much probably), and can't debug properly.
		//set initial heap position, to be restored at the end of the deconding.
		var pos = sbrk(0);

		var sptr = sbrk(len);
		heap.set(source, sptr);

		var decoder = exports.newDecoder(len, sptr);
		var nvert = exports.nvert(decoder);
		var nface = exports.nface(decoder);

		var geometry = {
			nvert: nvert,
			nface: nface,
		}

		var ngroups = exports.ngroups(decoder);
		if(ngroups > 0) {
			pad();
			var gp = sbrk(4*ngroups);
			exports.groups(decoder, gp);
			geometry.groups =  new Uint32Array( ngroups*4);
			geometry.grous.set(gp);
			free(gp);
		}

		var hasNormal = exports.hasNormal(decoder);
		var hasColor = exports.hasColor(decoder);
		var hasUv = exports.hasUv(decoder);

		var iptr = 0, pptr = 0, nptr = 0, cptr = 0, uptr = 0;
		if(nface) {
			pad();  //memory align needed for int, short, floats arrays if using sbrk
			if(shortIndex) {
				iptr = sbrk(nface * 6);
				exports.setIndex16(decoder, iptr);
			} else {
				iptr = sbrk(nface * 12);
				exports.setIndex32(decoder, iptr);
			}
		}

		pptr = sbrk(nvert * 12);
		exports.setPositions(decoder, pptr);

		if(hasUv) {
			pad();
			uptr = sbrk(nvert * 8);
			exports.setUvs(decoder, uptr);
		}

		if(hasNormal) {
			pad();
			if(shortNormal) {
				nptr = sbrk(nvert * 6);
				exports.setNormals16(decoder, nptr);
			} else {
				pptr = sbrk(nvert * 12);
				exports.setNormals32(decoder, nptr);
			}
		}

		if(hasColor) {
			pad();
			cptr = sbrk(nvert * colorComponents);
			exports.setColors(decoder, cptr, colorComponents);
		}
		pad();
		exports.decode(decoder);


		//typed  arrays needs to be created in javascript space, not as views of heap (next call will overwrite them!)
		//hence the double typed array.
		if(nface) {
			if(shortIndex)
				geometry.index = new Uint16Array(new Uint16Array(heap.buffer, iptr, nface*3));
			else
				geometry.index = new Uint32Array(new Uint32Array(heap.buffer, iptr, nface*3));
		}

		geometry.position = new Float32Array(new Float32Array(heap.buffer, pptr, nvert*3));

		if(hasNormal) {
			if(shortNormal)
				geometry.normal = new Int16Array(new Int16Array(heap.buffer, nptr, nvert*3));
			else
				geometry.normal = new Float32Array(new Float32Array(heap.buffer, nptr, nvert*3));
		}

		if(hasColor)
			geometry.color = new Uint8Array(new Uint8Array(heap.buffer, cptr, nvert*colorComponents));

		if(hasUv)
			geometry.uv = new Float32Array(new Float32Array(heap.buffer, uptr, nvert*2));

		exports.deleteDecoder(decoder);

		//restore heap position if using sbrk
		//sbrk(pos - sbrk(0)); 
		free(sptr);
		if(iptr) free(iptr);
		if(pptr) free(pptr);
		if(cptr) free(cptr);
		if(nptr) free(nptr);
		if(uptr) free(uptr);

		return geometry;
	};

	return {
		ready: promise,
		decode: decode
	};
})();

if (typeof exports === 'object' && typeof module === 'object')
	module.exports = CortoDecoder;
else if (typeof define === 'function' && define['amd'])
	define([], function() {
		return CortoDecoder;
	});
else if (typeof exports === 'object')
	exports["CortoDecoder"] = CortoDecoder;

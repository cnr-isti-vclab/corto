// This file is part of corto library and is distributed under the terms of MIT License.
// Copyright (C) 2019-2020, by Federico Ponchio (ponchio@gmail.com)


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
        let exports = instance.exports;
        let sbrk = exports.malloc; //yes I am a criminal.
        let free = exports.free;

        if(!source.length)
            source = new Uint8Array(source);
        let data_size = source.length;
        let sptr = sbrk(data_size);
        var pos = sbrk(0);
        heap.set(source, sptr);

        let decoder = exports.newDecoder(data_size, sptr);
        let geometry = {
            nvert: exports.nvert(decoder),
            nface: exports.nface(decoder),
            hasNormal: exports.hasNormal(decoder),
            hasColor: exports.hasColor(decoder),
            hasUv: exports.hasUv(decoder),
            iptr: null,
            pptr: null,
            nptr: null,
            cptr: null,
            uptr: null,
            dispose: function () {
                if(this.iptr) free(this.iptr);
                if(this.pptr) free(this.spptr);
                if(this.cptr) free(this.cptr);
                if(this.nptr) free(this.nptr);
                if(this.uptr) free(this.uptr);
            }
        }

        let ngroups = exports.ngroups(decoder);
        if(ngroups > 0) {
            pad();
            var gp = sbrk(4*ngroups);
            exports.groups(decoder, gp);
            geometry.groups =  new Uint32Array( ngroups*4);
            geometry.grous.set(gp);
            free(gp);
        }

        if(geometry.nface) {
            pad();  //memory align needed for int, short, floats arrays if using sbrk
            if(shortIndex) {
                geometry.iptr = sbrk(geometry.nface * 6);
                exports.setIndex16(decoder, iptr);
            } else {
                geometry.iptr = sbrk(geometry.nface * 12);
                exports.setIndex32(decoder, geometry.iptr);
            }
        }

        geometry.pptr = sbrk(geometry.nvert * 12);
        exports.setPositions(decoder, geometry.pptr);

        if(geometry.hasUv) {
            pad();
            geometry.uptr = sbrk(geometry.nvert * 8);
            exports.setUvs(decoder, geometry.uptr);
        }

        if(geometry.hasNormal) {
            pad();
            if(shortNormal) {
                geometry.nptr = sbrk(geometry.nvert * 6);
                exports.setNormals16(decoder, geometry.nptr);
            } else {
                geometry.nptr = sbrk(geometry.nvert * 12);
                exports.setNormals32(decoder, geometry.nptr);
            }
        }

        if(geometry.hasColor) {
            pad();
            geometry.cptr = sbrk(geometry.nvert * colorComponents);
            exports.setColors(decoder, geometry.cptr, colorComponents);
        }
        pad();

        exports.decode(decoder);

        if(geometry.nface) {
            if(shortIndex)
                geometry.index = new Uint16Array(new Uint16Array(heap.buffer, geometry.iptr, geometry.nface*3));
            else
                geometry.index = new Uint32Array(new Uint32Array(heap.buffer, geometry.iptr, geometry.nface*3));
        }

        geometry.vertex = new Float32Array(new Float32Array(heap.buffer, geometry.pptr, geometry.nvert*3));

        if(geometry.hasNormal) {
            if(shortNormal)
                geometry.normals = new Int16Array(new Int16Array(heap.buffer, geometry.nptr, geometry.nvert*3));
            else
                geometry.normal = new Float32Array(new Float32Array(heap.buffer, geometry.nptr, geometry.nvert*3));
        }

        if(geometry.hasColor)
            geometry.color = new Uint8Array(new Uint8Array(heap.buffer, geometry.cptr, geometry.nvert*colorComponents));

        if(geometry.hasUv)
            geometry.uv = new Float32Array(new Float32Array(heap.buffer, geometry.uptr, geometry.nvert*2));

        exports.deleteDecoder(decoder);
        free(sptr);

        return geometry;
    };

    return {
        ready: promise,
        decode: decode,
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

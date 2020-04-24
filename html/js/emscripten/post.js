Module["decode"] = function(len, ptr) {
	var bptr = Module._malloc(len);
	Module.HEAPU8.set(ptr, bptr);

	var decoder = Module.ccall('newDecoder', 'number', ['number', 'number'], [len, bptr]);
	var nvert = Module.ccall('nvert', 'number', ['number'], [ decoder]);
	var nface = Module.ccall('nface', 'number', ['number'], [ decoder]);

	var geometry = {
		nvert: nvert,
		nface: nface,
	}
	var ngroups = Module.ccall('ngroups', 'number', ['number'], [ decoder]);
	if(ngroups > 0) {
		var groups = Module._malloc(ngroups*4);	
		Module.ccall('groups', null, ['number', 'number'], [ decoder, groups]);
		geometry.groups =  new Uint32Array(new Uint32Array(Module.HEAPU8.buffer, groups, ngroups*4));
		Module._free(groups);
	}

/* read attribute strings like this:
	readArray: function(n) {
		var a = this.buffer.subarray(this.pos, this.pos+n);
		this.pos += n;
		return a;
	},

	readString: function() {
		var n = this.readShort();
		var s = String.fromCharCode.apply(null, this.readArray(n-1));
		this.pos++; //null terminator of string.
		return s;
	},
*/



	var pptr      = Module._malloc(nvert*12);
	Module.ccall('setPositions', null, ['number', 'number'], [decoder, pptr]);

	var iptr = 0;
	if(nface) {
		iptr = Module._malloc(nface * 12);
		Module.ccall('setIndex32', null, ['number', 'number'], [decoder, iptr]);
	}


	var nptr = 0;
	var hasNormal = Module.ccall('hasAttr', 'number', ['number', 'string'], [decoder, 'normal']);
	if(hasNormal) {
		nptr = Module._malloc(nvert*12);
		Module.ccall('setNormals32', null, ['number', 'number'], [decoder, nptr]);
	}
	
	var cptr = 0;
	var hasColor = Module.ccall('hasAttr', 'number', ['number', 'string'], [decoder, 'color']);
	if(hasColor) {
		cptr = Module._malloc(nvert*4);
		Module.ccall('setColors', null, ['number', 'number'], [decoder, cptr]);
	}

	var uptr = 0;
	var hasUv = Module.ccall('hasAttr', 'number', ['number', 'string'], [decoder, 'uv']);
	if(hasUv) {
		uptr = Module._malloc(nvert*8);
		Module.ccall('setUvs', null, ['number', 'number'], [decoder, uptr]);
	}

	Module.ccall('decode', null, ['number'], [decoder]);

	//double Float32Array is used to make a copy, the first is a view
	geometry.position = new Float32Array(new Float32Array(Module.HEAPU8.buffer, pptr, nvert*3))

	Module._free(bptr);
	Module._free(pptr);

	if(nface) {
		geometry.index = new Uint32Array(new Uint32Array(Module.HEAPU8.buffer, iptr, nface*3))
		Module._free(iptr);
	}
	if(hasColor) {
		if(cptr) Module._free(cptr);
		geometry.color = new Uint8Array(new Uint8Array(Module.HEAPU8.buffer, cptr, nvert*4));
	}
	if(hasNormal) {
		geometry.normal = new Float32Array(new Float32Array(Module.HEAPU8.buffer, nptr, nvert*3));
		if(nptr) Module._free(nptr);
	}
	if(hasUv) {
		if(uptr) Module._free(uptr);
		geometry.uv = new Float32Array(new Float32Array(Module.HEAPU8.buffer, uptr, nvert*2));
	}

	Module.ccall('deleteDecoder', null, ['number'], [decoder]);
	return geometry; 
};




/*
Nexus

Copyright(C) 2012 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (http://www.gnu.org/licenses/gpl.txt)
for more details.
*/

function CortoDecoder(data) {
	var t = this;
	var stream = t.stream = new Stream(data);
	
	var magic = stream.readInt();
	if(magic != 2021286656) return;

	var version = stream.readInt();
	t.entropy = stream.readUChar();
	//exif
	t.exif = {};
	var n = stream.readInt();
	for(var i = 0; i < n; i++) {
		var key = stream.readString();
		t.exif[key] = stream.readString();
	}

	//attributes
	var n = stream.readInt();
	t.geometry = {};
	t.attributes = {};
	for(var i = 0; i < n; i++) {
		var a = {};
		var name = stream.readString();
		var codec = stream.readInt();
		var q = stream.readFloat();
		var components = stream.readUChar(); //internal number of components
		var type = stream.readUChar();     //default type (same as it was in input), can be overridden
		var strategy = stream.readUChar();
		var attr;
		switch(codec) {
		case 2: attr = NormalAttr; break;
		case 3: attr = ColorAttr; break;
		case 1: //generic codec
		default: attr = Attribute; break;
		}
		t.attributes[name] = new attr(name, q, components, type, strategy);
	}

//TODO move this vars into an array.
	t.geometry.nvert = t.nvert = t.stream.readInt();
	t.geometry.nface = t.nface = t.stream.readInt();
}

CortoDecoder.prototype = {

decode: function() {
	var t = this;

	t.last = new Uint32Array(t.nvert*3); //for parallelogram prediction
	t.last_count = 0;

	for(let i in t.attributes)
		t.attributes[i].init(t.nvert, t.nface);

	if(t.nface == 0)
		t.decodePointCloud();
	else
		t.decodeMesh();

	return t.geometry;
},

decodePointCloud: function() {
	var t = this;
	for(var i in t.attributes) {
		var a = t.attributes[i];
		a.decode(t.nvert, t.stream);
		a.deltaDecode(t.nvert);
		a.dequantize(t.nvert);
		t.geometry[a.name] = a.buffer;
	}
},

decodeMesh: function() {
	var t = this;
	t.index = new IndexAttr(t.nvert, t.nface, 0); //USE
	t.index.decode(t.stream);

	t.vertex_count = 0;
	var start = 0;
	t.cler = 0;
	for(var p = 0; p < t.index.groups.length; p++) {
		var end = t.index.groups[p].end;
		this.decodeFaces(start *3, end *3);
		start = end;
	}
	t.geometry['index'] = t.index.faces;
	t.geometry.groups = t.index.groups;
	for(var i in t.attributes) 
		t.attributes[i].decode(t.nvert, t.stream);
	for(var i in t.attributes) 
		t.attributes[i].deltaDecode(t.nvert, t.index.prediction);
	for(var i in t.attributes) 
		t.attributes[i].postDelta(t.nvert, t.nface, t.attributes, t.index);
	for(var i in t.attributes) { 
		var a = t.attributes[i];
		a.dequantize(t.nvert);
		t.geometry[a.name] = a.buffer;
	}
},

decodeNormals: function() {
	var norm_q = this.stream.readChar();

	var dtunstall = new Tunstall;
	var diffs = dtunstall.decompress(this.stream);

	var stunstall = new Tunstall;
	var signs = stunstall.decompress(this.stream);
	var bitstream = this.stream.readBitStream();

	var side = (1<<(16 - norm_q))>>>0;
	var diffcount = 0;
	var signcount = 0;

	if(!this.sig.indices) {
		for(var k = 0; k < 2; k++) {
			var on = 0;
			for(var i = 0; i < this.node.nvert; i++) {
				var d = this.decodeDiff(diffs[diffcount++], bitstream);
				on = on + d;
				this.normals[3*i + k] = on*side;
			}
		}
		for(var i = 0; i < this.node.nvert; i++) {
			var offset = i*3;
			var x = this.normals[offset + 0];
			var y = this.normals[offset + 1];
			var z = 32767.0*32767.0 - x*x - y*y;

        	if(z < 0) z = 0;
        	z = Math.sqrt(z);
			if(z > 32767) z = 32767;
			if(signs[i] == 0)
				z = -z;
			this.normals[offset + 2] = z;
		}
		return;
	}

	var boundary = this.markBoundary();
	this.computeNormals();

	if(this.sig.texcoords) //hack, fixing normals makes it worse actually
		return;

	var stat = 0;
	//get difference between original and predicted
	for(var i = 0; i < this.node.nvert; i++) {
		if(!boundary[i]) continue;
		var offset = i*3;
		var x = (this.normals[offset + 0]/side);
		var y = (this.normals[offset + 1]/side);
		var dx = this.decodeDiff(diffs[diffcount++], bitstream);
		var dy = this.decodeDiff(diffs[diffcount++], bitstream);
		x = (x + dx)*side;
		y = (y + dy)*side;

        var z = 32767.0*32767.0 - x*x - y*y;

        if(z < 0) z = 0;
        z = Math.sqrt(z);
        //sign
        if(z > 32767.0) z = 32767.0;
        var signbit = signs[signcount++];
//        if(this.normals[offset+2] < 0 != signbit)
        if((this.normals[offset+2] < 0 && signbit == 0) || (this.normals[offset+2] > 0 && signbit == 1))
        	z = -z;
		this.normals[offset + 0] = x;
		this.normals[offset + 1] = y;
        this.normals[offset + 2] = z;
	}
},

decodeColors: function() {
	var color_q = [];
	for(var k = 0; k < 4; k++)
		color_q[k] = this.stream.readChar();

	var diffs = [];
	for(var k = 0; k < 4; k++) {
		var tunstall = new Tunstall;;
		diffs[k] = tunstall.decompress(this.stream);
	}
	var bitstream = this.stream.readBitStream();

	var count = 0;
	if(this.sig.indices) {
		for(var i = 0; i < this.node.nvert; i++) {
			var last  = this.last[i]*4;
			var offset = i*4;

			for(var k = 0; k < 4; k++) {
				var c = this.decodeDiff(diffs[k][count], bitstream);

				if(last >= 0)
					c += this.colors[last + k];
				this.colors[offset] = c;
				offset++;
			}
			count++;
		}
	} else {
		for(var k = 0; k < 4; k++)
			this.colors[k] = this.decodeDiff(diffs[k][count], bitstream); 
		count++;

		var offset = 4;
		for(var i = 1; i < this.node.nvert; i++) {
			for(var k = 0; k < 4; k++) {
				var d = this.decodeDiff(diffs[k][count], bitstream); 
				this.colors[offset] = this.colors[offset-4] + d;
				offset ++;
			}
			count++;
		}
	}

	var steps = [];
	for(var k = 0; k < 4; k++)
		steps[k] = (1<<(8 - color_q[k]));

	//convert to rgb
	for(var i = 0; i < this.node.nvert; i++) {
		var offset = i*4;

		var e0 = this.colors[offset + 0] * steps[0];
		var e1 = this.colors[offset + 1] * steps[1];
		var e2 = this.colors[offset + 2] * steps[2];

		this.colors[offset + 0] = (e2 + e0)&0xff;
		this.colors[offset + 1] = e0;
		this.colors[offset + 2] = (e1 + e0)&0xff;
	}
},

//how to determine if a vertex is a boundary without topology:
//for each edge a vertex is in, add or subtract the id of the other vertex depending on order
//for internal vertices sum is zero.
//unless we have strange configurations and a lot of sfiga, zero wont happen. //TODO think about this
markBoundary: function() {
//	var boundary = new Uint8Array(this.node.nvert);
	var count = new Uint32Array(this.node.nvert);

	var offset = 0;
	for(var i = 0; i < this.node.nface; i++) {
		count[this.faces[offset + 0]] += this.faces[offset + 1] - this.faces[offset + 2];
		count[this.faces[offset + 1]] += this.faces[offset + 2] - this.faces[offset + 0];
		count[this.faces[offset + 2]] += this.faces[offset + 0] - this.faces[offset + 1];
		offset += 3;
	}
	return count;
//	for(var i = 0; i < this.node.nvert; i++)
//		if(count[i] != 0)
//			boundary[i] = true;
//	return boundary;
},

norm: function(buffer, a, b, c) { //a b c offsets in the buffer
	var ba0 = buffer[b+0] - buffer[a+0];
	var ba1 = buffer[b+1] - buffer[a+1];
	var ba2 = buffer[b+2] - buffer[a+2];

	var ca0 = buffer[c+0] - buffer[a+0];
	var ca1 = buffer[c+1] - buffer[a+1];
	var ca2 = buffer[c+2] - buffer[a+2];

	var p = [];
	p[0] = ba1*ca2 - ba2*ca1;
	p[1] = ba2*ca0 - ba0*ca2;
	p[2] = ba0*ca1 - ba1*ca0;
	return p;
},

normalize: function(buffer, offset) {
	var x = buffer[offset + 0];
	var y = buffer[offset + 1];
	var z = buffer[offset + 2];
	var n = Math.sqrt(x*x + y*y + z*z);
	if(n > 0) {
		buffer[offset + 0] = x/n;
		buffer[offset + 1] = y/n;
		buffer[offset + 2] = z/n;
	}
},

computeNormals:function() {
	var tmp_normals = new Float32Array(this.node.nvert*3);

	var offset = 0;
	for(var i = 0; i < this.node.nface; i++) {
		var a = 3*this.faces[offset + 0];
		var b = 3*this.faces[offset + 1];
		var c = 3*this.faces[offset + 2];

		var buffer = this.coords;
		var ba0 = buffer[b+0] - buffer[a+0];
		var ba1 = buffer[b+1] - buffer[a+1];
		var ba2 = buffer[b+2] - buffer[a+2];

		var ca0 = buffer[c+0] - buffer[a+0];
		var ca1 = buffer[c+1] - buffer[a+1];
		var ca2 = buffer[c+2] - buffer[a+2];

		var n0 = ba1*ca2 - ba2*ca1;
		var n1 = ba2*ca0 - ba0*ca2;
		var n2 = ba0*ca1 - ba1*ca0;

		tmp_normals[a + 0] += n0;
		tmp_normals[a + 1] += n1;
		tmp_normals[a + 2] += n2;
		tmp_normals[b + 0] += n0;
		tmp_normals[b + 1] += n1;
		tmp_normals[b + 2] += n2;
		tmp_normals[c + 0] += n0;
		tmp_normals[c + 1] += n1;
		tmp_normals[c + 2] += n2;
		offset += 3;
	}

	//normalize
	var offset = 0;
	for(var i = 0; i < this.node.nvert; i++) {
		var x = tmp_normals[offset + 0];
		var y = tmp_normals[offset + 1];
		var z = tmp_normals[offset + 2];
		var n = Math.sqrt(x*x + y*y + z*z);
		if(n > 0) {
			tmp_normals[offset + 0] = x/n;
			tmp_normals[offset + 1] = y/n;
			tmp_normals[offset + 2] = z/n;
		}
		this.normals[offset + 0] = tmp_normals[offset + 0]*32767;
		this.normals[offset + 1] = tmp_normals[offset + 1]*32767;
		this.normals[offset + 2] = tmp_normals[offset + 2]*32767;
		offset += 3;
	}
},

decodeDiff: function(diff, bitstream) {
	var val;
	if(diff == 0) {
		val = 1;
	} else {
		val = 1<<(diff);
		val |= bitstream.read(diff);
	};
	val--; //vall is always >= 1
	if(val & 0x1)
		val = -((val+1)>>1);
	else
		val = val>>1;
	return val;
},

/* an edge is:   uint16_t face, uint16_t side, uint32_t prev, next, bool deleted
I do not want to create millions of small objects, I will use aUint32Array.
Problem is how long, sqrt(nface) we will over blow using nface.
*/

ilog2: function(p) {
	var k = 0;
	while ( p>>=1 ) { ++k; }
	return k;
},


decodeFaces: function(start, end) {

	var t = this;
	var clers = t.index.clers;
	var bitstream = t.index.bitstream;

	var front = t.index.front;
	var front_count = 0; //count each integer so it's front_back*5

	function addFront(_v0, _v1, _v2, _prev, _next) {
		front[front_count] = _v0;
		front[front_count+1] = _v1;
		front[front_count+2] = _v2;
		front[front_count+3] = _prev;
		front[front_count+4] = _next;
		front_count += 5;
	}

	var faceorder = new Uint32Array((end - start));
	var order_front = 0;
	var order_back = 0;

	var delayed = [];

	var splitbits = t.ilog2(t.nvert) + 1;

	var new_edge = -1;

	var prediction = t.index.prediction;

	while(start < end) {

		if(new_edge == -1 && order_front >= order_back && !delayed.length) {

			var last_index = t.vertex_count-1;
			var vindex = [];

			var split = 0;
			if(clers[t.cler] == 6) { //split look ahead
				t.cler++;
				split = bitstream.read(3);
			}

			for(var k = 0; k < 3; k++) {
				var v;
				if(split & (1<<k))
					v = bitstream.read(splitbits);
				else {
					prediction[t.vertex_count*3] = prediction[t.vertex_count*3+1] = prediction[t.vertex_count*3+2] = last_index;
					last_index = v = t.vertex_count++;
				}
				vindex[k] = v;
				t.index.faces[start++] = v;
			}

			var current_edge = front_count;
			faceorder[order_back++] = front_count;
			addFront(vindex[1], vindex[2], vindex[0], current_edge + 2*5, current_edge + 1*5);
			faceorder[order_back++] = front_count;
			addFront(vindex[2], vindex[0], vindex[1], current_edge + 0*5, current_edge + 2*5);
			faceorder[order_back++] = front_count;
			addFront(vindex[0], vindex[1], vindex[2], current_edge + 1*5, current_edge + 0*5);
			continue;
		}
		var edge;
		if(new_edge != -1) {
			edge = new_edge;
			new_edge = -1;
		} else if(order_front < order_back) {
			edge = faceorder[order_front++];
		} else {
			edge = delayed.pop();
		}
		if(typeof(edge) == "undefined")
			throw "aarrhhj";

		if(front[edge] < 0) continue; //deleted

		var c = clers[t.cler++];
		if(c == 4) continue; //BOUNDARY

		var v0   = front[edge + 0];
		var v1   = front[edge + 1];
		var v2   = front[edge + 2];
		var prev = front[edge + 3];
		var next = front[edge + 4];

		new_edge = front_count; //points to new edge to be inserted
		var opposite = -1;
		if(c == 0) { //VERTEX
			if(clers[t.cler] == 6) { //split
				t.cler++;
				opposite = bitstream.read(splitbits);
			} else {
				prediction[t.vertex_count*3] = v1;
				prediction[t.vertex_count*3+1] = v0;
				prediction[t.vertex_count*3+2] = v2;
				opposite = t.vertex_count++;
			}

			front[prev + 4] = new_edge;
			front[next + 3] = new_edge + 5;

			front[front_count] = v0;
			front[front_count + 1] = opposite;
			front[front_count + 2] = v1;
			front[front_count + 3] = prev;
			front[front_count + 4] = new_edge+5;
			front_count += 5; 

			faceorder[order_back++] = front_count;

			front[front_count] = opposite;
			front[front_count + 1] = v1;
			front[front_count + 2] = v0;
			front[front_count + 3] = new_edge; 
			front[front_count + 4] = next;
			front_count += 5; 

		} else if(c == 1) { //LEFT

			front[front[prev + 3] + 4] = new_edge;
			front[next + 3] = new_edge;
			opposite = front[prev];

			front[front_count] = opposite;
			front[front_count + 1] = v1;
			front[front_count + 2] = v0;
			front[front_count + 3] = front[prev + 3];
			front[front_count + 4] = next;
			front_count += 5; 

			front[prev] = -1; //deleted

		} else if(c == 2) { //RIGHT
			front[front[next + 4] + 3] = new_edge;
			front[prev + 4] = new_edge;
			opposite = front[next + 1];

			front[front_count] = v0;
			front[front_count + 1] = opposite;
			front[front_count + 2] = v1;
			front[front_count + 3] = prev;
			front[front_count + 4] = front[next+4];
			front_count += 5; 

			front[next] = -1;

		} else if(c == 5) { //DELAY
			delayed.push(edge);
			new_edge = -1;

			continue;

		} else if(c == 3) { //END
			front[front[prev + 3] + 4] = front[next + 4];
			front[front[next + 4] + 3] = front[prev + 3];
			
			opposite = front[prev];

			front[prev] = -1;
			front[next] = -1;
			new_edge = -1;

		} else {
			throw "INVALID CLER!";
		}
		if(v1 >= t.nvert || v0 >= t.nvert || opposite >= t.nvert)
			throw "Topological error";
		t.index.faces[start] = v1;
		t.index.faces[start+1] = v0;
		t.index.faces[start+2] = opposite;
		start += 3;
	}
}

};

var tot = 0;



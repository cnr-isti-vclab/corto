/*
Corto

Copyright(C) 2017 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  You should have received 
a copy of the GNU General Public License along with Corto.
If not, see <http://www.gnu.org/licenses/>.
*/

onmessage = function(job) {
	if(typeof(job.data) == "string") return;

	var buffer = job.data.buffer;
	if(!buffer) return;

	var decoder = new CortoDecoder(buffer);
	if(job.data.short_normals)
		decoder.attributes.normal.type = 3;
	if(job.data.rgba_colors)
		decoder.attributes.color.outcomponents = 4;
	var model = decoder.decode();
	
	//pass back job
	postMessage({ model: model, buffer: buffer, request: job.data.request});
}


function CortoDecoder(data, byteOffset, byteLength) {
	if(byteOffset & 0x3)
		throw "Memory aligned on 4 bytes is mandatory";

	var t = this;
	var stream = t.stream = new Stream(data, byteOffset, byteLength);
	
	var magic = stream.readInt();
	if(magic != 2021286656) return;

	var version = stream.readInt();
	t.entropy = stream.readUChar();
	//exif
	t.geometry = {};
	var n = stream.readInt();
	for(var i = 0; i < n; i++) {
		var key = stream.readString();
		t.geometry[key] = stream.readString();
	}

	//attributes
	var n = stream.readInt();

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

	for(var i in t.attributes)
		t.attributes[i].init(t.nvert, t.nface);

	if(t.nface == 0)
		t.decodePointCloud();
	else
		t.decodeMesh();

	return t.geometry;
},

decodePointCloud: function() {
	var t = this;
	t.index = new IndexAttr(t.nvert, t.nface, 0);
	t.index.decodeGroups(t.stream);
	t.geometry.groups = t.index.groups;
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
	t.index = new IndexAttr(t.nvert, t.nface);
	t.index.decodeGroups(t.stream);
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
			if(clers[t.cler++] == 6) { //split look ahead
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
		if(c == 0 || c == 6) { //VERTEX
			if(c == 6) { //split
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



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




function Attribute(name, q, components, type, strategy) {
	var t = this;
	t.name = name;
	t.q = q; //float
	t.components = components; //integer
	t.type = type;
	t.strategy = strategy;
}

Attribute.prototype = {

Type: { UINT32:0, INT32:1, UINT16:2, INT16:3, UINT8:4, INT8:5, FLOAT:6, DOUBLE:7 }, 

Strategy: { PARALLEL:1, CORRELATED:2 },

init: function(nvert, nface) {
	var t = this;
	var n = nvert*t.components;
	t.values = new Int32Array(n);  //local workspace 

	//init output buffers
	switch(t.type) {
	case t.Type.UINT32:
	case t.Type.INT32: t.values = t.buffer = new Int32Array(n); break; //no point replicating.
	case t.Type.UINT16:
	case t.Type.INT16: t.buffer = new Int16Array(n); break;
	case t.Type.UINT8: t.buffer = new Uint8Array(n); break;
	case t.Type.INT8: t.buffer  = new Int8Array(n); break;
	case t.Type.FLOAT:
	case t.Type.DOUBLE: t.buffer = new Float32Array(n); break;
	default: throw "Error if reading";
	}
},

decode: function(nvert, stream) {
	var t = this;
	if(t.strategy & t.Strategy.CORRELATED) //correlated
		stream.decodeArray(t.components, t.values);
	else
		stream.decodeValues(t.components, t.values);
},

deltaDecode: function(nvert, context) {
	var t = this;
	var values = t.values;
	var N = t.components;

	if(t.strategy & t.Strategy.PARALLEL) { //parallel
		var n = context.length/3;
		for(var i = 1; i < n; i++) {
			for(var c = 0; c < N; c++) {
				values[i*N + c] += values[context[i*3]*N + c] + values[context[i*3+1]*N + c] - values[context[i*3+2]*N + c];
			}
		}
	} else if(context) {
		var n = context.length/3;
		for(var i = 1; i < n; i++)
			for(var c = 0; c < N; c++)
				values[i*N + c] += values[context[i*3]*N + c];
	} else {
		for(var i = N; i < nvert*N; i++)
			values[i] += values[i - N];
	}
},

postDelta: function() {},

dequantize: function(nvert) {
	var t= this;
	var n = t.components*nvert;
	switch(t.type) {
	case t.Type.UINT32:
	case t.Type.INT32: break;
	case t.Type.UINT16:
	case t.Type.INT16: 
	case t.Type.UINT8: 
	case t.Type.INT8: 
		for(var i = 0; i < n; i++)
			t.buffer[i] = t.values[i]*t.q;
		break;
	case t.Type.FLOAT:
	case t.Type.DOUBLE: 
		for(var i = 0; i < n; i++)
			t.buffer[i] = t.values[i]*t.q;
		break;
	}
}

};


/* COLOR ATTRIBUTE */

function ColorAttr(name, q, components, type, strategy) {
	Attribute.call(this, name, q, components, type, strategy);
	this.qc = [];
	this.outcomponents = 3;
}

ColorAttr.prototype = Object.create(Attribute.prototype);
ColorAttr.prototype.decode = function(nvert, stream) {
	for(var c = 0; c < 4; c++)
		this.qc[c] = stream.readUChar();
	Attribute.prototype.decode.call(this, nvert, stream);
}

ColorAttr.prototype.dequantize = function(nvert) {
	var t = this;
	for(var i = 0; i < nvert; i++) {
		var offset = i*4;
		var rgboff = i*t.outcomponents;

		var e0 = t.values[offset + 0];
		var e1 = t.values[offset + 1];
		var e2 = t.values[offset + 2];

		t.buffer[rgboff + 0] = ((e2 + e0)* t.qc[0])&0xff;
		t.buffer[rgboff + 1] = e0* t.qc[1];
		t.buffer[rgboff + 2] = ((e1 + e0)* t.qc[2])&0xff;
//		t.buffer[offset + 3] = t.values[offset + 3] * t.qc[3];
	}
}

/* NORMAL ATTRIBUTE */

function NormalAttr(name, q, components, type, strategy) {
	Attribute.call(this, name, q, components, type, strategy);
}

NormalAttr.prototype = Object.create(Attribute.prototype);

NormalAttr.prototype.Prediction = { DIFF: 0, ESTIMATED: 1, BORDER: 2 };

NormalAttr.prototype.init = function(nvert, nface) {
	var t = this;
	var n = nvert*t.components;
	t.values = new Int32Array(2*nvert);  //local workspace 

	//init output buffers
	switch(t.type) {
	case t.Type.INT16: t.buffer = new Int16Array(n); break;
	case t.Type.FLOAT:
	case t.Type.DOUBLE: t.buffer = new Float32Array(n); break;
	default: throw "Error if reading";
	}
};

NormalAttr.prototype.decode = function(nvert, stream) {
	var t = this;
	t.prediction = stream.readUChar();

	stream.decodeArray(2, t.values);
};

NormalAttr.prototype.deltaDecode = function(nvert, context) {
	var t = this;
	if(t.prediction != t.Prediction.DIFF)
		return;

	if(context) {
		for(var i = 1; i < nvert; i++) {
			for(var c = 0; c < 2; c++) {
				var d = t.values[i*2 + c];
				t.values[i*2 + c] += t.values[context[i*3]*2 + c];
			}
		}
	} else { //point clouds assuming values are already sorted by proximity.
		for(var i = 2; i < nvert*2; i++) {
			var d = t.values[i];
			t.values[i] += t.values[i-2];
		}
	}
};

NormalAttr.prototype.postDelta = function(nvert, nface, attrs, index) {
	var t = this;
	//for border and estimate we need the position already deltadecoded but before dequantized
	if(t.prediction == t.Prediction.DIFF)
		return;

	if(!attrs.position)
		throw "No position attribute found. Use DIFF normal strategy instead.";

	var coord = attrs.position;

	t.estimated = new Float32Array(nvert*3);
	t.estimateNormals(nvert, coord.values, nface, index.faces);

	if(t.prediction == t.Prediction.BORDER) {
		t.boundary = new Uint32Array(nvert);
		t.markBoundary(nvert, nface, index.faces, t.boundary);
	}

	t.computeNormals(nvert);
} 

NormalAttr.prototype.dequantize = function(nvert) {
	var t = this;
	if(t.prediction != t.Prediction.DIFF)
		return;

	for(var i = 0; i < nvert; i++)
		t.toSphere(i, t.values, i, t.buffer, t.q)
}

NormalAttr.prototype.computeNormals = function(nvert) {
	var t = this;
	var norm = t.estimated;

	if(t.prediction == t.Prediction.ESTIMATED) {
		for(var i = 0; i < nvert; i++) {
			t.toOcta(i, norm, i, t.values, t.q);
			t.toSphere(i, t.values, i, t.buffer, t.q);
		}

	} else { //BORDER
		var count = 0; //here for the border.
		for(var i = 0, k = 0; i < nvert; i++, k+=3) {
			if(t.boundary[i] != 0) {
				t.toOcta(i, norm, count, t.values, t.q);
				t.toSphere(count, t.values, i, t.buffer, t.q);
				count++;

			} else { //no correction
				var len = 1/Math.sqrt(norm[k]*norm[k] + norm[k+1]*norm[k+1] + norm[k+2]*norm[k+2]);
				if(t.type == t.Type.INT16)
					len *= 32767;

				t.buffer[k] = norm[k]*len;
				t.buffer[k+1] = norm[k+1]*len;
				t.buffer[k+2] = norm[k+2]*len;  
			}
		}
	}
}

NormalAttr.prototype.markBoundary = function( nvert,  nface, index, boundary) {	
	for(var f = 0; f < nface*3; f += 3) {
		boundary[index[f+0]] ^= index[f+1] ^ index[f+2];
		boundary[index[f+1]] ^= index[f+2] ^ index[f+0];
		boundary[index[f+2]] ^= index[f+0] ^ index[f+1];
	}
}


NormalAttr.prototype.estimateNormals = function(nvert, coords, nface, index) {
	var t = this;
	for(var f = 0; f < nface*3; f += 3) {
		var a = 3*index[f + 0];
		var b = 3*index[f + 1];
		var c = 3*index[f + 2];

		var ba0 = coords[b+0] - coords[a+0];
		var ba1 = coords[b+1] - coords[a+1];
		var ba2 = coords[b+2] - coords[a+2];

		var ca0 = coords[c+0] - coords[a+0];
		var ca1 = coords[c+1] - coords[a+1];
		var ca2 = coords[c+2] - coords[a+2];

		var n0 = ba1*ca2 - ba2*ca1;
		var n1 = ba2*ca0 - ba0*ca2;
		var n2 = ba0*ca1 - ba1*ca0;

		t.estimated[a + 0] += n0;
		t.estimated[a + 1] += n1;
		t.estimated[a + 2] += n2;
		t.estimated[b + 0] += n0;
		t.estimated[b + 1] += n1;
		t.estimated[b + 2] += n2;
		t.estimated[c + 0] += n0;
		t.estimated[c + 1] += n1;
		t.estimated[c + 2] += n2;
	}
}


//taks input in ingress at i offset, adds out at c offset
NormalAttr.prototype.toSphere = function(i, input, o, out, unit) {

	var t = this;
	var j = i*2;
	var k = o*3;
	var av0 = input[j] > 0? input[j]:-input[j];
	var av1 = input[j+1] > 0? input[j+1]:-input[j+1];
	out[k] = input[j];
	out[k+1] = input[j+1];
	out[k+2] = unit - av0 - av1;
	if (out[k+2] < 0) {
		out[k] = (input[j] > 0)? unit - av1 : av1 - unit;
		out[k+1] = (input[j+1] > 0)? unit - av0: av0 - unit;
	}
	var len = 1/Math.sqrt(out[k]*out[k] + out[k+1]*out[k+1] + out[k+2]*out[k+2]);
	if(t.type == t.Type.INT16)
		len *= 32767;

	out[k] *= len;
	out[k+1] *= len;
	out[k+2] *= len;
}

NormalAttr.prototype.toOcta = function(i, input, o, output, unit) {
	var k = o*2;
	var j = i*3; //input

	var av0 = input[j] > 0? input[j]:-input[j];
	var av1 = input[j+1] > 0? input[j+1]:-input[j+1];
	var av2 = input[j+2] > 0? input[j+2]:-input[j+2];
	var len = av0 + av1 + av2;
	var p0 = input[j]/len;
	var p1 = input[j+1]/len;

	var ap0 = p0 > 0? p0: -p0;
	var ap1 = p1 > 0? p1: -p1;

	if(input[j+2] < 0) {
		p0 = (input[j] >= 0)? 1.0 - ap1 : ap1 - 1;
		p1 = (input[j+1] >= 0)? 1.0 - ap0 : ap0 - 1;
	}
	output[k] += p0*unit;
	output[k+1] += p1*unit;

/*
		Point2f p(v[0], v[1]);
		p /= (fabs(v[0]) + fabs(v[1]) + fabs(v[2]));

		if(v[2] < 0) {
			p = Point2f(1.0f - fabs(p[1]), 1.0f - fabs(p[0]));
			if(v[0] < 0) p[0] = -p[0];
			if(v[1] < 0) p[1] = -p[1];
		}
		return Point2i(p[0]*unit, p[1]*unit); */
}


/* INDEX ATTRIBUTE */

function IndexAttr(nvert, nface, type) {
	var t = this;
	if((!type && nface < (1<<16)) || type == 0) //uint16
		t.faces = new Uint16Array(nface*3);
	else if(!type || type == 2) //uint32 
		t.faces = new Uint32Array(nface*3);
	else
		throw "Unsupported type";

	t.prediction = new Uint32Array(nvert*3);
}

IndexAttr.prototype = {
decode: function(stream) {
	var t = this;

	var max_front = stream.readInt();
	t.front = new Int32Array(max_front*5);

	var tunstall = new Tunstall;
	t.clers = tunstall.decompress(stream);
	t.bitstream = stream.readBitStream();
},

decodeGroups: function(stream) {
	var t = this;
	var n = stream.readInt();
	t.groups = new Array(n);
	for(var i = 0; i < n; i++) {
		var end = stream.readInt();
		var np = stream.readUChar();
		var g = { end: end, properties: {} };
		for(var k = 0; k < np; k++) {
			var key = stream.readString();
			g.properties[key] = stream.readString();
		}
		t.groups[i] = g;
	}
}
};


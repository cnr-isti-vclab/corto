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


BitStream = function(array) {
	var t = this;
	t.a = array;
	t.current = array[0];
	t.position = 0; //position in the buffer
	t.pending = 32;  //bits still to read
};
 
BitStream.prototype = { 
	read: function(bits) {
		var t = this;
		if(bits > t.pending) {
			t.pending = bits - t.pending;
			var result = (t.current << t.pending)>>>0; //looks the same.
			t.pending = 32 - t.pending;

			t.current = t.a[++t.position];
			result |= (t.current >>> t.pending);
			t.current = (t.current & ((1<<t.pending)-1))>>>0; //slighting faster than mask.
			return result;
		} else { //splitting result in branch seems faster.

			t.pending -= bits;
			var result = (t.current >>> t.pending);
			t.current = (t.current & ((1<<t.pending)-1))>>>0; //slighting faster than mask, 
			return result;
		}
	}
};

Stream = function(buffer, byteOffset, byteLength, entropy) {
	var t = this;
	t.data = buffer;
	t.buffer = new Uint8Array(buffer);
	t.pos = byteOffset?byteOffset:0;
	t.view = new DataView(buffer);
    t.entropy = entropy ? entropy : 'tunstall';
}

Stream.prototype = {
	logs: new Uint8Array(16768), 
	readChar: function() {
		var c = this.buffer[this.pos++];
		if(c > 127) c -= 256;
		return c;
	},
	readUChar: function() {
		return this.buffer[this.pos++];
	},	
	readShort: function() {
		this.pos += 2;
		return this.view.getInt16(this.pos-2, true);
	},
	readFloat: function() {
		this.pos += 4;
		return this.view.getFloat32(this.pos-4, true);
	},
	readInt: function() {
		this.pos += 4;
		return this.view.getInt32(this.pos-4, true);
	},
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
	readBitStream:function() {
		var n = this.readInt();
		var pad = this.pos & 0x3;
		if(pad != 0)
			this.pos += 4 - pad;
		var b = new BitStream(new Uint32Array(this.data, this.pos, n));
		this.pos += n*4;
		return b;
	},
	//make decodearray2,3 later //TODO faster to create values here or passing them?
	decodeArray: function(N, values) {
		var t = this;
		var bitstream = t.readBitStream();
        while(t.logs.length < values.length)
            t.logs = new Uint8Array(values.length);

        if (t.entropy == 'tunstall') {
            var tunstall = new Tunstall;
            tunstall.decompress(this, t.logs);
        }
        else {
            // LZ4 decompress
        }

		for(var i = 0; i < t.logs.readed; i++) {
			var diff = t.logs[i];
			if(diff == 0) {
				for(var c = 0; c < N; c++)
					values[i*N + c] = 0;
				continue;
			}
			var max = (1<<diff)>>>1;
			for(var c = 0; c < N; c++)
				values[i*N + c] = bitstream.read(diff) - max;
		}
		return t.logs.readed;
	},

	//assumes values alread allocated
	decodeValues: function(N, values) {
		var t = this;
		var bitstream = t.readBitStream();
		var tunstall = new Tunstall;
		var size = values.length/N;
		while(t.logs.length < size)
			t.logs = new Uint8Array(size);

		for(var c = 0; c < N; c++) {
			tunstall.decompress(this, t.logs);

			for(var i = 0; i < t.logs.readed; i++) {
				var diff = t.logs[i];
				if(diff == 0) {
					values[i*N + c] = 0;
					continue;
				}

				var val = bitstream.read(diff);
				var middle = (1<<(diff-1))>>>0;
				if(val < middle)
					val = -val -middle;
				values[i*N + c] = val;
			}
		}
		return t.logs.readed;
	},

	//assumes values alread allocated
	decodeDiffs: function(values) {
		var t = this;
		var bitstream = t.readBitStream();
		var tunstall = new Tunstall;
		var size = values.length;
		while(t.logs.length < size)
			t.logs = new Uint8Array(size);

		tunstall.decompress(this, t.logs);

		for(var i = 0; i < t.logs.readed; i++) {
			var diff = t.logs[i];
			if(diff == 0) {
				values[i] = 0;
				continue;
			}
			var max = (1<<diff)>>>1;
			values[i] = bitstream.read(diff) - max;
		}
		return t.logs.readed;
	},

	//assumes values alread allocated
	decodeIndices: function(values) {
		var t = this;
		var bitstream = t.readBitStream();

		var tunstall = new Tunstall;
		var size = values.length;
		while(t.logs.length < size)
			t.logs = new Uint8Array(size);

		tunstall.decompress(this, t.logs);

		for(var i = 0; i < t.logs.readed; i++) {
			var ret = t.logs[i];
			if(ret == 0) {
				values[i] = 0;
				continue;
			}
			values[i] = (1<<ret) + bitstream.read(ret) -1;
		}
		return t.logs.readed;
	}
};
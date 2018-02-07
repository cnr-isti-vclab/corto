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

Stream = function(buffer, byteOffset, byteLength) {
	var t = this;
	t.data = buffer;
	t.buffer = new Uint8Array(buffer);
	t.pos = byteOffset?byteOffset:0;
	t.view = new DataView(buffer);
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

		var tunstall = new Tunstall;
		while(t.logs.length < values.length)
			t.logs = new Uint8Array(values.length);

		tunstall.decompress(this, t.logs);

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


function Tunstall() {
}


Tunstall.prototype = {
	wordsize: 8,
	dictionary_size: 256,
	starts: new Uint32Array(256), //starts of each queue
	queue: new Uint32Array(512), //array of n symbols array of prob, position in buffer, length //limit to 8k*3
	index: new Uint32Array(512),
	lengths: new Uint32Array(512),
	table: new Uint8Array(8192), //worst case for 2 

	decompress: function(stream, data) {
		var nsymbols = stream.readUChar();
		this.probs = stream.readArray(nsymbols*2);
		this.createDecodingTables();
		var size = stream.readInt();
		if(size > 100000000) throw("TOO LARGE!");
		if(!data)
			data = new Uint8Array(size);
		if(data.length < size)
			throw "Array for results too small";
		data.readed = size;

		var compressed_size = stream.readInt();
		if(size > 100000000) throw("TOO LARGE!");
		var compressed_data = stream.readArray(compressed_size);
		if(size)
			this._decompress(compressed_data, compressed_size, data, size);
		return data;
	}, 


	createDecodingTables: function() {

		var t = this;
		var n_symbols = t.probs.length/2;
		if(n_symbols <= 1) return;

		var queue = Tunstall.prototype.queue;

		var end = 0; //keep track of queue end
		var pos = 0; //keep track of buffer first free space
		var n_words = 0;

		//Here probs will range from 0 to 0xffff for better precision
		for(var i = 0; i < n_symbols; i++)
			queue[i] = t.probs[2*i+1] << 8;

		var max_repeat = Math.floor((t.dictionary_size - 1)/(n_symbols - 1));
		var repeat = 2;
		var p0 = queue[0];
		var p1 = queue[1];
		var prob = (p0*p0)>>>16;
		while(prob > p1 && repeat < max_repeat) {
			prob = (prob*p0)>>> 16;
			repeat++;
		}

		if(repeat >= 16) { //Very low entropy results in large tables > 8K.
			t.table[pos++] = t.probs[0];
			for(var k = 1; k < n_symbols; k++) {
				for(var i = 0; i < repeat-1; i++)
					t.table[pos++] = t.probs[0];
				t.table[pos++] = t.probs[2*k];
			}
			t.starts[0] = (repeat-1)*n_symbols;
			for(var k = 1; k < n_symbols; k++)
				t.starts[k] = k;

			for(var col = 0; col < repeat; col++) {
				for(var row = 1; row < n_symbols; row++) {
					var off = (row + col*n_symbols);
					if(col > 0)
						queue[off] = (prob * queue[row]) >> 16;
					t.index[off] = row*repeat - col;
					t.lengths[off] = col+1;
				}
				if(col == 0)
					prob = p0;
				else
					prob = (prob*p0) >>> 16;
			}
			var first = ((repeat-1)*n_symbols);
			queue[first] = prob;
			t.index[first] = 0;
			t.lengths[first] = repeat;

			n_words = 1 + repeat*(n_symbols - 1);
			end = repeat*n_symbols;

		} else {
			//initialize adding all symbols to queues
			for(var i = 0; i < n_symbols; i++) {
				queue[i] = t.probs[i*2+1]<<8;
				t.index[i] = i;
				t.lengths[i] = 1;
	
				t.starts[i] = i;
				t.table[i] = t.probs[i*2];
			}
			pos = n_symbols;
			end = n_symbols;
			n_words = n_symbols;
		}

		//at each step we grow all queues using the most probable sequence
		while(n_words < t.dictionary_size) {
			//find highest probability word
			var best = 0;
			var max_prob = 0;
			for(var i = 0; i < n_symbols; i++) {
				var p = queue[t.starts[i]]; //front of queue probability.
				if(p > max_prob) {
					best = i;
					max_prob = p;
				}
			}
			var start = t.starts[best];
			var offset = t.index[start];
			var len = t.lengths[start];

			for(var i = 0; i < n_symbols; i++) {
				queue[end] = (queue[i] * queue[start])>>>16;
				t.index[end] = pos;
				t.lengths[end] = len + 1;
				end++;

				for(var k  = 0; k < len; k++)
					t.table[pos + k] = t.table[offset + k]; //copy sequence of symbols
				pos += len;
				t.table[pos++] = t.probs[i*2]; //append symbol
				if(i + n_words == t.dictionary_size - 1)
					break;
			}
			if(i == n_symbols)
				t.starts[best] += n_symbols; //move one column
			n_words += n_symbols -1;
		}


		var word = 0;
		for(i = 0, row = 0; i < end; i ++, row++) {
			if(row >= n_symbols)
				row  = 0;
			if(t.starts[row] > i) continue; //skip deleted words

			t.index[word] = t.index[i];
			t.lengths[word] = t.lengths[i];
			word++;
		}
	},
	_decompress: function(input, input_size, output, output_size) {
		//TODO optimize using buffer arrays
		var input_pos = 0;
		var output_pos = 0;
		if(this.probs.length == 2) {
			var symbol = this.probs[0];
			for(var i = 0; i < output_size; i++)
				output[i] = symbol;
			return;
		}

		while(input_pos < input_size-1) {
			var symbol = input[input_pos++];
			var start = this.index[symbol];
			var end = start + this.lengths[symbol];
			for(var i = start; i < end; i++) 
				output[output_pos++] = this.table[i];
		}

		//last symbol might override so we check.
		var symbol = input[input_pos];
		var start = this.index[symbol];
		var end = start + output_size - output_pos;
		var length = output_size - output_pos;
		for(var i = start; i < end; i++)
			output[output_pos++] = this.table[i];

		return output;
	}
}

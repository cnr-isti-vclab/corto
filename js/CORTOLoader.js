/**
 * @author Don McCurdy / https://www.donmccurdy.com
 */

import {
	BufferAttribute,
	BufferGeometry,
	FileLoader,
	Loader
} from "../../../build/three.module.js";

var CORTOLoader = function ( manager ) {

	Loader.call( this, manager );

	this.decoderPath = '';
	this.decoderConfig = {};
	this.decoderBinary = null;
	this.decoderPending = null;

	this.workerLimit = 4;
	this.workerPool = [];
	this.workerNextTaskID = 1;
	this.workerSourceURL = '';

	this.short_index = false;
	this.short_normals = false;
	this.worker = null;
	this.lastRequest = 0;
	this.callbacks = {};

	this.defaultAttributes = [
		{ name: "position", numComponents: "3" },
		{ name: "normal"  , numComponents: "3" },
		{ name: "color"   , numComponents: "4" },
		{ name: "uv"      , numComponents: "2" },
	]
	this.preload();
};


CORTOLoader.prototype = Object.assign( Object.create( Loader.prototype ), {

	constructor: CORTOLoader,

	setDecoderPath: function ( path ) {
		this.decoderPath = path;
		return this;
	},

	/* use 16bits for face index if true */

	setShortIndex: function(enabled) {
		this.short_index = enabled;
		return this;
	},

	/* use 16bit for normals id true */
	setShortNormals: function(enabled) {
		this.short_normals = enabled;
		return this;
	},

	load: function ( url, onLoad, onProgress, onError ) {

		var loader = new FileLoader( this.manager );
		loader.setPath( this.path );
		loader.setResponseType( 'arraybuffer' );

		if ( this.crossOrigin === 'use-credentials' )
			loader.setWithCredentials( true );

		loader.load( url, ( buffer ) => {

			let request = this.lastRequest++;
			this.worker.postMessage( {
				request: request,
				buffer:buffer,
				short_index: this.short_index,
				short_normals: this.short_normals 
			} );
			this.callbacks[request] = { onLoad: onLoad, onError: onError }

		}, onProgress, onError );

	},

	preload: function () {

		if ( this.decoderPending ) return this.decoderPending;

		let that = this;
		let callbacks = this.callbacks
		let lib = this.decoderConfig.type === 'js'? 'corto.js' : 'corto.em.js';

		this.decoderPending = this._loadLibrary(lib, 'text')
			.then((text) => { 
				text = URL.createObjectURL( new Blob( [ text ] ) )
				this.worker = new Worker( text );

				this.worker.onmessage = function(e) {
					var message = e.data;
					if(!callbacks[message.request])
						return;

					let callback = callbacks[ message.request ];
					let geometry = that._createGeometry( message.geometry );
					callback.onLoad( geometry );
					delete(callbacks[ message.request ]);
				}
			});

		return;
	},

	dispose: function () {

		if(this.worker) {
			this.worker.terminate();
			this.worker = null;
		}
		return this;
	},

	_createGeometry: function ( geometry ) {

		var bufferGeometry = new BufferGeometry();

		if ( geometry.index )
			bufferGeometry.setIndex( new BufferAttribute( geometry.index, 1 ) );


		for(let i = 0; i < this.defaultAttributes.length; i++) {
			let attr = this.defaultAttributes[ i ];
			if(!geometry[ attr.name ])
				continue;
			let buffer = geometry[ attr.name ];
			bufferGeometry.setAttribute( attr.name, new BufferAttribute( buffer, attr.numComponents ) );
		}
		return geo;
	},

	_loadLibrary: function ( url, responseType ) {

		var loader = new FileLoader( this.manager );
		loader.setPath( this.decoderPath );
		loader.setResponseType( responseType );

		return new Promise( ( resolve, reject ) => {
			loader.load( url, resolve, undefined, reject );
		} );

	},

} );


export { CORTOLoader };

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


THREE.CORTOLoader = function(options, manager) {
	this.manager = (manager !== undefined) ? manager : THREE.DefaultLoadingManager;

	if(!options)
		options = {};
	this.path = options.path? options.path : '';
	this.loadMaterial = options.loadMaterial ? options.loadMaterial : true;
};


THREE.CORTOLoader.prototype = {

	constructor: THREE.CORTOLoader,

	load: function(url, onLoad, onProgress, onError) {
		const scope = this;
		const loader = new THREE.FileLoader(scope.manager);
		loader.setPath(this.path);
		loader.setResponseType('arraybuffer');
		loader.load(url, function(blob) {

			var now = performance.now();
			var decoder = new CortoDecoder(blob);
			var model = decoder.decode();

			/*used for debug and profiling */
			var ms = performance.now() - now;
			scope.blob = blob;
			scope.decode_time = ms;
			console.log((model.nvert/1024.0).toFixed(1) + "KV", ms.toFixed(1) + "ms", ((model.nvert/1000)/ms).toFixed(2) + "MV/s");

			var geometry = scope.geometry(model);
			if(model.nface) {
				var mesh = new THREE.Mesh(geometry);
			} else {
				var mesh = new THREE.Points(geometry);
			}

			if(scope.loadMaterial)
				scope.materials(model, mesh);

			onLoad(mesh);

		}, onProgress, onError);
	},

	geometry: function(model) {
		var geometry = new THREE.BufferGeometry();
		if (model.nface)
			geometry.setIndex(new THREE.BufferAttribute(model.index, 1));

		if(model.groups.length > 0) {
			var start = 0;
			for(var i = 0; i < model.groups.length; i++) {
				var g = model.groups[i];
				geometry.addGroup(start*3, g.end*3, i);
				start = g.end;
			}
		}

		geometry.addAttribute('position', new THREE.BufferAttribute(model.position, 3));
		if(model.color)
			geometry.addAttribute('color', new THREE.BufferAttribute(model.color, 3, true));
		if (model.normal)
			geometry.addAttribute('normal', new THREE.BufferAttribute(model.normal, 3, true));
		if (model.uv)
			geometry.addAttribute('uv', new THREE.BufferAttribute(model.uv, 2));
		return geometry;
	},

	materials: function(model, mesh) {

		var promise = { waiting: 0 }
		var options = {}
		options.shading = model.normal? THREE.SmoothShading : THREE.FlatShading;
		if(model.color)
			options.vertexColors = THREE.VertexColors;
		else
			options.color = 0xffffffff;

		if(model.nface) {
			options.side = THREE.DoubleSide;

			var materials = [];
			for(var i = 0; i < model.groups.length; i++) {
				var opt = Object.assign({}, options);
				var group = model.groups[i];
				if(group.properties.texture) {
					var textureLoader = new THREE.TextureLoader();
					promise.waiting++;
					opt.map = textureLoader.load(this.path + group.properties.texture, 
						function() { mesh.dispatchEvent({ type: "change" }); });
				}
				//use diffuse only when normals present and no texture.
				if(group.properties.texture && !model.normal)
					materials.push(new THREE.MeshBasicMaterial(opt));
				else
					materials.push(new THREE.MeshLambertMaterial(opt));
			}
			mesh.material = new THREE.MultiMaterial(materials);

		} else {
			options.size = 2;
			options.sizeAttenuation = false;
			mesh.material = new THREE.PointsMaterial(options);

		}


		//replace default material with mtl from lib if present
		if(model.mtllib) {
			var scope = this;
			var mtlLoader = new THREE.MTLLoader();
			mtlLoader.setPath( this.path );
			mtlLoader.load(model.mtllib, function( matcreator ) {
				matcreator.preload();
				if(model.nface == 0)
					mesh.material = new THREE.MultiMaterial(materials); //point cloud has a mesh.material...
				for(var i = 0; i < model.groups.length; i++) {
					var group = model.groups[i];
					if(group.properties.material) {
						var mat = mesh.material.materials[i] = matcreator.create(group.properties.material);
						function checkTex() {
							if(mat.map.image && mat.map.image.complete)
								mesh.dispatchEvent({type: "change"});
							else
								setTimeout(checkTex, 10);
						}
						if(mat.map)
							checkTex();
					}
				}
			});
		}
	}
};

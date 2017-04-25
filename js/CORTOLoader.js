

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
			var ms = performance.now() - now;

			var geometry = scope.geometry(model);
			if(model.nface) {
				console.log((model.nface/1024.0).toFixed(0) + "KT", ms.toFixed(0) + "ms", ((model.nface/1000)/ms).toFixed(2) + "MT/s");
				var mesh = new THREE.Mesh(geometry);
			} else {
				console.log((model.nvert/1024.0).toFixed(0) + "KV", ms.toFixed(0) + "ms", ((model.nvert/1000)/ms).toFixed(2) + "MV/s");
				var mesh = new THREE.Points(geometry);
			}

			if(scope.loadMaterial)
				scope.materials(model, mesh, onLoad);
			else
				onLoad(mesh);

		}, onProgress, onError);
	},

	geometry: function(model) {
		var geometry = new THREE.BufferGeometry();
		if (model.nface) {
			geometry.setIndex(new THREE.BufferAttribute(model.index, 1));
			if(model.groups.length > 0) {
				var start = 0;
				for(var i = 0; i < model.groups.length; i++) {
					var g = model.groups[i];
					geometry.addGroup(start*3, g.end*3, i);
					start = g.end;
				}
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

	materials: function(model, mesh, onLoad) {

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
					opt.map = textureLoader.load(this.path + group.properties.texture, function() { if(--promise.waiting == 0) onLoad(mesh); });
					materials.push(new THREE.MeshBasicMaterial(opt));
				} else
					materials.push(new THREE.MeshLambertMaterial(opt));
			}
			mesh.material = new THREE.MultiMaterial(materials);

		} else {
			options.size = 2;
			options.sizeAttenuation = false;
			mesh.material = THREE.PointsMaterial(options);
		}


		//replace default material with mtl from lib if present
		if(model.mtllib) {
			var mtlLoader = new THREE.MTLLoader();
			mtlLoader.setPath( this.path );
			mtlLoader.load(model.mtllib, function( matcreator ) {
				matcreator.preload();
		
				for(var i = 0; i < model.groups.length; i++) {
					var group = model.groups[i];
					if(group.properties.material)
						mesh.material.materials[i] = matcreator.create(group.properties.material);
				}
			});
		}
		if(promise.waiting == 0)
			onLoad(mesh);
	}
};

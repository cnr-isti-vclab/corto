/* Copyright (c) 2012, Gerhard Reitmayr
   All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef OBJLOAD_H_
#define OBJLOAD_H_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <vector>

namespace obj {

struct Block {
	uint32_t start, end;
	std::string material;
	std::set<std::string> groups;
};

struct IndexedModel {
	std::vector<float> vertex;   //< 3 * N entries
	std::vector<float> texCoord; //< 2 * N entries
	std::vector<float> normal;   //< 3 * N entries

	std::vector<uint32_t> faces;

	std::vector<Block> blocks;
	std::vector<std::string> mtllibs;
};

struct ObjModel {
	struct FaceVertex {
		FaceVertex() : v(-1), t(-1), n(-1) {}
		int v, t, n;

		bool operator<( const FaceVertex & other ) const;
		bool operator==( const FaceVertex & other ) const;
	};

	typedef std::pair<std::vector<FaceVertex>, std::vector<unsigned> > FaceList;

	std::vector<float> vertex;   //< 3 * N entries
	std::vector<float> texCoord; //< 2 * N entries
	std::vector<float> normal;   //< 3 * N entries

	std::vector<FaceVertex> index;
	std::vector<unsigned> face_offsets;  //store offset of first face_vertex in index to supports polygonal faces + 1 at the end.
	std::vector<Block> blocks;
	std::vector<std::string> mtllibs;
};

inline ObjModel parseObjModel( std::istream & in);
inline void tesselateObjModel( ObjModel & obj);
inline ObjModel tesselateObjModel( const ObjModel & obj );
inline IndexedModel convertToModel(const ObjModel &obj );

inline IndexedModel loadModel( std::istream & in );
inline IndexedModel loadModelFromString( const std::string & in );
inline IndexedModel loadModelFromFile( const std::string & in );

inline std::ostream & operator<<( std::ostream & out, const IndexedModel & m );
inline std::ostream & operator<<( std::ostream & out, const ObjModel::FaceVertex & f);

// ---------------------------- Implementation starts here -----------------------

inline bool ObjModel::FaceVertex::operator<( const ObjModel::FaceVertex & other ) const {
	return (v < other.v) || (v == other.v && t < other.t ) || (v == other.v && t == other.t && n < other.n);
}

inline bool ObjModel::FaceVertex::operator==( const ObjModel::FaceVertex & other ) const {
	return (v == other.v && t == other.t && n == other.n);
}

template <typename T>
inline std::istream & operator>>(std::istream & in, std::vector<T> & vec ){
	T temp;
	if(in >> temp)
		vec.push_back(temp);
	return in;
}

template <typename T>
inline std::istream & operator>>(std::istream & in, std::set<T> & vec ){
	T temp;
	if(in >> temp)
		vec.insert(temp);
	return in;
}

inline std::istream & operator>>( std::istream & in, ObjModel::FaceVertex & f) {
	if(in >> f.v){
		   if(in.peek() == '/'){
			   in.get();
			   in >> f.t;
			   in.clear();
			   if(in.peek() == '/'){
				   in.get();
				   in >> f.n;
				   in.clear();
			   }
		   }
		   in.clear();
		   --f.v;
		   --f.t;
		   --f.n;
	   }
	return in;

	in >> f.v;
//	cout << "V: " << f.v << endl;
	if(in.peek() == ' ' || !in.good()) { // v
		in.get();
		goto finish;
	}
	in.get(); //reading texture now

	if(in.peek() == ' ' || !in.good()) { // v
		in.get();
		goto finish;
	}

	if(in.peek() != '/')
		in >> f.t; //if nothing after / just f.n remains -1
//	cout << "T: " << f.t << endl;

	if(in.peek() == ' ') { // v/n or v/
		in.get();
		goto finish;
	}
	in.get(); //reading normal now

	if(in.peek() == ' ' || !in.good()) { // v
		in.get();
		goto finish;
	}

	if(in.peek() != '/')
		in >> f.n;
//	cout << "N: " << f.n << endl;

	finish:
	--f.v;
	--f.t;
	--f.n;

	return in;
}

ObjModel parseObjModel( std::istream & in ){
	char line[1024];
	std::string op;
	std::istringstream line_in;

	Block current_block;
	bool new_block =  true;

	ObjModel data;

	while(in.good()){
		in.getline(line, 1023);
		line_in.clear();
		line_in.str(line);

		if(!(line_in >> op))
			continue;
		if(op == "v")
			line_in >> data.vertex >> data.vertex >> data.vertex;
		else if(op == "vt")
			line_in >> data.texCoord >> data.texCoord >> data.texCoord;
		else if(op == "vn")
			line_in >> data.normal >> data.normal >> data.normal;

		else if(op == "g") { //new groups, start a new block
			current_block.groups.clear();
			while(line_in >> current_block.groups);
			new_block = true;

		} else if(op == "f") {

			if(new_block) {
				if(data.blocks.size())
					data.blocks.back().end = data.face_offsets.size();
				current_block.start = data.face_offsets.size();
				data.blocks.push_back(current_block);
				new_block = false;
			}

			data.face_offsets.push_back(data.index.size());
			while(line_in >> data.index) ;

		} else if(op == "usemtl") { //new material, start a new block
			line_in >> current_block.material;
			 new_block = true;

		} else if(op == "mtllib") {
			line_in >> op;
			data.mtllibs.push_back(op);
		}
	}
	if(data.blocks.size())
		data.blocks.back().end = data.face_offsets.size();
	data.face_offsets.push_back(data.index.size()); //add guard at the end.

	return data;
}

void tesselateObjModel( ObjModel & obj){
	std::vector<ObjModel::FaceVertex> & input = obj.index;
	std::vector<unsigned> & offsets = obj.face_offsets;

	std::vector<ObjModel::FaceVertex> output;
	std::vector<unsigned> output_offsets;
	output.reserve(input.size());
	output_offsets.reserve(offsets.size());

	std::vector<uint32_t> remap(offsets.size());
	for(size_t i = 0; i < offsets.size()-1; i++) { //-1 because of guard.
		unsigned start = offsets[i];
		unsigned end = offsets[i+1];
		remap[i] = output_offsets.size();
		for(unsigned k = start+1; k < end-1; k++) {
			output_offsets.push_back(output.size());
			output.push_back(input[start]);
			output.push_back(input[k]);
			output.push_back(input[k+1]);
		}
	}
	remap.back() = output_offsets.size();
	output_offsets.push_back(output.size());

	input.swap(output);
	offsets.swap(output_offsets);

	for(Block &block: obj.blocks) {
		block.start = remap[block.start];
		block.end = remap[block.end];
	}
}

IndexedModel convertToModel(const ObjModel & obj ) {
	IndexedModel model;
	model.mtllibs = obj.mtllibs;

	//sort facevertices according to coord index, then texture then normal index.
	std::vector<std::pair<ObjModel::FaceVertex, uint32_t>> vertices;
	vertices.reserve(obj.index.size());
	for(uint32_t i = 0; i < obj.index.size(); i++)
		vertices.emplace_back(std::make_pair(obj.index[i], i));

	std::sort(vertices.begin(), vertices.end());

	//collapse facevertices with same v, t, e n indices
	//build vertex remap for faces
	std::vector<uint32_t> remap;
	remap.resize(vertices.size());
	ObjModel::FaceVertex last;
	int count = 0; //number of denormalized vertices
	for(uint32_t i = 0; i < vertices.size(); i++) {
		int id = vertices[i].second;
		if(vertices[i].first == last) {
			remap[id] = count-1;
			continue;
		}
		last = vertices[i].first;

		remap[id] = count++;
		model.vertex.push_back(obj.vertex[last.v*3+0]);
		model.vertex.push_back(obj.vertex[last.v*3+1]);
		model.vertex.push_back(obj.vertex[last.v*3+2]);

		if(obj.normal.size()) {

			if(last.n >= 0) {
				model.normal.push_back(obj.normal[last.n*3+0]);
				model.normal.push_back(obj.normal[last.n*3+1]);
				model.normal.push_back(obj.normal[last.n*3+2]);
			} else {
				model.normal.push_back(0);
				model.normal.push_back(0);
				model.normal.push_back(0);
			}
		}
		if(obj.texCoord.size()) {
			if(last.t >= 0) {
				model.texCoord.push_back(obj.texCoord[last.t*2+0]);
				model.texCoord.push_back(obj.texCoord[last.t*2+1]);
			} else {
				model.texCoord.push_back(0);
				model.texCoord.push_back(0);
			}
		}
	}


	for(uint32_t i = 0; i < obj.index.size(); i++)
		model.faces.push_back(remap[i]);

	model.blocks = obj.blocks;

	for(auto i: model.faces)
		assert(i < model.vertex.size()/3);
	return model;


	/*
	// insert all face vertices into a vector and make unique
	std::vector<ObjModel::FaceVertex> unique(obj.faces.find("default")->second.first);
	std::sort(unique.begin(), unique.end());
	unique.erase( std::unique(unique.begin(), unique.end()), unique.end());

	// build a new model with repeated vertices/texcoords/normals to have single indexing
	IndexedModel model;
	for(std::vector<ObjModel::FaceVertex>::const_iterator f = unique.begin(); f != unique.end(); ++f){
		model.vertex.insert(model.vertex.end(), obj.vertex.begin() + 3*f->v, obj.vertex.begin() + 3*f->v + 3);
		if(!obj.texCoord.empty()){
			const int index = (f->t > -1) ? f->t : f->v;
			model.texCoord.insert(model.texCoord.end(), obj.texCoord.begin() + 2*index, obj.texCoord.begin() + 2*index + 2);
		}
		if(!obj.normal.empty()){
			const int index = (f->n > -1) ? f->n : f->v;
			model.normal.insert(model.normal.end(), obj.normal.begin() + 3*index, obj.normal.begin() + 3*index + 3);
		}
	}
	// look up unique index and transform face descriptions
	for(std::map<std::string, ObjModel::FaceList>::const_iterator g = obj.faces.begin(); g != obj.faces.end(); ++g){
		const std::string & name = g->first;
		const ObjModel::FaceList & fl = g->second;
		std::vector<uint32_t> & v = model.faces[name];
		v.reserve(fl.first.size());
		for(std::vector<ObjModel::FaceVertex>::const_iterator f = fl.first.begin(); f != fl.first.end(); ++f){
			const uint32_t index = std::distance(unique.begin(), std::lower_bound(unique.begin(), unique.end(), *f));
			v.push_back(index);
		}
	}
	return model; */
}

ObjModel tesselateObjModel( const ObjModel & obj ){
	ObjModel result = obj;
	tesselateObjModel(result);
	return result;
}

IndexedModel loadModel( std::istream & in ){
	ObjModel model = parseObjModel(in);
	tesselateObjModel(model);
	return convertToModel(model);
}

IndexedModel loadModelFromString( const std::string & str ){
	std::istringstream in(str);
	return loadModel(in);
}

IndexedModel loadModelFromFile( const std::string & str) {
	std::ifstream in(str.c_str());
	if(!in.is_open())
		return IndexedModel();
	return loadModel(in);
}

inline std::ostream & operator<<( std::ostream & out, const ObjModel::FaceVertex & f){
	out << f.v << "\t" << f.t << "\t" << f.n;
	return out;
}

std::ostream & operator<<( std::ostream & out, const IndexedModel & m ){
	if(!m.vertex.empty()){
		out << "vertex\n";
		for(size_t i = 0; i < m.vertex.size(); ++i)
			out << m.vertex[i] << (((i % 3) == 2)?"\n":"\t");
	}
	if(!m.texCoord.empty()){
		out << "texCoord\n";
		for(size_t i = 0; i < m.texCoord.size(); ++i)
			out << m.texCoord[i] << (((i % 2) == 1)?"\n":"\t");
	}
	if(!m.normal.empty()){
		out << "normal\n";
		for(size_t i = 0; i < m.normal.size(); ++i)
			out << m.normal[i] << (((i % 3) == 2)?"\n":"\t");
	}
	if(!m.faces.empty()){
		out << "faces\t";
		for(const Block &block: m.blocks) {
			if(block.material.size())
				out << "usemtl " << block.material << "\n";
			if(block.groups.size()) {
				out << "g";
				for(auto &g: block.groups)
					out << " " << g;
				out << "\n";
			}
			for(uint32_t i = block.start; i < block.end; i++)
				out << m.faces[i] << (((i % 3) == 2)?"\n":"\t");
		}
	}
	return out;
}

} // namespace obj

#endif // OBJLOAD_H_

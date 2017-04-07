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

struct Model {
	std::vector<float> vertex;   //< 3 * N entries
    std::vector<float> texCoord; //< 2 * N entries
	std::vector<float> normal;   //< 3 * N entries

	//faces split into set with groups and material
	std::map<std::string, std::vector<uint32_t> > faces; //< assume triangels and uniform indexing
};

struct ObjModel {
    struct FaceVertex {
        FaceVertex() : v(-1), t(-1), n(-1) {}
        int v, t, n;
        
        bool operator<( const FaceVertex & other ) const;
        bool operator==( const FaceVertex & other ) const;
    };
    
    typedef std::pair<std::vector<FaceVertex>, std::vector<unsigned> > FaceList;

    std::vector<float> vertex; //< 3 * N entries
    std::vector<float> texCoord; //< 2 * N entries
    std::vector<float> normal; //< 3 * N entries

    std::map<std::string, FaceList > faces;
	std::vector<std::string> mtllibs;
};

inline ObjModel parseObjModel( std::istream & in);
inline void tesselateObjModel( ObjModel & obj);
inline ObjModel tesselateObjModel( const ObjModel & obj );
inline Model convertToModel( const ObjModel & obj );

inline Model loadModel( std::istream & in );
inline Model loadModelFromString( const std::string & in );
inline Model loadModelFromFile( const std::string & in );

inline std::ostream & operator<<( std::ostream & out, const Model & m );
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

inline std::istream & operator>>( std::istream & in, ObjModel::FaceVertex & f){
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
    // std::cout << f << std::endl;
    return in;
}

ObjModel parseObjModel( std::istream & in ){
    char line[1024];
    std::string op;
    std::istringstream line_in;
    std::set<std::string> groups;
    groups.insert("default");

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
        else if(op == "g"){
            groups.clear();
            while(line_in >> groups) ;
            groups.insert("default");
        }
        else if(op == "f") {
            std::vector<ObjModel::FaceVertex> list;
            while(line_in >> list) ;
            
            for(std::set<std::string>::const_iterator g = groups.begin(); g != groups.end(); ++g){
                ObjModel::FaceList & fl = data.faces[*g];
                fl.second.push_back(fl.first.size());
                fl.first.insert(fl.first.end(), list.begin(), list.end());
            }
	   } else if(op == "mtllib") {
			line_in >> op;
			data.mtllibs.push_back(op);
		} else if(op == "usemtl") {
			line_in >> op;
			data.mtls.push_back(op);
        }
    }
    for(std::map<std::string, ObjModel::FaceList>::iterator g = data.faces.begin(); g != data.faces.end(); ++g){
        ObjModel::FaceList & fl = g->second;
        fl.second.push_back(fl.first.size());
    }
    return data;
}

inline void tesselateObjModel( std::vector<ObjModel::FaceVertex> & input, std::vector<unsigned> & input_start){
    std::vector<ObjModel::FaceVertex> output;
    std::vector<unsigned> output_start;
    output.reserve(input.size());
    output_start.reserve(input_start.size());
    
    for(std::vector<unsigned>::const_iterator s = input_start.begin(); s != input_start.end() - 1; ++s){
        const unsigned size = *(s+1) - *s;
        if(size > 3){
            const ObjModel::FaceVertex & start_vertex = input[*s];
			for( unsigned i = 1; i < size-1; ++i){
                output_start.push_back(output.size());
                output.push_back(start_vertex);
                output.push_back(input[*s+i]);
                output.push_back(input[*s+i+1]);
            }
        } else {
            output_start.push_back(output.size());
            output.insert(output.end(), input.begin() + *s, input.begin() + *(s+1));
        }
    }
    output_start.push_back(output.size());
    input.swap(output);
    input_start.swap(output_start);
}

void tesselateObjModel( ObjModel & obj){
    for(std::map<std::string, ObjModel::FaceList>::iterator g = obj.faces.begin(); g != obj.faces.end(); ++g){
        ObjModel::FaceList & fl = g->second;
        tesselateObjModel(fl.first, fl.second);
    }
}

Model convertToModel( const ObjModel & obj ) {
    // insert all face vertices into a vector and make unique
    std::vector<ObjModel::FaceVertex> unique(obj.faces.find("default")->second.first);
    std::sort(unique.begin(), unique.end());
    unique.erase( std::unique(unique.begin(), unique.end()), unique.end());

    // build a new model with repeated vertices/texcoords/normals to have single indexing
    Model model;
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
    return model;
}

ObjModel tesselateObjModel( const ObjModel & obj ){
    ObjModel result = obj;
    tesselateObjModel(result);
    return result;
}

Model loadModel( std::istream & in ){
    ObjModel model = parseObjModel(in);
    tesselateObjModel(model);
    return convertToModel(model);
}

Model loadModelFromString( const std::string & str ){
    std::istringstream in(str);
    return loadModel(in);
}

Model loadModelFromFile( const std::string & str) {
    std::ifstream in(str.c_str());
	if(!in.is_open())
		return Model();
    return loadModel(in);
}

inline std::ostream & operator<<( std::ostream & out, const ObjModel::FaceVertex & f){
    out << f.v << "\t" << f.t << "\t" << f.n;
    return out;
}

std::ostream & operator<<( std::ostream & out, const Model & m ){
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
		for(std::map<std::string, std::vector<uint32_t> >::const_iterator g = m.faces.begin(); g != m.faces.end(); ++g){
            out << g->first << " ";
        }
        out << "\n";
//        for(int i = 0; i < m.face.size(); ++i)
//            out << m.face[i] << (((i % 3) == 2)?"\n":"\t");
    }
    return out;
}

} // namespace obj

#endif // OBJLOAD_H_

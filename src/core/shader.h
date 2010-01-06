/*
    Copyright (c) 2009 Andrew Caudwell (acaudwell@gmail.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef GLSL_SHADER_H
#define GLSL_SHADER_H

#include "vectors.h"
#include "matrix.h"
#include "resource.h"
#include "display.h"

#include <map>
#include <string>
#include <fstream>

class Shader : public Resource {

    std::map<std::string, GLint> varMap;

    GLenum shaderProg;
    GLenum vertexShader;
    GLenum fragmentShader;

    GLint getVarLocation(std::string& name);

    std::string readSource(std::string filename);
    GLenum load(std::string filename, GLenum shaderType);
    void makeProgram();

    void checkError(std::string filename, GLenum shaderRef);
public:
    Shader(std::string prefix);
    ~Shader();

    GLenum getProgram();
    GLenum getVertexShader();
    GLenum getFragmentShader();

    void setInteger (std::string varname, int value);
    void setFloat(std::string varname, float value);
    void setVec2 (std::string varname, vec2f value);
    void setVec3 (std::string varname, vec3f value);
    void setVec4 (std::string varname, vec4f value);

    void setMat3 (std::string varname, mat3f value);
    void use();
};

class ShaderManager : public ResourceManager {
public:
    Shader* grab(std::string shader_prefix);
};

extern ShaderManager shadermanager;

#endif

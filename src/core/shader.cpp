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

#include "shader.h"

ShaderManager shadermanager;

//ShaderManager

Shader* ShaderManager::grab(std::string shader_prefix) {
    Resource* s = resources[shader_prefix];

    if(s==0) {
        s = new Shader(shader_prefix);
        resources[shader_prefix] = s;
    }

    s->addref();

    return (Shader*) s;
}

//Shader
Shader::Shader(std::string prefix) : Resource(prefix) {

    if(!gShadersEnabled) {
        printf("shaders are not enabled\n");
        exit(1);
    }

    std::string shader_dir = shadermanager.getDir();

    std::string vertexSrc   = shader_dir + prefix + std::string(".vert");
    std::string fragmentSrc = shader_dir + prefix + std::string(".frag");

    vertexShader   = load(vertexSrc,   GL_VERTEX_SHADER_ARB);
    fragmentShader = load(fragmentSrc, GL_FRAGMENT_SHADER_ARB);

    makeProgram();
}

Shader::~Shader() {
    glDeleteObjectARB(vertexShader);
    glDeleteObjectARB(fragmentShader);
    glDeleteObjectARB(shaderProg);
}

void Shader::makeProgram() {

    shaderProg = glCreateProgramObjectARB();
    glAttachObjectARB(shaderProg,fragmentShader);
    glAttachObjectARB(shaderProg,vertexShader);

    glLinkProgramARB(shaderProg);
}

void Shader::checkError(std::string filename, GLenum shaderRef) {
    char errormsg[1024];
    int errorlen = 0;

    glGetInfoLogARB(shaderRef, 1023, &errorlen, errormsg);
    errormsg[errorlen] = '\0';

    if(errorlen != 0) {
        printf("shader %s failed to compile: %s\n", filename.c_str(), errormsg);
        exit(1);
    }
}

GLenum Shader::load(std::string filename, GLenum shaderType) {

    std::string source = readSource(filename);

    if(source.size()==0) {
        printf("could not read shader %s\n", filename.c_str());
        exit(1);
    }

    GLenum shaderRef = glCreateShaderObjectARB(shaderType);

    const char* source_ptr = source.c_str();
    int source_len = source.size();

    glShaderSourceARB(shaderRef, 1, (const GLcharARB**) &source_ptr, &source_len);

    glCompileShaderARB(shaderRef);

    checkError(filename, shaderRef);

    return shaderRef;
}

std::string Shader::readSource(std::string file) {

    std::string source;

    // get length
    std::ifstream in(file.c_str());

    if(!in.is_open()) return source;

    std::string line;
    while( std::getline(in,line) ) {
        source += line;
        source += "\n";
    }

    in.close();

    return source;
}

void Shader::use() {
    glUseProgramObjectARB(shaderProg);
}

GLenum Shader::getProgram() {
    return shaderProg;
}

GLenum Shader::getVertexShader() {
    return vertexShader;
}

GLenum Shader::getFragmentShader() {
    return fragmentShader;
}

GLint Shader::getVarLocation(std::string& name) {

    GLint loc = varMap[name] - 1;

    if(loc != -1) return loc;

    loc = glGetUniformLocationARB( shaderProg, name.c_str() );

    varMap[name] = loc + 1;

    return loc;
}

void Shader::setFloat(std::string varname, float value) {
    GLint loc = getVarLocation(varname);
    glUniform1fARB(loc, value);
}

void Shader::setVec2 (std::string varname, vec2f value) {
    GLint loc = getVarLocation(varname);
    glUniform2fvARB(loc, 1, value);
}

void Shader::setVec3 (std::string varname, vec3f value) {
    GLint loc = getVarLocation(varname);
    glUniform3fvARB(loc, 1, value);
}

void Shader::setVec4 (std::string varname, vec4f value) {
    GLint loc =  getVarLocation(varname);
    glUniform4fvARB(loc, 1, value);
}

void Shader::setMat3 (std::string varname, mat3f value) {
    GLint loc =  getVarLocation(varname);
    glUniformMatrix3fvARB(loc, 1, 0, value);
}

void Shader::setInteger (std::string varname, int value) {
    GLint loc =  getVarLocation(varname);
    glUniform1iARB(loc, value);
}




#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>

class Shader {
public:
    unsigned int shaderProgram;

    Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    ~Shader();

private:
    unsigned int compileShader(GLenum type, const std::string& source);
    unsigned int createShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
};

#endif
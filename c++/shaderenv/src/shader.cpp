#include "shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>

constexpr int COMPILATION_ERROR = 0;
constexpr int LINKING_ERROR = 1;

Shader::Shader(const char* vertex_src, const char* fragment_src)
{
    vert_handle = glCreateShader(GL_VERTEX_SHADER);
    frag_handle = glCreateShader(GL_FRAGMENT_SHADER);

    compile_from_source(vertex_src, vert_handle);
    compile_from_source(fragment_src, frag_handle);

    // shader Program
    compile_program(vert_handle, frag_handle);

    glUseProgram(handle);
}

void Shader::recompile_frag(const char* new_frag_src)
{
    glDeleteShader(frag_handle);
    glDeleteProgram(handle);

    frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    compile_from_source(new_frag_src, frag_handle);

    compile_program(vert_handle, frag_handle);
    glUseProgram(handle);
}

void Shader::compile_from_source(std::string src, unsigned shader) const
{
    const char* csource = src.c_str();

    glShaderSource(shader, 1, &csource, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
        log_error(COMPILATION_ERROR, shader, "shader");
}

void Shader::compile_program(unsigned v_handle, unsigned f_handle)
{
    handle = glCreateProgram();
    glAttachShader(handle, v_handle);
    glAttachShader(handle, f_handle);
    glLinkProgram(handle);

    int success;
    glGetProgramiv(handle, GL_LINK_STATUS, &success);
    
    if (!success) 
        log_error(LINKING_ERROR, handle, "program");
}

void Shader::log_error(int error_type, unsigned shader, const char* target) const
{
    char info[1024];

    switch (error_type) {
    case COMPILATION_ERROR: 
        glGetShaderInfoLog(shader, 1024, NULL, info);
        printf("Error compiling %s:\n%s", target, info);
        break;
    case LINKING_ERROR: 
        glGetProgramInfoLog(shader, 1024, NULL, info);
        printf("Error linking %s:\n%s", target, info);
        break;
    }
}

void Shader::uniform_int(const char* name, int value) const
{
    glUniform1i(glGetUniformLocation(handle, name), value);
}

void Shader::uniform_float(const char* name, float value) const
{
    glUniform1f(glGetUniformLocation(handle, name), value);
}

void Shader::uniform_vec2(const char* name, const float* value) const
{
    glUniform2fv(glGetUniformLocation(handle, name), 1, value);
}

void Shader::uniform_vec2(const char* name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(handle, name), x, y);
}

void Shader::uniform_vec3(const char* name, const float* value) const
{
    glUniform3fv(glGetUniformLocation(handle, name), 1, value);
}

void Shader::uniform_vec3(const char* name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(handle, name), x, y, z);
}

void Shader::uniform_vec4(const char* name, const float* value) const
{
    glUniform4fv(glGetUniformLocation(handle, name), 1, value);
}

void Shader::uniform_vec4(const char* name, 
                          float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(handle, name), x, y, z, w);
}

void Shader::uniform_mat4(const char* name, float* value) const
{
    int location = glGetUniformLocation(handle, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
}
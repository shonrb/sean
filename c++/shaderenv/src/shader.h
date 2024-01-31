#ifndef __SHADER_H_
#define __SHADER_H_

#include <string>

struct Shader {
    unsigned handle;
    unsigned vert_handle;
    unsigned frag_handle;

    Shader(const char* vert_path, const char* frag_path);
    void recompile_frag(const char* new_frag_src);
    void uniform_int(const char* name, int value) const;
    void uniform_float(const char* name, float value) const;
    void uniform_vec2(const char* name, const float* value) const;
    void uniform_vec2(const char* name, float x, float y) const;
    void uniform_vec3(const char* name, const float* value) const;
    void uniform_vec3(const char* name, float x, float y, float z) const;
    void uniform_vec4(const char* name, const float* value) const;
    void uniform_vec4(const char* name, float x, float y, float z, float w) const;
    void uniform_mat4(const char* name, float* value) const;

private:
    void compile_from_source(std::string src, unsigned shader) const;
    void compile_program(unsigned v_handle, unsigned f_handle);
    void log_error(int error_type, unsigned shader, const char* target) const;
};

#endif
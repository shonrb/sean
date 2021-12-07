#ifndef __OPENGL_OBJECTS_H
#define __OPENGL_OBJECTS_H

#include <glad/glad.h>
#include <array>
#include <vector>

unsigned make_buffer(void* data, unsigned size, int type);

template<typename ...T> // Assumes that only unsigned ints are passed in
unsigned make_vertex_array(T&&... attrs) 
{
    std::vector<unsigned> attrs_vec = { attrs... };
    unsigned offset = 0;
    unsigned vertex_size = 0;

    std::for_each(attrs_vec.begin(), attrs_vec.end(), [&](unsigned n) 
    {
        vertex_size += n;
    });

    unsigned handle;
    glGenVertexArrays(1, &handle);
    glBindVertexArray(handle);

    for (int i = 0; i < attrs_vec.size(); ++i)
    {
        unsigned v = attrs_vec[i];
        glVertexAttribPointer(
            i, v, GL_FLOAT, false, vertex_size * sizeof(float), (void*) (offset * sizeof(float)));
        glEnableVertexAttribArray(i);
        offset += v;
    }
    return handle;
}

#endif
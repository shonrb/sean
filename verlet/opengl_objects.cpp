#include "opengl_objects.h"

unsigned make_buffer(void* data, unsigned size, int type)
{
    unsigned handle;
    glGenBuffers(1, &handle);
    glBindBuffer(type, handle);
    glBufferData(type, size, data, GL_STATIC_DRAW);
    return handle;
}

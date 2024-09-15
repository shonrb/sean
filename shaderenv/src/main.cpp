/*
 * shaderenv
 * An environment for glsl fragment shaders.
 * automatically recompiles the shader when changes are made.
 */
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#include "shader.h"
#include "watcher.h"

#include <string>
#include <iostream>

constexpr int STARTING_WIDTH = 800;
constexpr int STARTING_HEIGHT = 800;

static const char* vertex_shader_src =
    "#version 460 core\n"                                  \
    "layout (location = 0) in vec3 aPos;"                  \
    "void main()"                                          \
    "{"                                                    \
    "    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);" \
    "}";

static const float screen_vertices[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f,  1.0f,
    1.0f, -1.0f,
    1.0f,  1.0f,
};

int main(int argc, const char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " [shader file]\n";
        return 1;
    }

    const char *filename = argv[1];

    // Set up the windowing
    SDL_Window *window = SDL_CreateWindow(
        "Shaders",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        STARTING_WIDTH, STARTING_HEIGHT, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    // Set up openGL extensions
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialisation failed\n";
        return 1;
    }

    // Set up the vertex objects
    unsigned VBO_handle, VAO_handle;
    glGenBuffers(1, &VBO_handle);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_handle);
    glBufferData(
        GL_ARRAY_BUFFER, 
        sizeof(screen_vertices), 
        screen_vertices, 
        GL_STATIC_DRAW
    );
    glGenVertexArrays(1, &VAO_handle);
    glBindVertexArray(VAO_handle);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    // Set up the shader and file watcher
    FileWatcher watcher(filename);
    Shader shader(vertex_shader_src, watcher.get_contents().c_str());

    // Set up everything else
    float current_width = STARTING_WIDTH;
    float current_height = STARTING_HEIGHT;
    bool running = true;

    while (running) {
        float time = float(SDL_GetTicks()) / 1000.0f;
        glClear(GL_COLOR_BUFFER_BIT);

        // Handle events
        for (SDL_Event e; SDL_PollEvent(&e);) {
            switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    current_width = e.window.data1;
                    current_height = e.window.data2;
                }
                break;
            }
        }

        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);

        // Recompile the shader if the file has changed
        watcher.check_for_change([&](std::string contents) {
            shader.recompile_frag(contents.c_str());
        });

        // Send the uniforms to the shader
        shader.uniform_float("u_time", time);
        shader.uniform_vec2("u_resolution", current_width, current_height);
        shader.uniform_vec2("u_mouse", float(mousex), float(mousey));

        // Draw the screen and swap the buffers
        glViewport(0, 0, current_width, current_height);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    return 0;
}
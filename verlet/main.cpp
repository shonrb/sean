/* verlet
 * A verlet physics engine with an openGL frontend
 */
#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <array>
#include <cassert>
#include <string>
#include <iostream>
#include "camera.h"
#include "opengl_objects.h"
#include "verlet_system.h"

constexpr int   
WIDTH    = 853,
HEIGHT   = 480,
CENTRE_X = WIDTH / 2,
CENTRE_Y = HEIGHT / 2;

constexpr float 
HEAD_HEIGHT  = 1.5f,
GROUND_SCALE = 50.0f,
POINT_SCALE  = 0.1f,
NEAR_C       = 0.1f,
FAR_C        = 1000.0f,
FOV          = 45.0f,
SKY_R        = 0.2f,
SKY_G        = 0.3f,
SKY_B        = 1.0f;

std::array<float, 6*5> ground_plane = { // Pos & tex coords
    -0.5, 0.0,  -0.5,   0.0, 0.0,
    -0.5, 0.0,  0.5,    0.0, 1.0,
    0.5,  0.0,  0.5,    1.0, 1.0,
    0.5,  0.0,  0.5,    1.0, 1.0,
    0.5,  0.0,  -0.5,   1.0, 0.0,
    -0.5, 0.0,  -0.5,   0.0, 0.0
};

std::array<float, 36*6> cube = {
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f, 0.5f,  0.5f, -0.5f,  
    -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 
    -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  0.5f,  
    0.5f,  0.5f,  0.5f, 0.5f,  0.5f,  0.5f,  
    -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, 
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, 
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 
    -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, 
    0.5f,  0.5f,  0.5f, 0.5f,  0.5f, -0.5f,  
    0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f,  
    0.5f, -0.5f,  0.5f, 0.5f,  0.5f,  0.5f,  
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f,  
    0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  0.5f,  
    -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, 
    -0.5f,  0.5f, -0.5f, 0.5f,  0.5f, -0.5f,  
    0.5f,  0.5f,  0.5f, 0.5f,  0.5f,  0.5f,  
    -0.5f,  0.5f,  0.5f, 0.5f,  0.5f, -0.5f, 
};

void render(sf::Shader& shader, unsigned vertex_array, unsigned size)
{
    glUseProgram(shader.getNativeHandle());
    glBindVertexArray(vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, size);
}

auto uniform(glm::mat4 matrix)
{
    return sf::Glsl::Mat4(glm::value_ptr(matrix));
}

void cast_interaction_ray(VerletSystem& verlet_system, const Camera& camera)
{
    constexpr int NUM_STEPS = 1000;
    constexpr float STEP_SCALE = 0.1;
    
    auto ray = camera.position;
    for (int step = 0; step < NUM_STEPS; ++step)
    {
        ray.x += glm::cos(glm::radians(camera.yaw)) * STEP_SCALE;
        ray.y += glm::tan(glm::radians(camera.pitch)) * STEP_SCALE;
        ray.z += glm::sin(glm::radians(camera.yaw)) * STEP_SCALE;

        if (verlet_system.grab_nearby_point(ray.x, ray.y, ray.z))
            return;
    }
}

void update_grabbed_point(VerletSystem& verlet_system, const Camera& camera)
{
    constexpr float REACH = 3.0;
    auto pos = camera.position;
    auto x = pos.x + glm::cos(glm::radians(camera.yaw)) * REACH;
    auto y = pos.y + glm::tan(glm::radians(camera.pitch)) * REACH;
    auto z = pos.z + glm::sin(glm::radians(camera.yaw)) * REACH;
    verlet_system.update_grabbed_point(x, y, z);
}

void handle_input(sf::Window& window, 
                  VerletSystem& verlet_system, 
                  Camera& camera,
                  bool& running, bool& physics_running,
                  std::array<bool, sf::Keyboard::KeyCount>& keys)
{
    for (sf::Event event; window.pollEvent(event);)
    {
        switch (event.type)
        {
        case sf::Event::MouseMoved:
        {
            float dx = event.mouseMove.x - CENTRE_X;
            float dy = event.mouseMove.y - CENTRE_Y;
            camera.rotate(dx, -dy);
            break;
        }
        case sf::Event::MouseButtonPressed:
            cast_interaction_ray(verlet_system, camera);
            break;
        case sf::Event::MouseButtonReleased:
            verlet_system.ungrab_point();
            break;
        case sf::Event::KeyPressed:  keys[event.key.code] = true;  break;
        case sf::Event::KeyReleased: keys[event.key.code] = false; break;
        case sf::Event::Closed:      running = false;              break;
        }
    }
#define BIND(key, action) if (keys[key]) {action;}
    BIND(sf::Keyboard::Escape,  running = false)
    BIND(sf::Keyboard::W,       camera.move(CameraMove::FORWARDS))
    BIND(sf::Keyboard::S,       camera.move(CameraMove::BACKWARDS))
    BIND(sf::Keyboard::D,       camera.move(CameraMove::RIGHT))
    BIND(sf::Keyboard::A,       camera.move(CameraMove::LEFT))
    BIND(sf::Keyboard::O,       physics_running = false)
    BIND(sf::Keyboard::P,       physics_running = true)
#undef BIND
}

int main() 
{
    // Set up window
    sf::Clock clock;
    std::array<bool, sf::Keyboard::KeyCount> keys = { false };

    sf::ContextSettings settings(
        /*Depth bits   */ 24, /*Stencil bits */ 8, /*Antialiasing     */ 4,
        /*Version major*/ 4,  /*Version minor*/ 2, /*Attr flags (core)*/ 1);
    sf::Window window(
        sf::VideoMode(WIDTH, HEIGHT, 32), 
        "Verlet physics", sf::Style::Default, settings
    );
    window.setKeyRepeatEnabled(false);
    window.requestFocus();
    window.setMouseCursorVisible(false);

    if (!gladLoadGLLoader((GLADloadproc) sf::Context::getFunction))
    {
        std::fprintf(stderr, "GLAD Error");
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);

    // Init opengl objects
    sf::Shader ground_shader;
    ground_shader.loadFromFile("ground_vert.glsl", "ground_frag.glsl");
    unsigned ground_vbo = make_buffer(&ground_plane, sizeof(ground_plane), GL_ARRAY_BUFFER);
    unsigned ground_vao = make_vertex_array(3u, 2u); // Position: 3, Texcoords: 2

    sf::Shader point_shader;
    point_shader.loadFromFile("basic_vert.glsl", "basic_frag.glsl");
    unsigned point_vbo = make_buffer(&cube, sizeof(cube), GL_ARRAY_BUFFER);
    unsigned point_vao = make_vertex_array(3u); // Position: 3
    
    // Generate a checkerboard pattern for the ground
    unsigned texture;
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        constexpr int TEX_SIZE = 31;
        std::array<unsigned char, TEX_SIZE*TEX_SIZE*4> texdata;
        for (int i = 0; i < TEX_SIZE*TEX_SIZE; ++i)
        {
            int offset = i * 4;
            bool odd = i % 2 == 0;
            texdata[offset + 0] = odd ? 255 : 0; // r
            texdata[offset + 1] = odd ? 255 : 0; // g
            texdata[offset + 2] = odd ? 255 : 0; // b
            texdata[offset + 3] = 255;           // a
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, &texdata);
        glGenerateMipmap(GL_TEXTURE_2D);

        ground_shader.setUniform("tex_sampler", 0);
    }

    Camera camera(0, HEAD_HEIGHT, 0);
    bool physics_running = true;

    VerletSystem verlet_system;
    verlet_system.add_point(0, 3, -6);
    verlet_system.add_point(0.2, 3, -6);
    verlet_system.add_point(0.4, 3, -6);
    verlet_system.add_point(0.6, 3, -6);
    verlet_system.add_constraint(0, 1);
    verlet_system.add_constraint(1, 2);
    verlet_system.add_constraint(2, 3);

    for (bool running = true; running;)
    {
        float start_of_frame = clock.getElapsedTime().asSeconds();

        if (physics_running)
        {
            verlet_system.update();
            update_grabbed_point(verlet_system, camera);
        }
        
        // Handle input
        handle_input(
            window, verlet_system, camera, running, physics_running, keys);

        // Drawing
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Create matrices
        auto view = camera.get_view_matrix();
        auto projection = glm::perspective(
            glm::radians(FOV), float(WIDTH) / float(HEIGHT), NEAR_C, FAR_C);

        // Draw ground
        auto model = glm::scale(
            glm::mat4(1.0f), glm::vec3(GROUND_SCALE, 0, GROUND_SCALE));
        ground_shader.setUniform("model", uniform(model));
        ground_shader.setUniform("view", uniform(view));
        ground_shader.setUniform("projection", uniform(projection));
        render(ground_shader, ground_vao, 6);

        // Draw points
        point_shader.setUniform("view", uniform(view));
        point_shader.setUniform("projection", uniform(projection));
        verlet_system.draw_points([&](float x, float y, float z)
        {
            // Translate the model matrix to the point position
            model = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
                glm::vec3(POINT_SCALE));
            point_shader.setUniform("model", uniform(model));
            render(point_shader, point_vao, 36);
        });

        window.display();

        sf::Mouse::setPosition({CENTRE_X, CENTRE_Y}, window);
        // Lock the framerate to 60fps
        {
            const float target_period = 0.016666;
            float end_of_frame, frame_period, wait_time;

            end_of_frame = clock.getElapsedTime().asSeconds();
            frame_period = end_of_frame - start_of_frame;
            wait_time = target_period - frame_period;
            sf::sleep(sf::seconds(wait_time));
        }
    }
    
    return 0;
}
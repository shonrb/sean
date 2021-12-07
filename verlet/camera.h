#ifndef __CAMERA_H
#define __CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class CameraMove
{
    FORWARDS,
    BACKWARDS,
    LEFT,
    RIGHT,
};

class Camera
{
public:
    glm::vec3 position;

    // euler angles
    glm::vec3 euler_z;
    glm::vec3 euler_y;
    glm::vec3 euler_x;

    glm::vec3 forwards;

    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float pitch;
    float yaw;
    float fov;
    float sensitivity;
    float speed;

public:
    Camera(float, float, float);
    void rotate(float, float);
    void move(CameraMove);
    glm::mat4 get_view_matrix() const;
};

#endif
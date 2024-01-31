#include "camera.h"

Camera::Camera(float x, float y, float z)
: yaw(-90.0f), pitch(0.0f), fov(0.78f), sensitivity(0.1f), speed(0.1f),
  position(glm::vec3(x, y, z)), euler_z(glm::vec3(0.0f, 0.0f, -1.0f))
{
    rotate(0.0f, 0.0f);
}

void Camera::rotate(float x, float y)
{
    yaw   += x * sensitivity;
    pitch += y * sensitivity;

    if      (pitch > 89.0f)  pitch = 89.0f;
    else if (pitch < -89.0f) pitch = -89.0f;

    euler_z = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))));

    euler_x = glm::normalize(glm::cross(euler_z, up));
    euler_y = glm::normalize(glm::cross(euler_x, euler_z));
    forwards = glm::normalize(glm::cross(up, euler_x));
}

void Camera::move(CameraMove movement)
{
    switch (movement)
    {
    case CameraMove::FORWARDS:
        position += forwards * speed;
        break;
    case CameraMove::BACKWARDS:
        position += forwards * -speed;
        break;
    case CameraMove::RIGHT:
        position += euler_x * speed;
        break;
    case CameraMove::LEFT:
        position += euler_x * -speed;
        break;
    }
}

glm::mat4 Camera::get_view_matrix() const
{
    return glm::lookAt(position, position + euler_z, euler_y);
}
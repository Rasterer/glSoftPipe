#pragma once

#include <glm/glm.hpp>

namespace glsp {

class GlspCamera
{
public:
    GlspCamera() = default;
    ~GlspCamera() = default;

    void InitCamera(const glm::vec3 &position, const glm::vec3 &target, const glm::vec3 &up);
    glm::mat4 GetViewMatrix() const;
    bool CameraControl(unsigned long cmd);

private:
    glm::vec3  mCameraPosition;
    glm::vec3  mCameraTarget;
    glm::vec3  mCameraUp;
    glm::vec3  mCameraRight;
    glm::mat4  mViewMatrix;
};

} // namespace glsp

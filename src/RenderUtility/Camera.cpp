#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace glsp {

void GlspCamera::InitCamera(const glm::vec3 &position, const glm::vec3 &target, const glm::vec3 &up)
{
    mCameraPosition = position;
    mCameraTarget   = glm::normalize(target - position);
    mCameraUp       = glm::normalize(up);
    mCameraRight    = glm::cross(mCameraTarget, mCameraUp);
    mViewMatrix     = glm::lookAt(mCameraPosition,
                                target,
                                mCameraUp);
}

glm::mat4 GlspCamera::GetViewMatrix() const
{
    return mViewMatrix;
}

bool GlspCamera::CameraControl(unsigned long cmd)
{
    switch (cmd)
    {
        case 'W':
        {
            mCameraPosition += (mCameraUp * 1.0f);
            break;
        }
        case 'X':
        {
            mCameraPosition += (mCameraUp * (-1.0f));
            break;
        }
        case 'A':
        {
            mCameraPosition += (mCameraRight * (-1.0f));
            break;
        }
        case 'D':
        {
            mCameraPosition += (mCameraRight * 1.0f);
            break;
        }
        case 'S':
        {
            mCameraPosition += (mCameraTarget * 1.0f);
            break;
        }
        case 'Z':
        {
            mCameraPosition += (mCameraTarget * (-1.0f));
            break;
        }
        case 'U':
        {
            // quaternion rotation around right axis
            mCameraTarget = glm::rotate(glm::angleAxis(0.05f, mCameraRight), mCameraTarget);
            mCameraUp = glm::cross(mCameraRight, mCameraTarget);
            break;
        }
        case 'M':
        {
            // quaternion rotation around right axis
            mCameraTarget = glm::rotate(glm::angleAxis(-0.05f, mCameraRight), mCameraTarget);
            mCameraUp = glm::cross(mCameraRight, mCameraTarget);
            break;
        }
        case 'H':
        {
            // quaternion rotation around up axis
            mCameraTarget = glm::rotate(glm::angleAxis(0.05f, mCameraUp), mCameraTarget);
            mCameraRight = glm::cross(mCameraTarget, mCameraUp);
            break;
        }
        case 'K':
        {
            // quaternion rotation around up axis
            mCameraTarget = glm::rotate(glm::angleAxis(-0.05f, mCameraUp), mCameraTarget);
            mCameraRight = glm::cross(mCameraTarget, mCameraUp);
            break;
        }
        default:
            return false;
    }

    mViewMatrix = glm::lookAt(mCameraPosition,
                            mCameraTarget + mCameraPosition,
                            mCameraUp);
    return true;
}

} // namespace glsp

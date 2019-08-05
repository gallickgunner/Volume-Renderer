#ifndef CAMERA_H
#define CAMERA_H

#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"

#include <vector>

class Camera
{
    public:
        Camera();
        Camera(float y_FOV, float rot_speed = 0.5f, float mov_speed = 0.3f);
         ~Camera();

        void setOrientation(float zoom, float zenith, float azimuth);
        void setViewMatrix(glm::vec4 eye, glm::vec4 side, glm::vec4 up, glm::vec4 look_at);
        void setUBO(std::vector<float>& cam_data);
        bool is_changed;
        glm::vec4 look_at;
        glm::vec4 side;
        glm::vec4 up;
        glm::vec4 eye;

    private:
        float view_plane_dist, y_FOV,
        rotation_speed, mov_speed, zenith, azimuth, radius;
        glm::mat4 view2world_mat;

};

#endif // CAMERA_H

#include "Camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/constants.hpp"

#include <iostream>
#include <algorithm>
#include <math.h>

Camera::Camera()
{
    //ctor
}

Camera::Camera(float y_FOV, float rot_speed, float mov_speed) : y_FOV(y_FOV), rotation_speed(rot_speed), mov_speed(mov_speed)
{

    view_plane_dist =  1/tan(y_FOV * glm::pi<float>()/360);
    is_changed = true;
    resetCamera();
    //ctor
}

Camera::~Camera()
{

}

void Camera::resetCamera()
{
    view2world_mat = glm::mat4(1.0f);
    rot_mat = glm::mat4(1.0f);
    setViewMatrix(glm::vec4(0, 0, 3, 1),
                   glm::vec4(1, 0, 0, 0),
                   glm::vec4(0, 1, 0, 0),
                   glm::vec4(0, 0, -1, 0)
                   );
    zenith = glm::pi<float>()/2.0;
    azimuth = 0;
    tot_zenith = 0;
    tot2_azimuth = tot_azimuth =0;
    radius = 3;
}

void Camera::setViewMatrix(glm::vec4 eye, glm::vec4 side, glm::vec4 up, glm::vec4 look_at)
{
    this->eye = eye;
    this->side = glm::normalize(side);
    this->up = glm::normalize(up);
    this->look_at = glm::normalize(look_at);

    /* Always Remember that for Right Handed Coordinate Systems, the Camera (initially aligned with World Reference frame)
     * has the look direction negative to that of the Z axis. Hence to get the basis vector in Z we have to invert the look vector.
     */
    view2world_mat = glm::mat4(side, up, -look_at, eye);
}

void Camera::setUBO(std::vector<float>& cam_data)
{
    for(int i = 0; i < 21; i+=4)
    {
        if(i == 16)
        {
            cam_data.push_back(eye.x);
            cam_data.push_back(eye.y);
            cam_data.push_back(eye.z);
            cam_data.push_back(1);

            cam_data.push_back(this->view_plane_dist);
            return;
        }
        glm::vec4 temp = glm::column(view2world_mat, i/4);
        cam_data.push_back(temp.x);
        cam_data.push_back(temp.y);
        cam_data.push_back(temp.z);
        cam_data.push_back(temp.w);
    }
    is_changed = false;
}


void Camera::setOrientation(float zoom , float zenith, float azimuth)
{
    if(zenith == 0 && azimuth == 0)
    {
        if(zoom > 0)
            eye += look_at;
        else
            eye -= look_at;
        radius = glm::length(glm::vec3(eye.x, eye.y, eye.z));
        view2world_mat = glm::column(view2world_mat, 3, eye);
        is_changed = true;
        return;
    }
    float new_zenith, new_azimuth;
    float pi2 = glm::pi<float>() * 2;

    new_zenith = glm::clamp(this->zenith + zenith * rotation_speed, 0.0f, glm::pi<float>());
    new_azimuth = this->azimuth + azimuth * rotation_speed;

    //Wrap Azimuth angle around 360 degrees.
    if(new_azimuth < 0)
        new_azimuth = pi2 - new_azimuth;
    else if(new_azimuth > pi2)
        new_azimuth = new_azimuth - pi2;

    //If no change in angle return
    if(new_zenith == this->zenith && new_azimuth == this->azimuth)
        return;

    this->zenith = new_zenith;
    this->azimuth = new_azimuth;

    /* Spherical to Cartesian. The Cartesian Coordinate Reference Frame used is the one used in Computer Graphics ( Z points forward, Y upwards)
     * rather than normal Mathematics where Z points upwards.
     *
     *  y = r * cos(zenith)
     *  z = r * sin(zenith) * cos(azimuth)
     *  x = r * sin(zenith) * sin(azimuth)
     */
    eye.x = this->radius * glm::sin(this->zenith) * glm::sin(this->azimuth);
    eye.y = this->radius * glm::cos(this->zenith);
    eye.z = this->radius * glm::sin(this->zenith) * glm::cos(this->azimuth);
    eye.w = 1;


    look_at = glm::vec4(-eye);
    look_at.w = 0;
    look_at = glm::normalize(look_at);

    /* When Zenith is 0 or PI, the look at vector collapses on to the world up.
     * We compute the side vector as (1,0,0) and rotate it around the world up based on
     * azimuth angle.
     */
    if(this->zenith == 0 || this->zenith == glm::pi<float>() )
    {
        side = glm::vec4(1,0,0,0);
        glm::mat4 rot_mat = glm::rotate(glm::mat4(1.0), this->azimuth, glm::vec3(0,1,0));
        side = rot_mat * side;
    }
    else
        side = glm::vec4(glm::cross(glm::vec3(look_at.x, look_at.y, look_at.z), glm::vec3(0,1,0)), 0);

    up = glm::vec4(glm::cross(glm::vec3(side.x, side.y, side.z), glm::vec3(look_at.x, look_at.y, look_at.z)), 0);
    side = glm::normalize(side);
    up = glm::normalize(up);

    view2world_mat = glm::mat4(side, up, -look_at, eye);
    is_changed = true;
}

#version 430 core

layout (local_size_x = 16, local_size_y = 16) in;

struct Ray
{
    vec4 origin;
    vec4 dir;
    float length;
    bool is_shadow_ray;
};

struct AABB
{
  vec4 p_min;
  vec4 p_max;
};

const float view_plane_dist = 1.733;
const float EPSILON = 0.000001f;
const float GAMMA = 2.2;
float x_ratio = 1.0, y_ratio = 1.0, z_ratio = 1.0;

AABB bb = AABB(vec4(0,0,0,1), vec4(1,1,1,1));
ivec3 vol_size;
vec4 half_len = vec4(0,0,0,1);

layout(binding = 1, std140) uniform Camera     //    16
{
    mat4 view_mat;                             //    16                 0   (c1)
                                               //    16                 16  (c2)
                                               //    16                 32  (c3)
                                               //    16                 48  (c4)
    vec4 eye;                                  //    16                 64
    float view_plane_dist;                     //    4                  80
} main_cam;

layout(location = 0) uniform float alpha_scale;
layout(location = 1) uniform vec3 voxel_size;
layout(location = 2) uniform int min_val;
layout(location = 3) uniform int max_val;
layout(location = 4) uniform int is_MIP;
layout(location = 5) uniform int view_top;
layout(location = 6) uniform int view_bottom;

layout(binding = 0, rgba32f) uniform image2D render_texture;
layout(binding = 1) uniform usampler3D vol_tex3D;

void computeRay(float pixel_x, float pixel_y, int img_width, int img_height, out Ray eye_ray);
bool intersectRayAABB(Ray ray, AABB bb, out float t_min, out float t_max);
vec4 rayMarchVolume(Ray eye_ray, float t_min, float t_max);
vec4 MIP(Ray eye_ray, float t_min, float t_max);
vec3 cartesianToTextureCoord(vec4 point);

void main()
{
    ivec2 img_size = imageSize(render_texture);    
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    if (pix.x >= img_size.x || pix.y >= img_size.y)
        return;
    
    vol_size = textureSize(vol_tex3D,0);
    
    // Normalize Bounding box from arbitrary xyz size to 0 to aspect ratio range
    int max_dim = max(vol_size.x, vol_size.y);
    max_dim = max(max_dim, vol_size.z);
    
    if(view_bottom == 1 || view_top == 1)
        bb.p_max = vec4(vol_size.xzy, 1.0);
    else
        bb.p_max = vec4(vol_size.xyz, 1.0);
    
    bb.p_max /= max_dim;
    
    if(view_bottom == 1 || view_top == 1)
        bb.p_max *= vec4(voxel_size.xzy,1.0);
    else
        bb.p_max *= vec4(voxel_size.xyz,1.0);
        
    //Align bounding box in the center of the screen.    
    half_len = vec4(bb.p_max.xyz/2.0, 0.0);  
    bb.p_min -= half_len;
    bb.p_max -= half_len;
    
    Ray eye_ray;
    computeRay(pix.x + 0.5, pix.y + 0.5, img_size.x, img_size.y, eye_ray);
    float t_max, t_min;
    
    if(intersectRayAABB(eye_ray, bb, t_min, t_max))
    {
        vec4 color;
        if(is_MIP == 1)
            color = MIP(eye_ray, t_min, t_max);
        else
            color = rayMarchVolume(eye_ray, t_min, t_max);
        imageStore(render_texture, pix, color);
    }
    else
    {
        imageStore(render_texture, pix, vec4(0.0f));
    }
}

vec4 rayMarchVolume(Ray eye_ray, float t_min, float t_max)
{    
    float step_size;
    vec4 start_point = eye_ray.origin + (eye_ray.dir * t_min);
    vec4 end_point = eye_ray.origin + (eye_ray.dir * t_max);        
    step_size = length(bb.p_max.xyz - bb.p_min.xyz) /  length(vol_size.xzy); 
    
    vec4 dest = vec4(0.0);
    vec4 src = vec4(0.0);
    uvec4 sample_tex = uvec4(0);
    vec4 pos = start_point + eye_ray.dir * EPSILON;  
    for(int i = 0; i < 10000; i++)
    {
        vec3 tex_coord = cartesianToTextureCoord(pos);        
        if( any(greaterThan(tex_coord, vec3(1.0))) || any(lessThan(tex_coord, vec3(0.0))) || dest.a >= 0.95)
            break;
        
        src = vec4(texture(vol_tex3D, tex_coord).r);
        src = clamp(src, vec4(min_val), vec4(max_val)); 
        if(src.a <= max_val && src.a >= min_val)
            src = (src - min_val) /(max_val - min_val);       
        
        /** We can set colors manually for a range of isovalues after visualizing the histogram like so (x and y are the control points)
            if(src.r * 255.0 >= x && src.r *255.0 <= y)
                src.rgb = vec3(color.x, color.y, color.z);            
        */
        src.a *= alpha_scale;            
        src.rgb *= src.a;
        dest += src * (1 - dest.a);
        
        if(dest.a > 0.99)
            break;
        pos += eye_ray.dir * step_size;		
    }
    return dest;
}

vec4 MIP(Ray eye_ray, float t_min, float t_max)
{
    float step_size;
    vec4 start_point = eye_ray.origin + (eye_ray.dir * t_min);
    vec4 end_point = eye_ray.origin + (eye_ray.dir * t_max);        
    step_size = length(bb.p_max.xyz - bb.p_min.xyz) /  length(vol_size.xyz); 
    
    vec4 dest = vec4(0.0);
    vec4 src = vec4(0.0);
    uvec4 sample_tex = uvec4(0);
    vec4 pos = start_point + eye_ray.dir * EPSILON; 
    //MIP
    for(int i = 0; i < 10000; i++)
    {
        vec3 tex_coord = cartesianToTextureCoord(pos);        
        if( any(greaterThan(tex_coord, vec3(1.0))) || any(lessThan(tex_coord, vec3(0.0))) || dest.a >= 0.95)
            break;
        
        src = vec4(texture(vol_tex3D, tex_coord).r);
        src = clamp(src, vec4(min_val), vec4(max_val)); 
        if(src.a <= max_val && src.a >= min_val)
            src = (src - min_val) /(max_val - min_val);  
        
        src *= alpha_scale;
        if(dest.a < src.a)
        {
            dest = src;                    
        }
        pos += eye_ray.dir * step_size;
    }
    
    return dest;
}

vec3 cartesianToTextureCoord(vec4 point)
{
    //Since the BB was aligned in the center we need to remap the coordinates back in 0-1 range
    point += half_len;
    point /= (bb.p_max + half_len);   
    
    /* Since (1,1,1) is supposed to be the back of the volume but in reality it's actually the front side, (right handed coord system)
       we need to inverse it so we dont sample from the back
     * when at the start of the bounding box
     */
    point.z = 1 - point.z; 
    if(view_top == 1)
        return vec3(point.x, 1 - point.z, point.y);
    else if(view_bottom == 1)
        return vec3(point.x, point.z, 1 - point.y);
    else
        return point.xyz;
}

void computeRay(float pixel_x, float pixel_y, int img_width, int img_height, out Ray eye_ray)
{
    float x, y, z, aspect_ratio;
    aspect_ratio = (img_width*1.0)/img_height;
	
	x = aspect_ratio *((2.0 * pixel_x/img_width) - 1);
    y = (2.0 * pixel_y/img_height) -1 ;
    z = -main_cam.view_plane_dist;		
	
	//Orthographic Projection
	/*
	eye_ray.dir = -main_cam.view_mat[2];
	eye_ray.origin = vec4(x,y,z,0);
	eye_ray.origin = main_cam.view_mat * vec4(eye_ray.origin);
	eye_ray.origin.w = 1;	
	eye_ray.length = 99999999.9;*/
	
    eye_ray.dir = normalize(vec4(x,y,z,0));
    eye_ray.dir = normalize((main_cam.view_mat * vec4(eye_ray.dir)));    
    eye_ray.origin = main_cam.eye;
    
    eye_ray.length = 99999999.9;
}

bool intersectRayAABB(Ray ray, AABB bb, out float t_min, out float t_max)
{
    t_max = 1.0/0.0, t_min = -t_max;
    vec3 dir_inv =  1 / ray.dir.xyz;
	
	vec3 min_diff = (bb.p_min - ray.origin).xyz * dir_inv;
    vec3 max_diff = (bb.p_max - ray.origin).xyz * dir_inv;
	
	t_min = max(t_min, min(min_diff.x, max_diff.x));
    t_max = min(t_max, max(min_diff.x, max_diff.x));
	
	t_min = max(t_min, min(min_diff.y, max_diff.y));
    t_max = min(t_max, max(min_diff.y, max_diff.y));
	if(t_max < t_min)
		return false;
	
	t_min = max(t_min, min(min_diff.z, max_diff.z));
    t_max = min(t_max, max(min_diff.z, max_diff.z));
	
    return(t_max > max(t_min, 0));
}

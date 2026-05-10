
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include <sdf_constants.h>

typedef struct {
    float perf;
    float score;
    float episode_return;
    float episode_length;
    float n;
} Log;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float w;
    float x;
    float y;
    float z;
} Quat;

typedef struct {
    Vec3 pos;
    Quat quat;
} DroneState;

typedef struct {
    Log log;
    DroneState DroneState;
    float* observations;
    float* actions;
    float* rewards;
    float* terminals;

    int num_agents;
    int tick;
    int max_ticks;
    float episode_return;

    unsigned int rng;
    // Add AUV state here.
} AUVSDF;

inline float sdf_ocean_surface(float y){
return y;
}

inline float sdf_ocean_floor(float y){
return ocean_floor_base - y;
}

inline float map_ocean(float x, float y, float z){
    // 1. Call your inline primitives
    float dist_surface = sdf_ocean_surface(y);
    float dist_floor = sdf_ocean_floor(y);
    
    // 2. Combine them. 
    // fmaxf combines signed SDFs for the bounded ocean volume.
    return fmaxf(dist_surface, dist_floor);
}

inline Vec3 get_drone_unit_vector(AUVSDF* env){

    //Typical robotics convention is define foward along the
    //positive X-axis in local or body frame xyz, 1, 0, 0
    float qw = env->DroneState.quat.w;
    float qx = env->DroneState.quat.x;
    float qy = env->DroneState.quat.y;
    float qz = env->DroneState.quat.z;

    Vec3 forward;
    forward.x = 1.0f - 2.0f * (qy * qy + qz * qz);
    forward.y = 2.0f * (qx * qy + qw * qz);
    forward.z = 2.0f * (qx * qz - qw * qy);

    return forward;
}

inline float sphere_marching_distance(AUVSDF* env, Vec3* unit_vec){
    float total_dist = 0.0f;

    float x = env->DroneState.pos.x;
    float y = env->DroneState.pos.y;
    float z = env->DroneState.pos.z;

    for (int i = 0; i < max_jumps; i++) {
        float step_dist = map_ocean(x, y, z);
        if (fabsf(step_dist) < min_dist) {
            break;
        }

        float step_size = fabsf(step_dist);
        total_dist += step_size;

        x += step_size * unit_vec->x;
        y += step_size * unit_vec->y;
        z += step_size * unit_vec->z;
    }

    return total_dist;
}

//Now we need to write the sphere marching math here

void render(AUVSDF* env){
    float x = env->DroneState.pos.x;
    float y = env->DroneState.pos.y;
    float z = env->DroneState.pos.z;

    for (int pixel_y = 0; pixel_y < height; pixel_y++){
        for (int pixel_x = 0; pixel_x < width; pixel_x++){
            float u = (pixel_x - width / 2.0f) / (height / 2.0f);
            float v = (height / 2.0f - pixel_y) / (height / 2.0f);

            Vector3 raw_dir = {u, v, focal_length};

            //Vector3 is a raylib struct that looks like this. This is why we can use raw_dir.x for math below
            // typedef struct Vector3 {
            //     float x;
            //     float y;
            //     float z;
            // } Vector3;
            //
            // So this:
            // Vector3 raw_dir = {u, v, focal_length};
            //
            // means:
            // raw_dir.x = u;
            // raw_dir.y = v;
            // raw_dir.z = focal_length;
            //
            // That is why this works:

            //First we calcualte then length and then we divide by the length to normalize
            float length = sqrtf(raw_dir.x * raw_dir.x + raw_dir.y * raw_dir.y + raw_dir.z * raw_dir.z);
            Vec3 ray_dir = {
            raw_dir.x / length,
            raw_dir.y / length,
            raw_dir.z / length};

            float distance = sphere_marching_distance(env, &ray_dir);

            // 1. Calculate Intensity (1.0 = right in your face, 0.0 = lost in the dark)            
            float intensity =1.0f - (distance / max_dist_view);
            // Clamp it so math errors don't wrap our colors around
            if (intensity < 0.0f) intensity = 0.0f; 
            if (intensity > 1.0f) intensity = 1.0f;
            // 2. Create the Underwater Color
            // Multiply our base RGB values by the intensity
            unsigned char r = (unsigned char)(intensity * 0);   // No red underwater
            unsigned char g = (unsigned char)(intensity * 150); // Some green
            unsigned char b = (unsigned char)(intensity * 200); // Lots of blue

            Color pixel_color = {r, g, b, 255};
            // 3. Draw it to the screen!
            DrawPixel(pixel_x, pixel_y, pixel_color);
        }
    }

    // REQUIRED BY RAYLIB: Swap the buffers and actually display the image
    EndDrawing();
}

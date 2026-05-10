
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "sdf_constants.h"
#include "noise.h"

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
    fnl_state seafloor_noise;
    fnl_state surface_noise;

    unsigned int rng;
    // Add AUV state here.
} AUVSDF;

static inline float ocean_surface_height(AUVSDF* env, float x, float z){
    float phase_noise = fnlGetNoise2D(&env->surface_noise, x, z) * ocean_surface_noise_phase;
    float wave_a = sinf((x * ocean_surface_wave_frequency) + phase_noise);
    float wave_b = sinf(((x * 0.55f + z * 0.85f) * ocean_surface_wave_frequency) - phase_noise);
    return ocean_surface_wave_amplitude * 0.5f * (wave_a + wave_b);
}

static inline float sdf_ocean_surface(AUVSDF* env, float x, float y, float z){
return y - ocean_surface_height(env, x, z);
}

static inline float sdf_ocean_floor(AUVSDF* env, float x, float y, float z){
    float noise_val = fnlGetNoise2D(&env->seafloor_noise, x, z);
    float dynamic_floor_height = ocean_floor_base + (noise_val * ocean_floor_noise_amplitude);
    return dynamic_floor_height - y;
}

static inline float map_ocean(AUVSDF* env, float x, float y, float z){
    // 1. Call your inline primitives
    float dist_surface = sdf_ocean_surface(env, x, y, z);
    float dist_floor = sdf_ocean_floor(env, x, y, z);
    
    // 2. Combine them. 
    // fmaxf combines signed SDFs for the bounded ocean volume.
    return fmaxf(dist_surface, dist_floor);
}

static inline Vec3 get_drone_unit_vector(AUVSDF* env){

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

//Now we need to write the sphere marching math here
static inline float sphere_marching_distance(AUVSDF* env, Vec3* unit_vec){
    float total_dist = 0.0f;

    float x = env->DroneState.pos.x;
    float y = env->DroneState.pos.y;
    float z = env->DroneState.pos.z;

    for (int i = 0; i < max_jumps; i++) {
        float step_dist = map_ocean(env, x, y, z);
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

// Calculate the surface normal at a specific point in the SDF
static inline Vec3 calc_normal(AUVSDF* env, float x, float y, float z) {
    float eps = 0.001f; // A tiny offset
    
    // Finite difference: sample the SDF slightly forward and backward on each axis
    float nx = map_ocean(env, x + eps, y, z) - map_ocean(env, x - eps, y, z);
    float ny = map_ocean(env, x, y + eps, z) - map_ocean(env, x, y - eps, z);
    float nz = map_ocean(env, x, y, z + eps) - map_ocean(env, x, y, z - eps);
    
    // Normalize the result to get a unit vector representing the surface direction
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    Vec3 normal = {nx / len, ny / len, nz / len};
    
    return normal;
}


void c_render(AUVSDF* env){
    if (!IsWindowReady()) {
        InitWindow(width, height, "PufferLib AUV SDF");
        SetTargetFPS(30);
    }

    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground(BLACK);

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

            // ... (Your existing ray direction code remains identical)
            float distance = sphere_marching_distance(env, &ray_dir);

            Color pixel_color = {0, 12, 28, 255}; // Default to dark water for misses

            if (distance < max_dist_view) {
                // 1. Calculate the exact XYZ coordinate of the surface hit
                float hit_x = env->DroneState.pos.x + ray_dir.x * distance;
                float hit_y = env->DroneState.pos.y + ray_dir.y * distance;
                float hit_z = env->DroneState.pos.z + ray_dir.z * distance;

                // 2. Calculate which way the surface is facing
                Vec3 normal = calc_normal(env, hit_x, hit_y, hit_z);

                // 3. Define a light source (e.g., light shining straight down from the surface)
                Vec3 light_dir = {0.0f, 1.0f, 0.0f}; // Normalize this if using angled light
                
                // 4. Calculate Diffuse Lighting (Dot Product)
                float diffuse = (normal.x * light_dir.x) + (normal.y * light_dir.y) + (normal.z * light_dir.z);
                diffuse = fabsf(diffuse);
                
                // 5. Calculate your original Depth Fog (so things fade out in the distance)
                float fog = 1.0f - (distance / max_dist_view);
                if (fog < 0.15f) fog = 0.15f;
                if (fog > 1.0f) fog = 1.0f;

                // 6. Combine Lighting and Fog
                float final_intensity = (0.35f + 0.65f * diffuse) * fog;

                // Multiply base RGB by our new mathematically accurate intensity
                unsigned char r = (unsigned char)(final_intensity * 0);   
                unsigned char g = (unsigned char)(final_intensity * 150); 
                unsigned char b = (unsigned char)(final_intensity * 200); 

                pixel_color = (Color){r, g, b, 255};
            }

            // Draw it to the screen!
            DrawPixel(pixel_x, pixel_y, pixel_color);
        }
    }

    // REQUIRED BY RAYLIB: Swap the buffers and actually display the image
    EndDrawing();
}

#define FNL_IMPL
#include "auv_sdf.h"

int main() {
    AUVSDF env = {0};
    env.num_agents = 1;
    env.rng = 1;

    env.DroneState.pos = (Vec3){0.0f, -50.0f, 0.0f};
    env.DroneState.quat = (Quat){1.0f, 0.0f, 0.0f, 0.0f};
    env.seafloor_noise = fnlCreateState();
    env.seafloor_noise.seed = 1337;
    env.seafloor_noise.frequency = ocean_floor_noise_frequency;
    env.seafloor_noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    env.seafloor_noise.fractal_type = FNL_FRACTAL_FBM;
    env.seafloor_noise.octaves = 3;
    env.surface_noise = fnlCreateState();
    env.surface_noise.seed = 4242;
    env.surface_noise.frequency = ocean_surface_noise_frequency;
    env.surface_noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    env.surface_noise.fractal_type = FNL_FRACTAL_NONE;

    c_render(&env);

    while (!WindowShouldClose()) {
        c_render(&env);
    }

    CloseWindow();
    return 0;
}

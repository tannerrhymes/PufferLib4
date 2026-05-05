#include "auv_basic.h"

int main() {
    AUVBasic env = {0};
    env.num_agents = 1;
    env.rng = 1;

    env.observations = (float*)calloc(NUM_OBSERVATIONS, sizeof(float));
    env.actions = (float*)calloc(1, sizeof(float));
    env.rewards = (float*)calloc(1, sizeof(float));
    env.terminals = (float*)calloc(1, sizeof(float));

    c_reset(&env);
    c_render(&env);

    while (!WindowShouldClose()) {
        env.actions[0] = NOOP;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) env.actions[0] = UP;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) env.actions[0] = DOWN;

        c_step(&env);
        c_render(&env);
    }

    free(env.observations);
    free(env.actions);
    free(env.rewards);
    free(env.terminals);
    c_close(&env);
    return 0;
}

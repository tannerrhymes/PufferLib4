#pragma once

#include <stdlib.h>
#include <math.h>
#include "raylib.h"

const unsigned char NOOP = 0;
const unsigned char DOWN = 1;
const unsigned char UP = 2;

const unsigned char EMPTY = 0;
const unsigned char AGENT = 1;
const unsigned char OBSTACLE = 2;

#define NUM_OBSTACLES 15
#define NUM_OBSERVATIONS ((NUM_OBSTACLES + 1) * 3)
#define GRID_RES 84
#define MAX_TICKS 500
#define MAX_RADIUS 3

typedef struct {
    float perf;
    float score;
    float episode_return;
    float episode_length;
    float n;
} Log;

typedef struct {
    Log log;
    float* observations;
    float* actions;
    float* rewards;
    float* terminals;

    int num_agents;
    int tick;
    int max_ticks;
    float episode_return;

    int auv_x;
    int auv_y;
    int auv_r;

    int obstacle_x[NUM_OBSTACLES];
    int obstacle_y[NUM_OBSTACLES];
    int radius_obstacle[NUM_OBSTACLES];


    unsigned int rng;
    // Add AUV state here.
} AUVBasic;


void add_log(AUVBasic* env) {
    env->log.perf += (env->rewards[0] > 0) ? 1 : 0;
    env->log.score += env->episode_return;
    env->log.episode_length += env->tick;
    env->log.episode_return += env->episode_return;
    env->log.n++;
}

void c_reset(AUVBasic* env) {
    (void)env;
    // Reset env state and write initial observations.

    env->auv_x = GRID_RES/4;
    env->auv_y = GRID_RES/2;
    env->auv_r = 2;
    env->tick = 0;
    env->max_ticks = MAX_TICKS;
    env->episode_return = 0.0f;

    int max_radius = MAX_RADIUS;
    for (int i = 0; i < NUM_OBSTACLES; i++){
        int safe_spawn = 0;
        while (!safe_spawn){
            env->radius_obstacle[i] = 1 + rand_r(&env->rng) % max_radius;
            env->obstacle_x[i] = rand_r(&env->rng) % GRID_RES;
            env->obstacle_y[i] = rand_r(&env->rng) % GRID_RES;

            int dx = env->auv_x - env->obstacle_x[i];
            int dy = env->auv_y - env->obstacle_y[i];
            float distance = sqrtf(dx * dx + dy * dy);
            float collision_dist = env->auv_r + env->radius_obstacle[i] + 10;

            if (distance > collision_dist){
                safe_spawn = 1;
            }
        }
    }
    env->observations[0] = env->auv_x / (float)GRID_RES;
    env->observations[1] = env->auv_y / (float)GRID_RES;
    env->observations[2] = env->auv_r / (float)MAX_RADIUS;

    for (int i = 0; i < NUM_OBSTACLES; i++){
        int obs_idx_start = 3 + i * 3;
        
        int idx_obstacle_x = obs_idx_start;
        int idx_obstacle_y = obs_idx_start + 1;
        int idx_obstacle_r = obs_idx_start + 2;

        env->observations[idx_obstacle_x] = (env->obstacle_x[i] - env->auv_x) / (float)GRID_RES;
        env->observations[idx_obstacle_y] = (env->obstacle_y[i] - env->auv_y) / (float)GRID_RES;
        env->observations[idx_obstacle_r] = env->radius_obstacle[i] / (float)max_radius;
    }

}

void c_step(AUVBasic* env) {
    (void)env;
    // Read actions, advance state, write observations/rewards/terminals.
    env->tick += 1;
    int action = (int)env->actions[0]; //action equals the action stored in the struct, our nn would have overwritten the previous value
    env->terminals[0] = 0; //Set these to 0 for this time step
    env->rewards[0] = 0.001f; //default reward for staying alive this timestep

    if (action == UP){ //action for move up
        env->auv_y += 1;
    }
    if (action == DOWN){ //action for move down
        env->auv_y -= 1;
    }

    for (int i = 0; i < NUM_OBSTACLES; i++){ //move obstacles right and reset them when they move off screen
        env->obstacle_x[i] -= 1; 

        if (env->obstacle_x[i] < 0){
            env->obstacle_x[i] += GRID_RES;
            env->obstacle_y[i] = rand_r(&env->rng) % GRID_RES;
        }
    }

    int hit_obstacle = 0;
    for (int i = 0; i < NUM_OBSTACLES; i++){
        int dx = env->auv_x - env->obstacle_x[i];
        int dy = env->auv_y - env->obstacle_y[i];
        float distance = sqrtf(dx * dx + dy * dy);
        float collision_dist = env->auv_r + env->radius_obstacle[i];

        if (distance < collision_dist){
            hit_obstacle = 1;
            break;
        }
    }

    if (env->auv_y >= GRID_RES
            || env->auv_y < 0
            || hit_obstacle == 1){
        env->terminals[0] = 1;
        env->rewards[0] = -1.0;
        env->episode_return += env->rewards[0];
        add_log(env);
        c_reset(env);
        return;
        }

    if (env->tick > env->max_ticks){
        env->terminals[0] = 1;
        env->rewards[0] = 1.0;
        env->episode_return += env->rewards[0];
        add_log(env);
        c_reset(env);
        return;
    }

    env->episode_return += env->rewards[0];

    int max_radius = MAX_RADIUS;
    env->observations[0] = env->auv_x / (float)GRID_RES;
    env->observations[1] = env->auv_y / (float)GRID_RES;
    env->observations[2] = env->auv_r / (float)MAX_RADIUS;

    for (int i = 0; i < NUM_OBSTACLES; i++){
        int obs_idx_start = 3 + i * 3;
        
        int idx_obstacle_x = obs_idx_start;
        int idx_obstacle_y = obs_idx_start + 1;
        int idx_obstacle_r = obs_idx_start + 2;

        env->observations[idx_obstacle_x] = (env->obstacle_x[i] - env->auv_x) / (float)GRID_RES;
        env->observations[idx_obstacle_y] = (env->obstacle_y[i] - env->auv_y) / (float)GRID_RES;
        env->observations[idx_obstacle_r] = env->radius_obstacle[i] / (float)max_radius;
    }
}

// Required function. Should handle creating the client on first call
void c_render(AUVBasic* env) {
    int px = 8;

    if (!IsWindowReady()) {
        InitWindow(px*GRID_RES, px*GRID_RES, "PufferLib AUV Basic");
        SetTargetFPS(30);
    }

    // Standard across our envs so exiting is always the same
    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground((Color){6, 24, 24, 255});

    DrawCircle(env->auv_x*px, env->auv_y*px, env->auv_r*px, (Color){0, 187, 187, 255});

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        DrawCircle(
            env->obstacle_x[i]*px,
            env->obstacle_y[i]*px,
            env->radius_obstacle[i]*px,
            (Color){187, 0, 0, 255});
    }

    EndDrawing();
}

void c_close(AUVBasic* env) {
    (void)env;
    if (IsWindowReady()) {
        CloseWindow();
    }
}

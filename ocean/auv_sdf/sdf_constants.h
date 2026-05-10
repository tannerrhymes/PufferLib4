
//This is where we will define constants for the sdf equation
//We will also define units here, we are going to be working in meters
//This will be important for our physics calcs

const float ocean_floor_base = -100.0;
const float ocean_floor_noise_frequency = 0.05f;
const float ocean_floor_noise_amplitude = 5.0f;
const float ocean_surface_wave_amplitude = 2.0f;
const float ocean_surface_wave_frequency = 0.8f;
const float ocean_surface_noise_frequency = 0.02f;
const float ocean_surface_noise_phase = 1.5f;

//Raylib Constants for how we visualize
const int width = 1920;
const int height = 1080;
const float focal_length = 1.0f; // Roughly 90-degree FOV
const float max_dist_view = 500.0;

//Sphere marching constants
const int max_jumps = 10;
const float min_dist = 0.01;

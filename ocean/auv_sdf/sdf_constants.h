
//This is where we will define constants for the sdf equation
//We will also define units here, we are going to be working in meters
//This will be important for our physics calcs

const float ocean_floor_base = -100.0;

//Raylib Constants for how we visualize
const int width = 1920;
const int height = 1080;
const float focal_length = 1.0f; // Roughly 90-degree FOV
const float max_dist_view = 200.0;

//Sphere marching constants
const int max_jumps = 10;
const float min_dist = 0.01;
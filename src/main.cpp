#include <iostream>
#include <signal.h>

// ZED includes
#include <sl/Camera.hpp>

using namespace std;

sl::Camera zed;
sl::Pose pose;      // sl::Pose to hold pose data
sl::Mesh mesh;      // sl::Mesh to hold the mesh generated during spatial mapping
sl::SpatialMappingParameters spatial_mapping_params;
sl::MeshFilterParameters filter_params;
sl::TRACKING_STATE tracking_state;


std::chrono::high_resolution_clock::time_point t_last;
volatile sig_atomic_t flag = 0; // Flag for SIGINT


void zedLoop();
void startMapping();
void stopMapping();
void exiti(int sig);

int main(int argc, char** argv) {
    // Register signal SIGINT for Ctrl+C handling
    signal(SIGINT, exiti);

    // Setup configuration parameters for the ZED
    sl::InitParameters parameters;
    if (argc > 1) parameters.svo_input_filename = argv[1];

    parameters.depth_mode = sl::DEPTH_MODE_PERFORMANCE; // Use QUALITY depth mode to improve mapping results
    parameters.coordinate_units = sl::UNIT_METER;
    parameters.coordinate_system = sl::COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // OpenGL coordinates system

    // Open the ZED
    sl::ERROR_CODE err = zed.open(parameters);
    if (err != sl::ERROR_CODE::SUCCESS) {
        std::cout << sl::toString(err) << std::endl;
        zed.close();
        return -1;
    }

    // Configure Spatial Mapping and filtering parameters
    spatial_mapping_params.range_meter = sl::SpatialMappingParameters::get(sl::SpatialMappingParameters::MAPPING_RANGE_FAR);
    spatial_mapping_params.resolution_meter = sl::SpatialMappingParameters::get(sl::SpatialMappingParameters::MAPPING_RESOLUTION_LOW);
    spatial_mapping_params.save_texture = false;
    spatial_mapping_params.max_memory_usage = 1024;
    spatial_mapping_params.use_chunk_only = true;

    filter_params.set(sl::MeshFilterParameters::MESH_FILTER_LOW);


    startMapping();
    zedLoop();
    stopMapping();


    zed.close();
    return 0;
}


void exiti(int sig){ // can be called asynchronously
  flag = 1; // set flag
}

void zedLoop() {
    while(!flag) {
        if(zed.grab() == sl::SUCCESS) {
            tracking_state = zed.getPosition(pose);

            // Compute elapse time since the last call of sl::Camera::requestMeshAsync()
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t_last).count();
            // Ask for a mesh update if 500ms have spend since last request
            if (duration > 500) {
                zed.requestMeshAsync();
                t_last = std::chrono::high_resolution_clock::now();
            }

            if (zed.getMeshRequestStatusAsync() == sl::SUCCESS) {
                // Get the current mesh generated
                if (zed.retrieveMeshAsync(mesh) == sl::SUCCESS) {
                    for (int c = 0; c < mesh.chunks.size(); c++) {
                        // If the chunck has been updated by the spatial mapping, update it for rendering
                        if (mesh.chunks[c].has_been_updated) {

                        }
                    }
                }
            }
        }
    }
}

/**
Start the spatial mapping process
**/
void startMapping() {
    // clear previously used objects
    mesh.clear();

    // Enable positional tracking before starting spatial mapping
    zed.enableTracking();
    // Enable spatial mapping
    zed.enableSpatialMapping(spatial_mapping_params);

    // Start a timer, we retrieve the mesh every XXms.
    t_last = chrono::high_resolution_clock::now();

    cout << "** Spatial Mapping is started ... **" << endl;
}

/**
Stop the spatial mapping process
**/
void stopMapping() {
    // Stop the mesh request and extract the whole mesh to filter it and save it as an obj file
    std::cout << "** Stop Spatial Mapping ... **" << std::endl;

    // Extract the whole mesh
    sl::Mesh wholeMesh;
    zed.extractWholeMesh(wholeMesh);
    std::cout << ">> Mesh has been extracted..." << std::endl;

    // Filter the extracted mesh     |chunks true
    wholeMesh.filter(filter_params, true);
    std::cout << ">> Mesh has been filtered..." << std::endl;

    // If textures have been saved during spatial mapping, apply them to the mesh
    if (spatial_mapping_params.save_texture) {
        wholeMesh.applyTexture(sl::MESH_TEXTURE_RGB);
        std::cout << ">> Mesh has been textured..." << std::endl;
    }

    //Save as an OBJ file
    std::string saveName = "./mesh_gen.obj";
    bool t = wholeMesh.save(saveName.c_str());
    if (t) std::cout << ">> Mesh has been saved under " << saveName << std::endl;
    else std::cout << ">> Failed to save the mesh under  " << saveName << std::endl;
}

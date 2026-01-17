/**
 * @file model_reader.cpp
 * @brief Implementation of ModelReader service for 3D model file processing
 */

#include "io/model_reader.hpp"
#include "kangaroo/util/current_thread.hpp"
#include "util/logger.hpp"

#include <algorithm>

namespace OpenGeoLab::IO {

nlohmann::json ModelReader::processRequest(const std::string& module_name,
                                           const nlohmann::json& params,
                                           App::IProgressReporterPtr progress_reporter) {
    nlohmann::json result;
    int counter = 0;

    LOG_TRACE("import test: trace");
    LOG_DEBUG("import test: debug");
    LOG_INFO("import test: info");
    LOG_WARN("import test: warn");
    LOG_ERROR("import test: error");
    LOG_CRITICAL("import test: critical");
    const bool fast_mode = params.value("fast", false);
    int max_steps = params.value("max_steps", 100);
    int sleep_usec = params.value("sleep_usec", 100000);
    if(fast_mode) {
        // Test-friendly: finish quickly.
        sleep_usec = 0;
        max_steps = std::min(max_steps, 5);
    }
    if(max_steps <= 0) {
        max_steps = 1;
    }

    // Test long messages at different progress stages
    const std::vector<std::string> test_messages = {
        "Initializing model reader and preparing file system access for the specified 3D model "
        "file path. This operation may take a moment depending on file size and system "
        "performance...",
        "Parsing geometric primitives: vertices, edges, faces, and wireframe topology. "
        "Validating mesh integrity and checking for degenerate triangles or non-manifold "
        "edges in the input model data structure...",
        "Processing material properties, texture coordinates, and surface normals. Applying "
        "transformation matrices and computing bounding boxes for spatial indexing and "
        "collision detection preparation...",
        "Optimizing mesh data for GPU rendering: generating vertex buffer objects, index "
        "buffers, and level-of-detail representations. Compressing texture data and "
        "pre-computing ambient occlusion maps...",
        "Finalizing import operation: validating output data consistency, updating scene "
        "graph hierarchy, and preparing undo/redo state snapshot for the imported model "
        "geometry and associated metadata..."};

    while(!progress_reporter->isCancelled()) {
        double progress = static_cast<double>(counter) / static_cast<double>(max_steps);

        // Select message based on progress stage
        size_t message_index =
            std::min(static_cast<size_t>(counter / 20), test_messages.size() - 1);
        progress_reporter->reportProgress(progress, test_messages[message_index]);

        if(sleep_usec > 0) {
            Kangaroo::Util::CurrentThread::sleepUsec(static_cast<uint64_t>(sleep_usec));
        }

        if(progress_reporter->isCancelled()) {
            throw std::runtime_error("Operation was cancelled by user request.");
        }
        if(counter++ >= max_steps) {
            break;
        }
    }

    if(progress_reporter->isCancelled()) {
        progress_reporter->reportError("Operation was cancelled.");
        return nlohmann::json{};
    }
    result["module_name"] = module_name;
    result["params"] = params;
    return result;
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<ModelReader>();
    return singleton_instance;
}

} // namespace OpenGeoLab::IO
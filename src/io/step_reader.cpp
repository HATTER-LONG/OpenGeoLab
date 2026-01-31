/**
 * @file step_reader.cpp
 * @brief Implementation of STEP file reader
 */

#include "step_reader.hpp"
#include "geometry/geometry_document_manager.hpp"
#include "util/logger.hpp"

#include <BRep_Builder.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>

#include <filesystem>

namespace OpenGeoLab::IO {

ReadResult StepReader::readFile(const std::string& file_path,
                                Util::ProgressCallback progress_callback) {
    LOG_TRACE("Reading STEP file: {}", file_path);

    // Validate file existence
    if(!std::filesystem::exists(file_path)) {
        LOG_ERROR("STEP file does not exist: {}", file_path);
        return ReadResult::failure("File not found: " + file_path);
    }

    // Report initial progress
    if(!progress_callback(0.05, "Initializing STEP reader...")) {
        return ReadResult::failure("Operation cancelled");
    }

    try {
        STEPControl_Reader reader;

        // Report progress before reading
        if(!progress_callback(0.10, "Reading STEP file...")) {
            return ReadResult::failure("Operation cancelled");
        }

        // Read the STEP file (no progress support in this OCC version)
        IFSelect_ReturnStatus status = reader.ReadFile(file_path.c_str());
        if(status != IFSelect_RetDone) {
            LOG_ERROR("Failed to read STEP file: {} (status: {})", file_path,
                      static_cast<int>(status));
            return ReadResult::failure("Failed to parse STEP file");
        }

        // Report progress before translation
        if(!progress_callback(0.50, "Translating STEP model...")) {
            return ReadResult::failure("Operation cancelled");
        }

        // Transfer roots (convert STEP entities to OCC shapes)
        int num_roots = reader.TransferRoots();
        if(num_roots == 0) {
            LOG_ERROR("STEP file contains no transferable roots: {}", file_path);
            return ReadResult::failure("No geometry found in STEP file");
        }

        LOG_DEBUG("STEP file contains {} root(s)", num_roots);

        // Report progress before entity creation
        if(!progress_callback(0.70, "Creating geometry entities...")) {
            return ReadResult::failure("Operation cancelled");
        }

        // Get the resulting shape(s)
        TopoDS_Shape result_shape;
        if(num_roots == 1) {
            result_shape = reader.Shape(1);
        } else {
            // Multiple roots: create a compound
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            for(int i = 1; i <= num_roots; ++i) {
                builder.Add(compound, reader.Shape(i));
            }
            result_shape = compound;
        }

        // Validate the result
        if(result_shape.IsNull()) {
            LOG_ERROR("STEP translation produced no valid shape: {}", file_path);
            return ReadResult::failure("Translation produced no geometry");
        }

        // Load shape into current document
        auto document =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>()
                ->currentDocument();
        if(!document) {
            LOG_ERROR("No active geometry document");
            return ReadResult::failure("No active geometry document");
        }

        // Extract filename for part name
        std::filesystem::path path(file_path);
        std::string part_name = path.stem().string();
        auto sub_callback = Util::makeScaledProgressCallback(progress_callback, 0.7, 1.0);
        auto load_result = document->loadFromShape(result_shape, part_name, sub_callback);
        if(!load_result.m_success) {
            LOG_ERROR("Failed to load shape into document: {}", load_result.m_errorMessage);
            return ReadResult::failure("Failed to load geometry: " + load_result.m_errorMessage);
        }
        LOG_INFO("STEP file loaded successfully: {} entities created", load_result.m_entityCount);
        return ReadResult::success(load_result.m_entityCount);

    } catch(const Standard_Failure& e) {
        std::string error = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        LOG_ERROR("OCC exception reading STEP file: {}", error);
        return ReadResult::failure("OpenCASCADE error: " + error);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception reading STEP file: {}", e.what());
        return ReadResult::failure(std::string("Error: ") + e.what());
    }
}

} // namespace OpenGeoLab::IO
/**
 * @file step_reader.cpp
 * @brief Implementation of STEP file reader
 */

#include "step_reader.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/shape_builder.hpp"
#include "util/logger.hpp"
#include "util/occ_progress.hpp"

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
    if(progress_callback && !progress_callback(0.05, "Initializing STEP reader...")) {
        return ReadResult::failure("Operation cancelled");
    }

    try {
        STEPControl_Reader reader;

        // Report progress before reading
        if(progress_callback && !progress_callback(0.10, "Reading STEP file...")) {
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
        if(progress_callback && !progress_callback(0.50, "Translating STEP model...")) {
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
        if(progress_callback && !progress_callback(0.70, "Creating geometry entities...")) {
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

        // Get current document and build entity hierarchy
        auto document = Geometry::GeometryDocumentManager::instance().currentDocument();
        if(!document) {
            LOG_ERROR("No active document for STEP import");
            return ReadResult::failure("No active document");
        }

        // Extract filename for part name
        std::filesystem::path path(file_path);
        std::string part_name = path.stem().string();

        // Build entity hierarchy from shape
        Geometry::ShapeBuilder shape_builder(document);
        auto build_progress = Util::makeScaledProgressCallback(progress_callback, 0.70, 0.95);
        auto build_result = shape_builder.buildFromShape(result_shape, part_name, build_progress);

        if(!build_result.m_success) {
            LOG_ERROR("Failed to build entity hierarchy: {}", build_result.m_errorMessage);
            return ReadResult::failure("Failed to create geometry entities: " +
                                       build_result.m_errorMessage);
        }

        if(progress_callback) {
            progress_callback(1.0, "STEP import completed successfully");
        }

        LOG_INFO("STEP import successful: {} entities created", build_result.totalEntityCount());

        return ReadResult::success(build_result.m_rootPart);

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
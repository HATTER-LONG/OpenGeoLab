/**
 * @file brep_reader.cpp
 * @brief Implementation of BREP file reader
 */

#include "brep_reader.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/shape_builder.hpp"
#include "util/logger.hpp"
#include "util/occ_progress.hpp"

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Shape.hxx>

#include <filesystem>

namespace OpenGeoLab::IO {

ReadResult BrepReader::readFile(const std::string& file_path,
                                Util::ProgressCallback progress_callback) {
    LOG_TRACE("Reading BREP file: {}", file_path);

    // Validate file existence
    if(!std::filesystem::exists(file_path)) {
        LOG_ERROR("BREP file does not exist: {}", file_path);
        return ReadResult::failure("File not found: " + file_path);
    }

    // Report initial progress
    if(progress_callback && !progress_callback(0.05, "Initializing BREP reader...")) {
        return ReadResult::failure("Operation cancelled");
    }

    try {
        TopoDS_Shape shape;
        BRep_Builder builder;

        // Create progress adapter for OCC operations
        auto occ_progress_callback =
            Util::makeScaledProgressCallback(progress_callback, 0.10, 0.50);
        auto occ_progress =
            Util::makeOccProgress(std::move(occ_progress_callback), "Reading BREP", 0.01);
        Message_ProgressRange progress = Message_ProgressIndicator::Start(occ_progress.m_indicator);

        // Read the BREP file
        if(!BRepTools::Read(shape, file_path.c_str(), builder, progress)) {
            if(occ_progress.isCancelled()) {
                return ReadResult::failure("Operation cancelled");
            }
            LOG_ERROR("Failed to read BREP file: {}", file_path);
            return ReadResult::failure("Failed to parse BREP file");
        }

        // Check for cancellation after read
        if(occ_progress.isCancelled()) {
            return ReadResult::failure("Operation cancelled");
        }

        // Validate the loaded shape
        if(shape.IsNull()) {
            LOG_ERROR("BREP file contains no valid shape: {}", file_path);
            return ReadResult::failure("No geometry found in BREP file");
        }

        // Report progress before entity creation
        if(progress_callback && !progress_callback(0.55, "Creating geometry entities...")) {
            return ReadResult::failure("Operation cancelled");
        }

        // Extract part name from file path
        std::filesystem::path p(file_path);
        std::string part_name = p.stem().string();

        // Get or create the current document
        auto doc = Geometry::GeometryDocumentManager::instance().currentDocument();

        // Build entity hierarchy using ShapeBuilder
        Geometry::ShapeBuilder shape_builder(doc);
        auto build_options = Geometry::ShapeBuildOptions::forRendering();

        auto build_result = shape_builder.buildFromShape(
            shape, part_name, build_options, [&](double p, const std::string& msg) {
                if(progress_callback) {
                    // Scale progress from 0.55 to 1.0
                    return progress_callback(0.55 + p * 0.45, msg);
                }
                return true;
            });

        if(!build_result.m_success) {
            LOG_ERROR("Failed to build geometry entities: {}", build_result.m_errorMessage);
            return ReadResult::failure("Failed to create geometry: " + build_result.m_errorMessage);
        }

        LOG_INFO("Successfully read BREP file: {} ({} entities)", file_path,
                 build_result.totalEntityCount());

        return ReadResult::success(build_result.m_rootPart);

    } catch(const Standard_Failure& e) {
        std::string error = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        LOG_ERROR("OCC exception reading BREP file: {}", error);
        return ReadResult::failure("OpenCASCADE error: " + error);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception reading BREP file: {}", e.what());
        return ReadResult::failure(std::string("Error: ") + e.what());
    }
}

} // namespace OpenGeoLab::IO
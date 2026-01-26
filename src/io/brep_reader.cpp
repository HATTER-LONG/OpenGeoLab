/**
 * @file brep_reader.cpp
 * @brief Implementation of BREP file reader
 */

#include "brep_reader.hpp"
#include "geometry/geometry_builder.hpp"
#include "geometry/geometry_document.hpp"
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
            Util::makeScaledProgressCallback(progress_callback, 0.10, 0.60);
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
        if(progress_callback && !progress_callback(0.75, "Creating geometry entities...")) {
            return ReadResult::failure("Operation cancelled");
        }

        // Build entity hierarchy
        auto document = Geometry::GeometryDocumentManager::instance().currentDocument();
        if(!document) {
            document = Geometry::GeometryDocumentManager::instance().newDocument();
        }

        Geometry::GeometryBuilder geometry_builder(document);
        std::string part_name = std::filesystem::path(file_path).stem().string();
        auto build_result = geometry_builder.buildFromShape(
            shape, part_name, [&progress_callback](double progress, const std::string& message) {
                if(!progress_callback) {
                    return true;
                }
                // Scale progress from 0.75 to 0.99
                double scaled_progress = 0.75 + progress * 0.24;
                return progress_callback(scaled_progress, message);
            });

        if(!build_result.m_success) {
            LOG_ERROR("Failed to build entity hierarchy: {}", build_result.m_errorMessage);
            return ReadResult::failure("Entity hierarchy construction failed: " +
                                       build_result.m_errorMessage);
        }

        LOG_INFO("BREP file imported successfully: {}", file_path);
        return ReadResult::success(build_result.m_partEntity);

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
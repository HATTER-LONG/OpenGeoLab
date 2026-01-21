#include "brep_reader.hpp"
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

    if(!std::filesystem::exists(file_path)) {
        LOG_ERROR("Brep file does not exist: {}", file_path);
        return ReadResult::failure("file not found: " + file_path);
    }

    if(progress_callback && !progress_callback(0.1, "Initializing BREP file read...")) {
        return ReadResult::failure("operation cancelled");
    }

    try {
        TopoDS_Shape shape;
        BRep_Builder builder;

        auto occ_progress_callback =
            OpenGeoLab::Util::makeScaledProgressCallback(progress_callback, 0.15, 0.70);
        auto occ_progress = OpenGeoLab::Util::makeOccProgress(std::move(occ_progress_callback),
                                                              "Reading BREP", 0.01);
        Message_ProgressRange progress = Message_ProgressIndicator::Start(occ_progress.m_indicator);

        if(!BRepTools::Read(shape, file_path.c_str(), builder, progress)) {
            if(occ_progress.isCancelled()) {
                return ReadResult::failure("operation cancelled");
            }
            LOG_ERROR("Failed to read BREP file: {}", file_path);
            return ReadResult::failure("failed to read BREP file");
        }
        if(occ_progress.isCancelled()) {
            return ReadResult::failure("operation cancelled");
        }

        if(shape.IsNull()) {
            LOG_ERROR("BREP file contains no valid shape: {}", file_path);
            return ReadResult::failure("No geometry found in BREP file");
        }

        if(progress_callback && !progress_callback(0.85, "Creating geometry entity...")) {
            return ReadResult::failure("operation cancelled");
        }

    } catch(const Standard_Failure& e) {
        std::string error = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        LOG_ERROR("OCC exception reading BREP file: {}", error);
        return ReadResult::failure("OCC error: " + error);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception reading BREP file: {}", e.what());
        return ReadResult::failure(std::string("Error: ") + e.what());
    }
    return ReadResult::success(nullptr);
}

} // namespace OpenGeoLab::IO
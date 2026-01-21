#include "step_reader.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::IO {
ReadResult StepReader::readFile(const std::string& file_path,
                                Util::ProgressCallback progress_callback) {
    LOG_TRACE("Reading STEP file: {}", file_path);
    return ReadResult::success(nullptr);
}

} // namespace OpenGeoLab::IO
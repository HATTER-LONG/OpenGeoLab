#include "step_reader.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::IO {
void StepReader::readFile(const std::string& file_path) {
    LOG_TRACE("Reading STEP file: {}", file_path);
}

} // namespace OpenGeoLab::IO
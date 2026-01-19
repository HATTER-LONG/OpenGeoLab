#include "brep_reader.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::IO {
void BrepReader::readFile(const std::string& file_path) {
    LOG_TRACE("Reading BREP file: {}", file_path);
}

} // namespace OpenGeoLab::IO
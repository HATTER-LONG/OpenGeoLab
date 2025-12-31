#include "io/model_reader.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab {
namespace IO {

bool IModelReader::processRequest(const std::string& action_id, const nlohmann::json& params) {
    if(action_id == "read_model") {
        if(!params.contains("file_path") || !params["file_path"].is_string()) {
            return false;
        }
        const std::string file_path = params["file_path"];
        return readModel(file_path);
    }
    return false;
}

bool IModelReader::readModel(const std::string& file_path) {
    // Placeholder implementation
    LOG_INFO("Reading model from file: {}", file_path);
    return true;
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static auto obj = std::make_shared<IModelReader>();
    return obj;
}

} // namespace IO
} // namespace OpenGeoLab
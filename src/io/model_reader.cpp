#include "io/model_reader.hpp"
#include "kangaroo/util/current_thread.hpp"
namespace OpenGeoLab::IO {
nlohmann::json ModelReader::processRequest(const std::string& module_name,
                                           const nlohmann::json& params,
                                           App::IProgressReporterPtr progress_reporter) {
    // Implementation for processing the request goes here.
    nlohmann::json result;
    int counter = 0;
    while(!progress_reporter->isCancelled()) {
        progress_reporter->reportProgress(counter / 100.0, "Processing...");
        Kangaroo::Util::CurrentThread::sleepUsec(1000000);
        if(progress_reporter->isCancelled()) {
            throw std::runtime_error("Operation was cancelled.");
        }
        if(counter++ >= 100) {
            break;
        }
    }

    if(progress_reporter->isCancelled()) {
        progress_reporter->reportError("Operation was cancelled.");
        return nlohmann::json{};
    }

    // Example: just echoing back the input parameters for demonstration.
    result["module_name"] = module_name;
    result["params"] = params;
    return result;
}

ModelReaderFactory::tObjectSharedPtr ModelReaderFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<ModelReader>();
    return singleton_instance;
}
} // namespace OpenGeoLab::IO
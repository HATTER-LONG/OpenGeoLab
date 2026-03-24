#include <opengeolab/geometry/create_box_action.hpp>

#include <opengeolab/base/notification_registry.hpp>
#include <opengeolab/base/notification_sink.hpp>

#include <chrono>
#include <format>
#include <thread>

namespace OpenGeoLab::Geometry {

BoxData createBox(std::array<double, 3> center,
                  std::array<double, 3> dimensions,
                  int vertex_count,
                  ProgressCallback progress_callback) {
    auto* sink = OpenGeoLab::Base::NotificationRegistry::sink();

    // Notify start
    if(sink != nullptr) {
        sink->notify("geometry.status", R"({"event":"started","action":"create_box"})");
    }

    // Simulate vertex generation with progress
    const int report_interval = std::max(1, vertex_count / 10);
    for(int i = 1; i <= vertex_count; ++i) {
        // Simulate work: 10ms per vertex
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if(i % report_interval == 0 || i == vertex_count) {
            const double progress = static_cast<double>(i) / static_cast<double>(vertex_count);
            const auto message = std::format("Generating vertex {}/{}", i, vertex_count);

            if(progress_callback) {
                progress_callback(progress, message);
            }

            if(sink != nullptr) {
                auto payload = std::format(R"({{"event":"progress","progress":{:.2f},"vertex":{},)"
                                           R"("total":{},"message":"{}"}})",
                                           progress, i, vertex_count, message);
                sink->notify("geometry.progress", payload);
            }
        }
    }

    BoxData result;
    result.center = center;
    result.size = dimensions;
    result.vertexCount = vertex_count;
    result.label = std::format("Box({}x{}x{})", dimensions[0], dimensions[1], dimensions[2]);

    // Notify completion
    if(sink != nullptr) {
        auto payload = std::format(R"({{"event":"completed","action":"create_box","label":"{}"}})",
                                   result.label);
        sink->notify("geometry.status", payload);
    }

    return result;
}

} // namespace OpenGeoLab::Geometry

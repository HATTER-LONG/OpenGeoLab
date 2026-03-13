#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <nlohmann/json.hpp>

int main() {
    ogl::geometry::registerGeometryComponents();

    const auto response = ogl::core::ComponentRequestDispatcher::dispatch(
        "geometry",
        nlohmann::json{{"operation", "placeholderModel"},
                       {"modelName", "SmokeTestModel"},
                       {"bodyCount", 2},
                       {"source", "test"}});

    if(!response.success) {
        return 1;
    }

    const auto model = response.payload.value("model", nlohmann::json::object());
    if(model.value("modelName", std::string{}) != "SmokeTestModel") {
        return 2;
    }

    if(model.value("bodyCount", 0) != 2) {
        return 3;
    }

    return 0;
}
#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <nlohmann/json.hpp>

int main() {
    ogl::selection::registerSelectionComponents();

    const auto response = ogl::core::ComponentRequestDispatcher::dispatch(
        "selection",
        nlohmann::json{{"operation", "pickPlaceholderEntity"},
                       {"modelName", "SelectionSmokeModel"},
                       {"bodyCount", 4},
                       {"viewportWidth", 1024},
                       {"viewportHeight", 768},
                       {"screenX", 90},
                       {"screenY", 30},
                       {"source", "test"}});

    if(!response.success) {
        return 1;
    }

    const auto scene_graph = response.payload.value("sceneGraph", nlohmann::json::object());
    if(scene_graph.value("nodeCount", 0) != 4) {
        return 2;
    }

    const auto render_frame = response.payload.value("renderFrame", nlohmann::json::object());
    if(render_frame.value("drawItemCount", 0) != 4) {
        return 3;
    }

    const auto selection_result =
        response.payload.value("selectionResult", nlohmann::json::object());
    if(selection_result.value("hitCount", 0) != 1) {
        return 4;
    }

    return 0;
}
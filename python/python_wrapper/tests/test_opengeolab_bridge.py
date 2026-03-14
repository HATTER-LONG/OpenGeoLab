import json

import opengeolab


def main() -> int:
    response_text = opengeolab.call(
        "selection",
        json.dumps(
            {
                "operation": "pickPlaceholderEntity",
                "modelName": "PythonSmokeModel",
                "bodyCount": 3,
                "viewportWidth": 1024,
                "viewportHeight": 768,
                "screenX": 144,
                "screenY": 96,
                "source": "python-test",
            }
        ),
    )
    response = json.loads(response_text)

    if not response.get("success", False):
        raise RuntimeError(f"selection call failed: {response_text}")

    payload = response.get("payload", {})
    scene_graph = payload.get("sceneGraph", {})
    render_frame = payload.get("renderFrame", {})
    selection_result = payload.get("selectionResult", {})

    if scene_graph.get("nodeCount") != 3:
        raise AssertionError(f"unexpected nodeCount: {scene_graph}")

    if render_frame.get("drawItemCount") != 3:
        raise AssertionError(f"unexpected drawItemCount: {render_frame}")

    if selection_result.get("hitCount") != 1:
        raise AssertionError(f"unexpected hitCount: {selection_result}")

    script = opengeolab.OpenGeoLabPythonBridge().suggest_placeholder_geometry_script(
        "PythonSmokeModel", 2
    )
    if "opengeolab.OpenGeoLabPythonBridge" not in script:
        raise AssertionError("expected exported script to reference OpenGeoLabPythonBridge")

    print(payload.get("summary", ""))
    return 0


if __name__ == "__main__":
    exit_code = main()
    if exit_code:
        raise SystemExit(exit_code)
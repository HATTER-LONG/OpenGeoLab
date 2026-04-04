<div align="center">

<img src="docs/images/banner.png" alt="OpenGeoLab" width="480" />

# OpenGeoLab

**AI-Powered Open-Source 3D Geometry Modeling Platform**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://isocpp.org/)
[![Qt 6.9](https://img.shields.io/badge/Qt-6.9-41CD52?logo=qt)](https://www.qt.io/)
[![OpenCASCADE](https://img.shields.io/badge/OCCT-CAD%20Kernel-orange)](https://dev.opencascade.org/)

[中文](README.md) · [Build Guide](#-quick-start) · [Architecture](#-architecture) · [Plugins](#-plugin-system)

</div>

---

## ✨ Highlights

<table>
<tr>
<td width="50%">

### 🤖 AI-Native Interaction

Built-in AI chat plugin drives geometry modeling, mesh generation,
and scene management through natural language. The unified JSON Action
protocol enables AI to directly invoke C++ module capabilities.

</td>
<td width="50%">

### 🧩 Modular Action Architecture

All features register as `module.action` pairs with a unified JSON
request/response protocol. Every Action self-describes its parameters
and return values — a natural fit for LLM Function Calling.

</td>
</tr>
<tr>
<td>

### 🐍 Python Plugin System

Embedded Python runtime with pybind11 bindings gives plugins full
access to all C++ modules. PySide6 UI extensions, hot-reloadable
plugins, and rapid prototyping out of the box.

</td>
<td>

### 🔬 Industrial CAD Kernel

Precise geometric modeling with OpenCASCADE + Gmsh finite element
mesh generation. BREP/STEP import, boolean operations, and
parametric modeling.

</td>
</tr>
<tr>
<td>

### 🎨 Real-Time OpenGL Rendering

Pure OpenGL 3.3 Core multi-pass pipeline: opaque, wireframe,
highlight, selection, and labels. DPI-aware, thick line rendering,
and MSDF text labels.

</td>
<td>

### 🎯 Unified Entity Selection

EntityRef abstraction uniformly addresses geometry topology
(faces/edges/vertices), mesh elements, and scene nodes. Supports
click pick, box select, type filtering, and hover highlight.

</td>
</tr>
</table>

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                   Qt6 / QML Application Layer                 │
│   ┌──────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐ │
│   │ Viewport │  │ Scene Tree│  │ Properties│  │  AI Chat  │ │
│   └────┬─────┘  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘ │
├────────┼───────────────┼──────────────┼──────────────┼───────┤
│        └───────────────┼──────────────┘              │       │
│                   RequestService          CopilotWorker      │
│                        │                       │             │
│              ┌─────────▼──────────┐    ┌───────▼───────┐     │
│              │  Command Dispatcher│◄───│ Python Runtime │     │
│              │  (module.action)   │    │  (pybind11)   │     │
│              └──┬──┬──┬──┬──┬────┘    └───────┬───────┘     │
├─────────────────┼──┼──┼──┼──┼─────────────────┼─────────────┤
│  ┌──────┐ ┌─────┤  │  │  │  ├─────┐    ┌─────▼─────┐       │
│  │Render│ │Scene│  │  │  │  │ I/O │    │  Plugins  │       │
│  │      │ │     │  │  │  │  │     │    │(Python UI)│       │
│  │Multi-│ │Graph│  │  │  │  │BREP │    │ AI Chat   │       │
│  │ Pass │ │Label│  │  │  │  │STEP │    │ Selection │       │
│  │OpenGL│ │Pick │  │  │  │  │     │    │ Demo UI   │       │
│  └──────┘ └─────┘  │  │  │  └─────┘    └───────────┘       │
│              ┌──────┘  │  └──────┐                           │
│              │Geometry  │  Mesh  │                           │
│              │         Core      │                           │
│              │(OCCT)  (Entity)  (Gmsh)                      │
│              └─────────┴─────────┘                           │
└──────────────────────────────────────────────────────────────┘
```

---

## 🤖 AI Integration

One of OpenGeoLab's core design philosophies is **AI-First**: every module
operation is exposed through a unified JSON Action protocol, enabling AI to
control the platform just like a human user.

```
User ──natural language──▶ AI Chat Plugin ──Function Call──▶ Command Dispatcher
                                                                  │
                 ┌────────────────────────────────────────────────┘
                 ▼              ▼              ▼               ▼
           geometry.*     mesh.*        scene.*           io.*
           Create shapes  Generate mesh  Manage scene      Import/Export
```

**Example AI-callable actions:**

| Action | Description |
|--------|-------------|
| `geometry.create_box` | Create a box with specified dimensions and position |
| `geometry.import_step` | Import an industrial STEP model |
| `mesh.generate_mesh` | Generate a finite element mesh for a shape |
| `scene.select` | Select entities by type and ID |
| `scene.add_label` | Add text labels to entities |
| `io.read_brep` | Read a BREP boundary representation file |

The AI plugin is built on the [Copilot SDK](https://github.com/nicepkg/copilot-sdk),
featuring streaming reasoning display, tool call visualization, multi-model
switching, and interactive user confirmation.

---

## 🧩 Modules

| Module | Responsibility | Key Capabilities |
|--------|---------------|------------------|
| **core** | Foundation | IAction interface, EntityRef/EntityTag addressing, module registry, logging |
| **geometry** | Geometric modeling | Primitives (Box/Cylinder/Sphere/Torus), BREP/STEP import, booleans, tessellation |
| **mesh** | Mesh generation | Gmsh FEM meshing, mesh storage & queries, render data building |
| **scene** | Scene management | Scene graph, labels, camera control, selection/hover, visibility, pick modes |
| **render** | OpenGL rendering | Multi-pass pipeline, thick lines, selection coloring, MSDF labels, font atlas |
| **command** | Command dispatch | Module registration & discovery, Action routing, data event notification |
| **io** | File I/O | BREP reader (extensible to more formats) |
| **python** | Python bridge | Embedded interpreter, pybind11 bindings, plugin runtime |

---

## 🔌 Plugin System

Python plugins have direct access to all C++ module capabilities:

```python
# Minimal plugin example
def describe_plugin():
    return {
        "name": "my_analysis_plugin",
        "description": "Custom geometry analysis tool",
        "author": "Your Name"
    }

def launch_ui():
    """Call C++ modules from Python"""
    import opengeolab_pywrapper as ogl

    # List all registered modules
    modules = ogl.list_modules()  # ['geometry', 'mesh', 'scene', 'io']

    # Execute an Action
    result = ogl.process('{"module":"geometry","action":"create_box",'
                         '"params":{"dx":10,"dy":20,"dz":5}}')
    return {"ok": True}
```

**Built-in plugins:**

| Plugin | Description |
|--------|-------------|
| `ai_chat_plugin` | AI assistant — natural language control for the entire platform |
| `demo_ui_plugin` | PySide6 UI integration example |
| `selection_demo_plugin` | Selection state interaction demo |
| `progress_demo_plugin` | Progress callback mechanism demo |

---

## 🛠️ Quick Start

### Prerequisites

- **Compiler**: C++20 support (MSVC 2022 / GCC 12+ / Clang 15+)
- **CMake** ≥ 3.25
- **Qt** 6.9 (Widgets, Quick, OpenGL, Concurrent)
- **Python** 3.11+ (for embedded runtime and plugins)
- **Ninja** (recommended build generator)

### Build

```bash
# Clone the repository
git clone https://github.com/HATTER-LONG/OpenGeoLab.git
cd OpenGeoLab

# Configure (first build auto-fetches CPM dependencies)
cmake -S . -B build -G Ninja

# Build
cmake --build build --config RelWithDebInfo --parallel

# Run tests
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `OPENGEOLAB_BUILD_TESTS` | `ON` | Build unit tests |
| `OPENGEOLAB_FETCH_DEPENDENCIES` | `ON` | Auto-fetch missing dependencies via CPM |
| `OPENGEOLAB_BUILD_SHARED_LIBS` | `ON` | Build as shared libraries |
| `OPENGEOLAB_ENABLE_PYSIDE6` | `ON`* | Set up Python venv with PySide6 |

\* Off by default in Debug configuration.

---

## 📦 Tech Stack

| Category | Technology |
|----------|-----------|
| Language | C++20 |
| Build | CMake 3.25+ / Ninja / CPM |
| UI Framework | Qt 6.9 (QML + Widgets) |
| CAD Kernel | OpenCASCADE Technology (OCCT) |
| Mesh Generation | Gmsh |
| Graphics API | OpenGL 3.3 Core (GLAD 2.0.8) |
| Math Library | GLM 1.0.3 |
| Python Bridge | pybind11 3.0.2 + PySide6 |
| JSON | nlohmann/json 3.12.0 |
| Logging | spdlog 1.17.0 + fmt 12.0.0 |
| Testing | doctest 2.5.0 + pytest |
| Image Loading | stb (header-only) |

---

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).

---

<div align="center">

**OpenGeoLab** — Teaching AI to understand the 3D world

</div>

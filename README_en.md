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

| Dependency | Version | Notes |
|-----------|---------|-------|
| **C++ Compiler** | C++20 | MSVC 2022 / GCC 12+ / Clang 15+ |
| **CMake** | ≥ 3.25 | Build system |
| **Ninja** | Latest | Recommended build generator |
| **Qt6** | ≥ 6.9 | GUI framework (see components below) |
| **OpenCASCADE** | ≥ 7.7 | CAD geometry kernel |
| **Gmsh** | ≥ 4.15 | Finite element mesh generator |
| **Python** | ≥ 3.11 | Embedded runtime & plugin system |

> **Auto-fetched dependencies (managed by CPM, no manual install needed):**
> fmt · spdlog · nlohmann/json · GLM · pybind11 · doctest · stb · GLAD · Kangaroo

### Installing External Dependencies

<details>
<summary><b>Windows (MSVC)</b></summary>

```powershell
# 1. Qt6 — Install via Qt Online Installer, select MSVC 2022 64-bit components:
#    Core, Gui, Widgets, Qml, Quick, QuickControls2, Concurrent, OpenGL,
#    Core5Compat, Svg, QuickLayouts, LinguistTools

# 2. OpenCASCADE — Build from source or download pre-built packages
#    https://dev.opencascade.org/release
#    Build with /MD dynamic CRT; note the cmake config directory path

# 3. Gmsh — Download SDK or build from source
#    https://gmsh.info/#Download
#    Note the share/gmsh directory path after installation

# 4. Python — Install from python.org, ensure "Add to PATH" is checked
```

</details>

<details>
<summary><b>Linux (Ubuntu/Debian)</b></summary>

```bash
# 1. Base tools
sudo apt install cmake ninja-build g++-12 python3 python3-dev

# 2. Qt6
sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev \
                 qt6-quick3d-dev qt6-svg-dev libqt6opengl6-dev \
                 qml6-module-qtquick-controls qml6-module-qtquick-layouts

# 3. OpenCASCADE — Recommended to build from source
#    https://dev.opencascade.org/release

# 4. Gmsh — Recommended to build from source or use SDK
#    https://gmsh.info/#Download
```

</details>

### Configure & Build

```bash
# Clone the repository
git clone https://github.com/HATTER-LONG/OpenGeoLab.git
cd OpenGeoLab

# Configure (specify external dependency paths; CPM auto-fetches the rest)
cmake -S . -B build -G Ninja \
  -DOpenCASCADE_DIR=<OCCT_install_path>/cmake \
  -Dgmsh_DIR=<Gmsh_install_path>/share/gmsh

# Build
cmake --build build --config RelWithDebInfo --parallel

# Run tests
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

> **Tip:** Create a `CMakeUserPresets.json` to persist local path configuration
> and avoid passing `-D` flags every time. See `CMakePresets.json` for the format.

### Required Qt6 Components

```
Core  Gui  Widgets  Concurrent  OpenGL
Qml  Quick  QuickControls2  QuickLayouts
Core5Compat  Svg  LinguistTools
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `OPENGEOLAB_BUILD_TESTS` | `ON` | Build unit tests (doctest) |
| `OPENGEOLAB_FETCH_DEPENDENCIES` | `ON` | Auto-fetch missing dependencies via CPM |
| `OPENGEOLAB_BUILD_SHARED_LIBS` | `ON` | Build as shared libraries |
| `OPENGEOLAB_ENABLE_PYSIDE6` | `ON`* | Set up Python venv with PySide6 |
| `OPENGEOLAB_WIN32_APP` | `OFF` | Windows GUI executable (no console) |

\* Off by default in Debug configuration.

---

## 📦 Tech Stack

<table>
<tr><th>Category</th><th>Technology</th><th>Install</th></tr>
<tr><td>Language</td><td>C++20</td><td>—</td></tr>
<tr><td>Build</td><td>CMake 3.25+ / Ninja / CPM</td><td>—</td></tr>
<tr><td>UI Framework</td><td>Qt 6.9 (QML + Widgets)</td><td>🔧 Pre-install</td></tr>
<tr><td>CAD Kernel</td><td>OpenCASCADE Technology (OCCT)</td><td>🔧 Pre-install</td></tr>
<tr><td>Mesh Generation</td><td>Gmsh ≥ 4.15</td><td>🔧 Pre-install</td></tr>
<tr><td>Graphics API</td><td>OpenGL 3.3 Core (GLAD 2.0.8)</td><td>📦 CPM</td></tr>
<tr><td>Math Library</td><td>GLM 1.0.3</td><td>📦 CPM</td></tr>
<tr><td>Python Bridge</td><td>pybind11 3.0.2 + PySide6</td><td>📦 CPM + venv</td></tr>
<tr><td>JSON</td><td>nlohmann/json 3.12.0</td><td>📦 CPM</td></tr>
<tr><td>Logging</td><td>spdlog 1.17.0 + fmt 12.0.0</td><td>📦 CPM</td></tr>
<tr><td>Testing</td><td>doctest 2.5.0 + pytest</td><td>📦 CPM + venv</td></tr>
<tr><td>Image Loading</td><td>stb (header-only)</td><td>📦 CPM</td></tr>
</table>

---

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).

---

<div align="center">

**OpenGeoLab** — Teaching AI to understand the 3D world

</div>

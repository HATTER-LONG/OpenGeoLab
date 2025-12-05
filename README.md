# OpenGeoLab

<p align="center">
  <strong>基于 Qt6/QML 和 OpenCASCADE 的现代 3D CAD 几何可视化应用</strong>
</p>

<p align="center">
  <a href="#功能特性">功能特性</a> •
  <a href="#快速开始">快速开始</a> •
  <a href="#项目结构">项目结构</a> •
  <a href="#构建说明">构建说明</a> •
  <a href="#使用示例">使用示例</a> •
  <a href="#许可证">许可证</a>
</p>

---

## 功能特性

- 🎨 **现代 UI 框架** - 基于 Qt6/QML 构建的响应式用户界面
- 🔧 **CAD 文件支持** - 导入 STEP (.step, .stp) 和 BREP (.brep, .brp) 格式
- 🖼️ **OpenGL 渲染** - 自定义 OpenGL 渲染器，支持光照和着色
- 🖱️ **交互式操作** - 鼠标拖拽旋转、滚轮缩放、Shift+拖拽平移
- 📊 **HDF5 数据支持** - 通过 HighFive 库支持 HDF5 数据格式
- 🧩 **组件化架构** - 基于依赖注入的可扩展文件读取器系统
- ✅ **单元测试** - 使用 doctest 框架的完整测试覆盖

## 依赖项

### 必需依赖

| 依赖项 | 最低版本 | 说明 |
|--------|----------|------|
| [Qt](https://www.qt.io/) | 6.8+ | GUI 框架 (Core, Gui, Qml, Quick) |
| [OpenCASCADE](https://dev.opencascade.org/) | 7.6+ | 3D CAD 建模内核 |
| [CMake](https://cmake.org/) | 3.14+ | 构建系统 |
| [HDF5](https://www.hdfgroup.org/solutions/hdf5/) | 1.10+ | 高性能数据格式 |

### 自动管理依赖 (通过 CPM)

| 依赖项 | 版本 | 说明 |
|--------|------|------|
| [cxxopts](https://github.com/jarro2783/cxxopts) | 3.0.0 | 命令行参数解析 |
| [Kangaroo](https://github.com/HATTER-LONG/Kangaroo) | 2.2.1 | 基础设施工具库 |
| [HighFive](https://github.com/highfive-devs/highfive) | 3.2.0 | 现代 C++ HDF5 接口 |
| [doctest](https://github.com/doctest/doctest) | 2.4.12 | 单元测试框架 |
| [spdlog](https://github.com/gabime/spdlog) | - | 日志库 (通过 Kangaroo) |
| [fmt](https://github.com/fmtlib/fmt) | - | 格式化库 (通过 Kangaroo) |

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/yourusername/OpenGeoLab.git
cd OpenGeoLab
```

### 2. 配置项目

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DQT_QML_GENERATE_QMLLS_INI=ON
```

> 💡 **提示**: 设置 `CPM_SOURCE_CACHE` 环境变量可以缓存下载的依赖项:
> ```bash
> export CPM_SOURCE_CACHE=$HOME/.cache/CPM
> ```

### 3. 构建

```bash
cmake --build build --config Debug
```

### 4. 运行

```bash
./build/bin/OpenGeoLabApp
```

## 项目结构

```
OpenGeoLab/
├── cmake/                  # CMake 工具模块
│   ├── CPM.cmake          # CPM 包管理器
│   └── tools.cmake        # 工具配置 (sanitizers, ccache 等)
├── include/               # 公共头文件
│   ├── core/             # 核心功能 (日志等)
│   ├── geometry/         # 几何数据结构
│   ├── io/               # 文件 I/O 接口
│   ├── render/           # 渲染器接口
│   └── ui/               # UI 组件接口
├── src/                   # 源代码
│   ├── app/              # 应用程序入口
│   ├── core/             # 核心功能实现
│   ├── io/               # 文件读取器实现
│   ├── render/           # OpenGL 渲染器实现
│   └── ui/               # QML 组件实现
├── resources/             # 资源文件
│   └── qml/              # QML 界面文件
├── test/                  # 单元测试
│   └── source/           # 测试源文件
├── CMakeLists.txt        # 主 CMake 配置
└── README.md             # 本文件
```

## 构建说明

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENABLE_CONSOLE` | ON | Windows 下启用控制台窗口 |
| `ENABLE_TEST_COVERAGE` | OFF | 启用测试覆盖率统计 |
| `USE_SANITIZER` | - | 启用 Sanitizer (Address, Memory, Thread 等) |
| `USE_STATIC_ANALYZER` | - | 启用静态分析 (clang-tidy, cppcheck 等) |
| `USE_CCACHE` | - | 启用 ccache 加速编译 |

### 构建示例

```bash
# Release 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 启用 Address Sanitizer
cmake -B build -DUSE_SANITIZER=Address
cmake --build build

# 启用 clang-tidy
cmake -B build -DUSE_STATIC_ANALYZER="clang-tidy"
cmake --build build
```

### 运行测试

```bash
# 构建并运行所有测试
cmake --build build --target OpenGeoLabTests
ctest --test-dir build --output-on-failure

# 或直接运行测试可执行文件
./build/bin/OpenGeoLabTests
```

## 使用示例

### 命令行参数

```bash
# 显示帮助
./OpenGeoLabApp --help

# 指定名称和语言
./OpenGeoLabApp --name="User" --lang=zh
```

### 交互操作

| 操作 | 功能 |
|------|------|
| 左键拖拽 | 旋转模型 |
| Shift + 左键拖拽 | 平移视图 |
| 滚轮 | 缩放视图 |

### 支持的文件格式

- **STEP** (.step, .stp) - ISO 10303 标准交换格式
- **BREP** (.brep, .brp) - OpenCASCADE 原生边界表示格式

## 架构设计

### 文件读取器系统

项目使用组件工厂模式实现可扩展的文件读取器系统：

```
IModelReader (接口)
    ├── BrepReader (.brep, .brp)
    └── StepReader (.step, .stp)

IModelReaderRegistry (注册表)
    └── 管理所有读取器的注册和查找
```

### 渲染管线

```
QML (Geometry3D)
    └── OpenGLRenderer
        ├── Vertex Shader (MVP 变换 + 光照)
        └── Fragment Shader (漫反射 + 环境光)
```

## 开发指南

### 代码风格

项目使用以下工具保证代码质量：

- **clang-format** - C++ 代码格式化
- **cmake-format** - CMake 文件格式化
- **clang-tidy** - 静态代码分析

### 添加新的文件格式支持

1. 在 `src/io/` 创建新的读取器类，实现 `IModelReader` 接口
2. 在 `model_reader_registry.cpp` 的 `registerBuiltinModelReaders()` 中注册
3. 更新 `Main.qml` 中的文件过滤器

## 许可证

本项目基于 [Unlicense](LICENSE) 发布 - 公共领域，可自由使用。

## 致谢

- [Qt Project](https://www.qt.io/) - 优秀的跨平台 GUI 框架
- [OpenCASCADE](https://dev.opencascade.org/) - 强大的 3D CAD 内核
- [HighFive](https://github.com/highfive-devs/highfive) - 现代化的 HDF5 C++ 接口
- [Kangaroo](https://github.com/HATTER-LONG/Kangaroo) - 实用的基础设施工具库
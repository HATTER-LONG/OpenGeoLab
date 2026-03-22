---
description: 'OpenGeoLab 现代 CMake 编写规范'
applyTo: '**/{CMakeLists.txt,*.cmake}'
---

# OpenGeoLab Modern CMake Instructions

## 1. 格式化优先级

- 以仓库根目录的 [.cmake-format](../../.cmake-format) 为唯一格式化基准。
- 保持 2 空格缩进、100 列限制和悬挂括号设置一致。
- 不要手工破坏 cmake-format 对参数换行和括号布局的输出。
- 自定义命令必须保持在 .cmake-format 中已声明的解析规则范围内。

## 2. 编写原则

### 2.1 现代 CMake 基线

- 以 target 为中心组织构建逻辑，避免目录级全局变量和隐式状态。
- 优先使用 `target_compile_features`、`target_sources`、`target_compile_options`、`target_include_directories`、`target_link_libraries`、`target_link_options`。
- 对编译选项、包含目录、链接库和编译定义使用 `PUBLIC`、`PRIVATE`、`INTERFACE` 明确传播范围。
- 依赖项优先使用 `find_package`、导入目标或仓库已有的 CPM 封装；避免全局 `include_directories`、`link_libraries`、`add_definitions` 这类旧式写法。
- 只在确有必要时使用 generator expressions，并保持表达式简洁、可读、可维护。
- 新增模块时优先通过目标属性传播依赖，而不是依赖隐式目录状态。

## 3. 结构与可维护性

- 每个 CMakeLists.txt 只承担一个清晰职责，避免把所有逻辑堆在顶层。
- 将平台差异、可选特性和第三方依赖隔离成独立选项或模块。
- 大段重复逻辑应抽取成函数或宏，但优先函数；只有确实需要作用域共享时才考虑宏。
- CMake 变量命名要清晰，避免和目标名、缓存项、环境变量混淆。
- 保持构建逻辑可读，避免过度魔法和难以推导的生成式表达。

## 4. 目标与依赖

- 新增库或可执行文件时，优先显式定义 target 名称、源文件和链接关系。
- 头文件路径和安装路径要保持一致，避免构建树和安装树行为不一致。
- 依赖顺序应清晰：先声明目标，再设置属性，再链接依赖，再配置安装与导出。
- 使用 generator expressions 时，只在确有必要时使用，并保持表达式简洁。
- 对可选依赖、测试、示例、工具程序使用开关控制，避免默认拉高构建成本。

## 5. 工程最佳实践

- 顶层应包含最低版本要求、项目名、语言和基本策略设置。
- 统一开启必要的警告、标准和导出规则，保持跨平台可预测性。
- 对外暴露的库应明确区分 public/private include、编译定义和链接库。
- 安装、导出、打包逻辑应与构建逻辑分离，避免互相耦合。
- 对于第三方依赖，优先保持封装，避免污染全局命名空间或编译设置。
- 优先使用 Ninja 作为构建生成器（`cmake -G Ninja`），提升增量编译速度；在 Windows 上确保 MSVC 环境已激活。

## 6. 生成代码时的默认行为

- 新增 CMake 逻辑时，优先补充注释说明目的，而不是逐行解释语法。
- 如果现有实现能用 target 级别写法替代目录级写法，应优先重构为 target 级。
- 如果某段写法会让 cmake-format 产生不稳定输出，应调整结构而不是手工对齐。
- 需要使用仓库现有自定义命令时，保持命令参数顺序和格式与现有配置兼容。

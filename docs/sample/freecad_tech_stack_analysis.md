# FreeCAD 技术栈结构与边界分析

## 1. 文档目标

本文基于 `C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\FreeCAD` 当前源码，对 FreeCAD 的技术栈结构做一次面向架构与实现边界的拆解，重点回答以下问题：

- FreeCAD 的总体技术栈和分层是什么样的。
- `Qt/C++`、`PySide6/PySide2`、FreeCAD 自身 Python wrapper、核心功能模块之间的边界在哪里。
- 3D / OpenGL 渲染区域的技术栈如何搭建。
- 选择 / 拾取（selection / picking）功能的技术路径如何流转。

本文不做 UI 使用教程，而是聚焦“代码如何组织、边界怎么划、事件和数据如何流动”。

---

## 2. 先给结论：FreeCAD 不是“PySide 驱动的 Qt 应用”，而是“C++ 核心 + C++ GUI + Python 扩展层”的混合架构

FreeCAD 的主体不是用 PySide 写出来的 Qt 应用，而是：

1. **C++ 核心层**
   - 目录核心：`src\Base`、`src\App`
   - 职责：基础设施、文档模型、属性系统、几何数据、事务、Python 解释器管理、核心对象的 Python 暴露。

2. **C++ GUI 层**
   - 目录核心：`src\Gui`
   - 职责：主窗口、命令系统、工作台管理、3D 视图、选择系统、Qt 控件、Coin3D/Quarter 集成。

3. **Python 模块 / 工作台层**
   - 目录核心：`src\Mod\*`
   - 职责：业务功能扩展、工作台定义、命令注册、工具栏/菜单组织、脚本化工作流。

4. **Qt for Python / PySide 桥接层**
   - 目录核心：`src\Gui\PythonWrapper.cpp`、`src\Gui\UiLoader.cpp`
   - 职责：把 Qt 的 `QObject/QWidget/QAction/...` 在 Python 和 C++ 之间互转，让 Python 层能够操作 Qt 对象和 `.ui` 文件。
   - **它是桥接层，不是 FreeCAD 核心 API 的主体。**

换句话说：

- **核心能力**主要由 C++ 提供；
- **GUI 骨架**主要由 C++ + Qt 提供；
- **Python** 主要用于模块装配、工作台扩展、业务脚本和自动化；
- **PySide** 主要用于 Qt 对象桥接，不负责 FreeCAD 文档/几何核心对象的主绑定模型。

---

## 3. 源码目录分层图

从 `src\CMakeLists.txt` 可以直接看到大层次（`src\CMakeLists.txt:3-17`）：

```text
src
├─ Base        基础设施 / Python 解释器 / 基础几何封装
├─ App         核心应用层 / 文档模型 / 属性系统 / Feature
├─ Main        主程序入口（GUI / CMD / Python 模块入口）
├─ Mod         各类业务模块与工作台
├─ Ext         扩展与打包辅助
├─ Gui         Qt GUI、3D Viewer、Selection、Workbench
└─ 3rdParty    第三方库（含 PyCXX 等）
```

关键点：

- `BUILD_GUI` 为真时才进入 `src\Gui`（`src\CMakeLists.txt:12-17`），说明 **GUI 不是核心层的前提**。
- `src\Main\MainCmd.cpp` 走命令行 / 无 GUI 启动；
- `src\Main\MainGui.cpp` 走 GUI 启动；
- `src\Main\MainPy.cpp` 暴露 `FreeCAD` Python 模块；
- `src\Main\FreeCADGuiPy.cpp` 暴露 `FreeCADGui` Python 模块与 GUI 启动辅助函数。

这意味着 FreeCAD 从设计上就是 **App/Core 可独立于 Gui 存在** 的。

---

## 4. 启动与装配流程：谁先启动，谁负责扫描模块

### 4.1 GUI 启动链路

GUI 主入口在 `src\Main\MainGui.cpp`：

- `App::Application::init(...)` 初始化核心应用（`src\Main\MainGui.cpp:219-224`）
- `Gui::Application::initApplication()` 初始化 GUI 体系（`src\Main\MainGui.cpp:248`）
- 随后 `Gui::Application::runApplication()` 进入 GUI 运行阶段（`src\Main\MainGui.cpp:333-339`）

在 `Gui::Application::initApplication()` 里：

- 注册 GUI 相关类型与视图类型（`src\Gui\Application.cpp:2275-2290`, `2298-2381`）
- 创建内置脚本生产者 `FreeCADGuiInit`（`src\Gui\Application.cpp:2284-2286`）
- 初始化 Open Inventor / Quarter / SoFCDB（`src\Gui\Application.cpp:2375-2381`）

其中：

- `SoDB::init()` 初始化 Coin3D/Open Inventor 数据库
- `SIM::Coin3D::Quarter::Quarter::init()` 初始化 Quarter
- `SoFCDB::init()` 初始化 FreeCAD 自己的 Inventor 扩展节点体系

### 4.2 App 初始化脚本：扫描 Mod 和 Python 路径

FreeCAD 的模块扫描，当前版本大量放在 Python 初始化脚本里，而不是全写在 C++。

`src\App\FreeCADInit.py` 明确写了这是 “always 运行”的初始化脚本（`src\App\FreeCADInit.py:27-38`）。

其中关键流程在：

- `InitSequence.scan()`（`src\App\FreeCADInit.py:1344-1434`）
- `InitSequence.load_mods()`（`src\App\FreeCADInit.py:1436-1463`）

这里做了几件重要事情：

1. 建立标准路径与用户路径：
   - `std_home / "Mod"`
   - `user_home / "Mod"`
   - 用户宏目录下的 `Mod`
   - 附加模块路径 `AdditionalModulePaths`

2. 扫描目录型模块（Dir Mods）：
   - `mods.scan_and_override(std_mod)` 等（`src\App\FreeCADInit.py:1393-1399`）

3. 将模块目录缓存给 GUI 初始化脚本：
   - `App.__ModDirs__ = [str(d) for d in mods.dirs()]`（`src\App\FreeCADInit.py:1401-1402`）

4. 建立 `App.__path__` 和 `sys.path`，让模块既能走 FreeCAD 风格装配，也能走 Python import（`src\App\FreeCADInit.py:1404-1429`）

5. 先加载目录型模块，再加载 Python import 路径里的扩展模块（`src\App\FreeCADInit.py:1446-1460`）

6. 将加载结果缓存到 `App.__ModCache__`，供 GUI 初始化继续使用（`src\App\FreeCADInit.py:1462-1463`）

### 4.3 GUI 初始化脚本：扫描 `InitGui.py` 并注册工作台

GUI 侧的模块装配则在 `src\Gui\FreeCADGuiInit.py`。

`InitApplications()` 的逻辑非常关键（`src\Gui\FreeCADGuiInit.py:386-420`）：

- 遍历 `App.__ModCache__`
- 对目录型模块，执行模块目录下的 `InitGui.py`（`DirModGui.run_init_gui()`，`src\Gui\FreeCADGuiInit.py:290-311`）
- 对扩展型模块，尝试 `import <module>.init_gui`（`src\Gui\FreeCADGuiInit.py:361-383`）

最后：

- `Gui.addWorkbench(NoneWorkbench())`（`src\Gui\FreeCADGuiInit.py:446`）
- `InitApplications()`（`src\Gui\FreeCADGuiInit.py:448-449`）
- `Gui.activateWorkbench("NoneWorkbench")`（`src\Gui\FreeCADGuiInit.py:451-452`）

这说明 **工作台的“发现与挂接”已经不是纯 C++ 静态注册，而是 C++ 框架 + Python 初始化脚本共同完成**。

---

## 5. Qt/C++、PySide、Python wrapper、核心功能的边界

## 5.1 C++ 核心层边界：`Base` + `App`

### `src\Base`

负责：

- Python 解释器包装：`src\Base\Interpreter.h/.cpp`
- 自定义 Python 对象基础设施：`src\Base\PyObjectBase.h`
- PyCXX 风格的几何对象包装：`src\Base\GeometryPyCXX.h/.cpp`
- 基础数学/向量/矩阵/旋转/单位/日志/参数等

例如：

- `InterpreterSingleton::addModule()`、`addType()` 把扩展模块和类型注册进 Python（`src\Base\Interpreter.cpp:505-533`）
- `InterpreterSingleton::createSWIGPointerObj()` 用于把某些对象包装为 SWIG 指针对象（`src\Base\Interpreter.cpp:941-959`）
- `PyObjectBase` 是大量 FreeCAD C++ 对象导出到 Python 的根基类（`src\Base\PyObjectBase.h:142-163`）

### `src\App`

负责：

- 文档模型：`Document`, `DocumentObject`, `FeaturePython`, `Property*`
- 应用级对象管理和配置
- 核心对象到 Python 的绑定导出

典型文件：

- `src\App\Application.cpp`
- `src\App\Document.cpp`
- `src\App\DocumentObject.cpp`
- `src\App\FeaturePython.cpp`
- 一大批 `*PyImp.cpp`

边界上可以理解为：

- `Base` 解决“解释器 + Python 绑定基础设施 + 基础数据结构”
- `App` 解决“FreeCAD 自己的业务核心对象”

## 5.2 Qt/C++ GUI 层边界：`src\Gui`

`src\Gui` 负责所有“真正的 GUI 骨架”：

- 主窗口、Dock、命令、菜单、工具栏
- 工作台管理
- 3D 视图
- 选择系统
- Python console / macro editor
- Qt widget 工厂、`.ui` 装载

重要的是：

- **主 GUI 不是 Python 拼出来的，而是 C++ Qt 架构先搭好**
- Python 更多是在已有 GUI 骨架上“追加工作台、菜单、命令和控件逻辑”

例如 `Gui::Application` 在构造时就创建：

- `FreeCADGui` Python 模块
- `Selection` Python 子模块
- 各类 `View3DInventorPy`, `MDIViewPy`, `MainWindowPy` 等 GUI 代理类型

见 `src\Gui\Application.cpp:524-676`。

## 5.3 Python 工作台边界：`src\Mod\*`

`src\Mod` 下是业务模块与工作台的主要承载区。

这些模块一般有两套面：

1. `App` 面：核心业务对象、算法、文档对象
2. `Gui` 面：工作台、视图提供者、命令、任务面板

典型模式是：

```text
src\Mod\<Module>\
├─ App
├─ Gui
└─ Init.py / InitGui.py 或对应 Python 模块
```

工作台本身的边界非常明确：

- C++ 侧提供 `Workbench / PythonWorkbench` 类型体系
- Python 侧用 `InitGui.py` 定义工作台类、菜单、工具栏、上下文菜单

`src\Gui\Workbench.cpp` 文档注释就直接说明：

- 启动时扫描所有模块目录并执行 `InitGui.py`
- 用户点击工作台条目后，对应模块才真正加载并激活（`src\Gui\Workbench.cpp:197-206`）

同时 Python 侧工作台可以通过：

- `appendMenu()`（`src\Gui\Workbench.cpp:1159-1195`）
- `appendToolbar()`（`src\Gui\Workbench.cpp:1240-1251`）
- `appendContextMenu()`（`src\Gui\Workbench.cpp:1206-1224`）

把 UI 元素挂入现有主窗口。

### 结论

**Python 工作台不是替代 Qt/C++ GUI，而是在 Qt/C++ GUI 壳上做业务层扩展。**

---

## 6. FreeCAD 自身 Python wrapper 与 PySide 的区别

这是最容易混淆的一块，必须分开看。

## 6.1 FreeCAD 自身 Python wrapper：主体绑定体系

FreeCAD 的核心对象暴露给 Python，主要依靠的是：

1. **Python C API + 自己的 `PyObjectBase` 体系**
2. **PyCXX**
3. **大量 `*PyImp.cpp` / `*Py.cpp`**
4. **`Interpreter::addType()` / `addModule()`**

典型证据：

- `src\Base\PyObjectBase.h`
- `src\Base\GeometryPyCXX.h`
- `src\Base\Interpreter.cpp:505-533`
- `src\Gui\Application.cpp:555-665`
- `src\App\Application.cpp` / `src\Gui\Application.cpp` 中大量 `...Py::init_type()`

例如：

- `UiLoaderPy::init_type()` 后用 `Base::Interpreter().addType(...)` 注册到 `FreeCADGui`（`src\Gui\Application.cpp:555-556`）
- `SelectionFilterPy` 注册到 `FreeCADGui.Selection`（`src\Gui\Application.cpp:571-586`）
- `View3DInventorPy`, `View3DInventorViewerPy`, `MDIViewPy`, `MainWindowPy` 等 GUI 类型注册到 `FreeCADGui`（`src\Gui\Application.cpp:661-665`）

这套体系才是 FreeCAD 核心 API 暴露给 Python 的主体。

## 6.2 PySide2 / PySide6：Qt 对象桥接层

FreeCAD **并不是不用 PySide**，当前源码明确支持：

- `PySide2 + shiboken2`
- `PySide6 + shiboken6`

证据：

- `cMake\FreeCAD_Helpers\SetupShibokenAndPyside.cmake:1-178`
- `cMake\FindPySide6.cmake:1-54`
- `src\Gui\CMakeLists.txt:299-326`
- `src\Gui\PythonWrapper.cpp:69-124`, `149-201`

构建层面：

- `FREECAD_USE_SHIBOKEN` 和 `FREECAD_USE_PYSIDE` 默认是 `ON`（`SetupShibokenAndPyside.cmake:4-5`）
- Qt5 时走 `PySide2`，Qt6 时走 `PySide6`（`SetupShibokenAndPyside.cmake:12-18`）
- `FreeCADGui` target 会链接 `PySide2::pyside2` 或 `PySide6::pyside6`，并注入 `HAVE_PYSIDE*` 宏（`src\Gui\CMakeLists.txt:299-326`）

运行时作用：

- `PythonWrapper::toQObject()` 把 Python 对象还原为 `QObject*`（`src\Gui\PythonWrapper.cpp:590-593`）
- `fromQObject()` / `fromQWidget()` 把 C++ Qt 对象包装成 Python 侧 Qt 对象（`src\Gui\PythonWrapper.cpp:798-853`）
- `loadCoreModule()` / `loadGuiModule()` / `loadWidgetsModule()` / `loadUiToolsModule()` 动态加载 PySide Qt 模块（`src\Gui\PythonWrapper.cpp:882-905`）

### 这意味着什么

PySide 在 FreeCAD 中的职责更像：

- “把 Qt 控件拿到 Python 层”
- “让 Python 能操作 QWidget / QAction / QUiLoader / QImage / QIcon”
- “支持 `.ui` 文件与 Python Qt 世界互通”

而不是：

- 用来定义 `Document` / `DocumentObject` / `Selection` / `ViewProvider` 这些 FreeCAD 主体对象。

## 6.3 UiLoader / PySideUic 的定位

`src\Gui\UiLoader.cpp` 非常能说明边界：

- `PySideUicModule` 被注册到 `FreeCADGui` 模块中（`src\Gui\Application.cpp:563-564`）
- 它补了 `loadUiType()` 和 `loadUi()`（`src\Gui\UiLoader.cpp:106-219`）
- `wrapFromWidgetFactory()` 通过 `PythonWrapper` 在 Python/C++ 间转换 `QWidget`（`src\Gui\UiLoader.cpp:58-102`）

这里本质上做的是：

- 给 Python 层提供 Qt Designer `.ui` 装载能力
- 给 Python 层提供 FreeCAD 自定义控件的创建能力

所以更准确的边界描述应该是：

> FreeCAD 的 Python wrapper 分成两类：  
> 一类是 FreeCAD 自身对象绑定（主体系）；  
> 一类是 PySide/shiboken Qt 桥接（辅助 GUI 集成层）。

---

## 7. 3D / OpenGL 渲染区域技术栈

## 7.1 不是“QOpenGLWidget 直接画场景”，而是 Qt + Quarter + Coin3D + FreeCAD 扩展节点

3D 视图最外层是 `View3DInventor`：

- 文件：`src\Gui\View3DInventor.cpp/.h`
- 类型：`Gui::MDIView` 的一个 3D 实现

在构造函数里它创建 `View3DInventorViewer`（`src\Gui\View3DInventor.cpp:95-145`）：

- 根据采样数决定是否传入 `QSurfaceFormat`
- 将 viewer 挂到 `QStackedWidget`
- 之后调用 `applySettings()`

而真正的 3D 栈主体在 `View3DInventorViewer`。

## 7.2 类继承链

`View3DInventorViewer` 的继承链是（`src\Gui\View3DInventorViewer.h:105-106`）：

```text
Gui::View3DInventorViewer
  └─ Quarter::SoQTQuarterAdaptor
       └─ Quarter::QuarterWidget
            └─ QGraphicsView
```

其中 `QuarterWidget` 不是简单的 QWidget 包装，而是：

- 类定义在 `src\Gui\Quarter\QuarterWidget.h`
- 继承 `QGraphicsView`（`QuarterWidget.h:62-64`）
- 内部使用 `QOpenGLWidget` 作为 viewport（构造细节见 `QuarterWidget.cpp:147-271`）

`QuarterWidget.cpp` 里的关键实现：

- 自定义 `CustomGLWidget : public QOpenGLWidget`（`src\Gui\Quarter\QuarterWidget.cpp:147-182`）
- `QuarterWidget::constructor()` 内部：
  - 创建 `QGraphicsScene`
  - `setViewport(new CustomGLWidget(...))`（`QuarterWidget.cpp:267-271`）

所以渲染区域的真实 Qt 宿主关系可以理解为：

```text
View3DInventor (MDIView)
  -> View3DInventorViewer
    -> SoQTQuarterAdaptor
      -> QuarterWidget(QGraphicsView)
        -> viewport = QOpenGLWidget
```

## 7.3 Coin3D / Open Inventor 是场景图核心

Quarter 的作用不是替代 Coin3D，而是做 **Qt 与 Coin3D 的适配层**。

`QuarterWidget` 暴露了：

- `SoRenderManager`
- `SoEventManager`
- `setSceneGraph(SoNode*)`
- `processSoEvent(...)`

见 `src\Gui\Quarter\QuarterWidget.h:162-177`。

`SoQTQuarterAdaptor` 则进一步补了 SoQt 兼容能力（`src\Gui\Quarter\SoQTQuarterAdaptor.h:49-160`）：

- camera 管理
- seek mode
- interaction callbacks
- `processSoEvent()` / `paintEvent()`

因此 3D 视图的核心渲染模型不是 Qt SceneGraph，而是 **Coin3D/Open Inventor 场景图**。

## 7.4 FreeCAD 在 Coin3D 场景图上的自定义层

`View3DInventorViewer` 初始化时会自己拼装一棵场景树（`src\Gui\View3DInventorViewer.cpp:540-698`）：

- `selectionRoot = new Gui::SoFCUnifiedSelection()`（543）
- `pcViewProviderRoot = selectionRoot`（547）
- 添加光照、环境节点（548-549）
- 添加透明对象修正用 hidden anchor（551-566）
- `setSceneGraph(pcViewProviderRoot)`（570-571）
- 增加 `SoEventCallback`（572-577）
- 创建 `dimensionRoot`、`editingRoot`、`objectGroup`（579-614）
- 替换默认 GL render action 为 `SoBoxSelectionRenderAction`（616-628）
- 设置透明渲染模式（637-640）
- 创建 `View3DInventorSelection`（596）

这说明 FreeCAD 的 3D 栈不是“通用 Coin3D viewer 直接拿来用”，而是在其上叠加了：

- FreeCAD 选择根节点
- FreeCAD 维度/编辑/物理对象分组
- FreeCAD 自定义 render action
- FreeCAD 的导航、NaviCube、颜色与高亮体系

## 7.5 OpenGL 处于什么位置

OpenGL 并不是被 Qt 直接高层封装到底，而是主要由 Coin3D 的 render action 驱动。

技术路径大致是：

```text
Qt QWidget / QOpenGLWidget
  -> QuarterWidget / SoQTQuarterAdaptor
    -> SoRenderManager
      -> SoGLRenderAction
        -> Coin3D/Open Inventor 节点遍历
          -> OpenGL 调用
```

FreeCAD 自己还会替换成 `SoBoxSelectionRenderAction`（`src\Gui\View3DInventorViewer.cpp:616-628`），说明它在 Coin3D 渲染动作层也做了扩展。

### 渲染栈总结

- **Qt**：窗口系统、输入系统、宿主控件、OpenGL context 容器
- **Quarter**：Qt 与 Coin3D 的适配器
- **Coin3D/Open Inventor**：真正的场景图与渲染/事件遍历核心
- **FreeCAD 自定义节点与动作**：选择、高亮、编辑根、尺寸、特殊渲染
- **OpenGL**：底层执行层

---

## 8. Select / 拾取功能的技术路径

这是整个 FreeCAD 3D 交互最关键的一层。

## 8.1 选择系统由两部分组成

可以把它理解成两套协作系统：

1. **场景图内选择层**
   - `SoFCUnifiedSelection`
   - `SoFCSelectionAction`
   - `SoFCPreselectionAction`
   - `View3DInventorSelection`

2. **全局选择状态层**
   - `SelectionSingleton`
   - `SelectionChanges`
   - `SelectionObserver`
   - `SelectionObject`

前者解决：

- 在 3D 场景里怎么 pick、怎么高亮、怎么显示选中态

后者解决：

- 选中了谁
- 当前 preselect 是谁
- 选择变化怎么广播给树、属性、3D 视图、命令系统

## 8.2 场景图入口：`SoFCUnifiedSelection`

`SoFCUnifiedSelection` 是 FreeCAD 新的统一选择根节点（`src\Gui\Selection\SoFCUnifiedSelection.h:51-58`）：

- 继承 `SoSeparator`
- 重写 `handleEvent()`（89）
- 重写 `GLRenderBelowPath()`（90）
- 内部有：
  - `selectionEnabled`
  - `preselectionMode`
  - `selectionMode`
  - `setPreselect(...)`
  - `setSelection(...)`
  - `getPickedList(...)`

它持有 `Gui::Document* pcDocument`（123），说明选择根节点知道自己属于哪个文档视图。

在 `View3DInventorViewer::setDocument()` 中也会把文档写进选择根（`src\Gui\View3DInventorViewer.cpp:799-804`）。

## 8.3 选择事件在 Viewer 中如何反映

`View3DInventorViewer` 自己是 `SelectionObserver`（`src\Gui\View3DInventorViewer.h:105-106`）。

当选择状态变化时：

- `View3DInventorViewer::onSelectionChanged()` 会收到 `SelectionChanges`
- 然后：
  - `inventorSelection->checkGroupOnTop(Reason)` 更新置顶高亮层（`src\Gui\View3DInventorViewer.cpp:845-867`）
  - 若是 preselect，则执行 `SoFCPreselectionAction`（874-879）
  - 否则执行 `SoFCSelectionAction`（880-882）

这意味着：

> 全局选择状态变更后，Viewer 不是自己手搓颜色，而是把变化重新投射回场景图动作（action）系统。

## 8.4 `View3DInventorSelection`：置顶显示层

`View3DInventorSelection` 用于管理“选中/预选对象置顶显示”的附加层（`src\Gui\View3DInventorSelection.cpp`）。

构造时它创建：

- `GroupOnTop`
- `GroupOnTopSel`
- `GroupOnTopPreSel`

见 `src\Gui\View3DInventorSelection.cpp:44-85`。

`checkGroupOnTop()` 根据 `SelectionChanges`：

- 对 preselect/selection 添加或移除 path annotation
- 根据 ViewProvider 的 `OnTopWhenSelected` 决定是否置顶
- 对 sub-element 路径做 detail/path 级处理

见 `src\Gui\View3DInventorSelection.cpp:95-307`。

因此它的职责不是“维护全局 selection state”，而是：

- **把全局选中态映射为一个额外的显示层**

## 8.5 全局状态入口：`SelectionSingleton`

全局选择状态由 `SelectionSingleton` 管理，核心头文件是 `src\Gui\Selection\Selection.h`。

`SelectionChanges` 定义了消息类型（`src\Gui\Selection\Selection.h:75-92`）：

- `AddSelection`
- `RmvSelection`
- `SetSelection`
- `ClrSelection`
- `SetPreselect`
- `RmvPreselect`
- `SetPreselectSignal`
- `PickedListChanged`
- `ShowSelection`
- `HideSelection`

### preselect 更新

`SelectionSingleton::setPreselect(...)`（`src\Gui\Selection\Selection.cpp:819-930`）会：

1. 先移除旧 preselect（841）
2. 检查 selection gate / filter（843-894）
3. 保存 `DocName / FeatName / SubName / x y z`（896-901）
4. 构造 `SelectionChanges`
5. `notify(Chng)` 广播出去（903-925）

### addSelection 更新

`SelectionSingleton::addSelection(...)`（`src\Gui\Selection\Selection.cpp:1181-1270`）会：

1. 更新 picked list（1192-1208）
2. 做选择合法性检查（1210-1245）
3. 压入 `_SelList`（1251）
4. 必要时清 preselect（1254-1256）
5. 构造 `SelectionChanges::AddSelection`（1258-1267）
6. 后续再走 notify 流程

所以 `SelectionSingleton` 是真正的“选中对象真相来源（source of truth）”。

## 8.6 实际拾取：`SoRayPickAction`

在 3D 视图中，具体拾取使用的是 Coin3D 的 `SoRayPickAction`。

`View3DInventorViewer::pickPoint(...)`（`src\Gui\View3DInventorViewer.cpp:3298-3330`）里：

- 用当前 viewport region 构造 `SoRayPickAction`
- `setPoint(pos)`
- `apply(sceneGraph)`
- 从 `getPickedPoint()` 取最近命中结果

这说明几何拾取本质上仍然是 **Coin3D 的 ray pick**，不是 Qt 自己的 hit test。

同时 `View3DInventorViewer::getPickedPoint(...)` 还会优先走：

- `selectionRoot->getPickedList(n->getAction(), true)`（`src\Gui\View3DInventorViewer.cpp:3333-3340`）

这说明 FreeCAD 在基础 ray pick 之上又封装了一层 selection-aware 的 picked list 逻辑。

## 8.7 事件从哪里进来

3D 事件不是直接在 `QMousePressEvent` 里完成全部逻辑，而是经由 Quarter/Coin3D 事件体系流转。

证据：

- `QuarterWidget` 内部维护 `SoEventManager`（`src\Gui\Quarter\QuarterWidget.h:165-169`）
- `QuarterWidget.cpp` 会同步 viewport region 给 render manager / event manager（例如 `QuarterWidget.cpp:701, 746, 839-840, 858-859`）
- `View3DInventorViewer::processSoEvent()` 则处理已经转换成 Coin3D 的 `SoEvent`（`src\Gui\View3DInventorViewer.cpp:2778-2808`）

`processSoEvent()` 的职责是：

- 先让 NaviCube 消费事件（2782-2783）
- 若是重定向模式，则交给父类 Quarter 流程（2785-2792）
- 普通情况下走 navigation style（2808）

也就是说：

```text
Qt 原生输入事件
  -> Quarter/SoEventManager 转成 SoEvent
  -> View3DInventorViewer::processSoEvent()
  -> 导航 / 选择 / 场景图 handleEvent
```

## 8.8 选择完整技术路径

把整个链路串起来，可以概括为：

### 鼠标悬停预选

```text
Qt 鼠标移动
  -> Quarter 事件系统 / SoEventManager
  -> SoFCUnifiedSelection::handleEvent()
  -> 计算 picked list / 当前命中对象
  -> SelectionSingleton::setPreselect(...)
  -> 广播 SelectionChanges::SetPreselect
  -> View3DInventorViewer::onSelectionChanged()
  -> SoFCPreselectionAction.apply(pcViewProviderRoot)
  -> 场景图高亮刷新
```

### 鼠标点击选中

```text
Qt 鼠标点击
  -> Quarter / Coin3D SoEvent
  -> SoFCUnifiedSelection / pick list
  -> SelectionSingleton::addSelection(...)
  -> 广播 SelectionChanges::AddSelection
  -> View3DInventorViewer::onSelectionChanged()
  -> SoFCSelectionAction.apply(pcViewProviderRoot)
  -> View3DInventorSelection::checkGroupOnTop(...)
  -> 场景中对象显示为选中态 / 置顶态
```

### 树视图反向驱动 3D

`SelectionChanges` 里有 `MsgSource::TreeView`（`src\Gui\Selection\Selection.h:93-99`）。

`View3DInventorViewer::onSelectionChanged()` 对 `TreeView` 来源做了特殊判断（`src\Gui\View3DInventorViewer.cpp:855-867`），说明：

- 选择不仅能从 3D 驱动 UI
- 也能从树、命令等其它界面反向驱动 3D 场景高亮

这就是 FreeCAD 选择系统的核心设计：**选择状态统一，显示层多端同步。**

---

## 9. OpenGL 渲染区与 Selection 的边界关系

这两者在 FreeCAD 里并不是硬分开的，而是“共享 Coin3D 场景图”。

### 渲染区负责

- OpenGL context 与 viewport 宿主
- 场景图渲染
- 导航、镜头、背景、灯光、编辑根、对象组

### Selection 系统负责

- 场景图中的 pick / preselect / selection 语义
- 统一选中状态广播
- 视觉高亮 / 置顶显示 / detail path 管理

### 交界处

交界处就在：

- `View3DInventorViewer`
- `SoFCUnifiedSelection`
- `SelectionSingleton`

可以把这三者理解为：

- `View3DInventorViewer`：3D 视图控制器
- `SoFCUnifiedSelection`：3D 场景内选择节点
- `SelectionSingleton`：应用级选择状态中心

---

## 10. 命令行 / 无 GUI 模式与 GUI 模式的边界

这一点对理解架构很重要。

### 无 GUI 模式

`src\Main\MainCmd.cpp`：

- 只调用 `App::Application::init(...)`（`MainCmd.cpp:77-86`）
- 运行 `Application::runApplication()`（`MainCmd.cpp:133-136`）
- 不需要 `Gui::Application::initApplication()`

### GUI Python 模块模式

`src\Main\FreeCADGuiPy.cpp` 提供了：

- `showMainWindow()`
- `exec_loop()`
- `setupWithoutGUI()`

其中 `setupWithoutGUI()` 会：

- 构造 `Gui::Application(false)`
- 初始化 `SoDB`, `SoNodeKit`, `SoInteraction`, `SoFCDB`

见 `src\Main\FreeCADGuiPy.cpp:194-217`。

这意味着：

- `FreeCADGui` 模块即使在“未显示主窗口”的场景下，也可以建立部分 GUI/Inventor 基础设施；
- 但真正主窗口、工作台、完整 3D 视图仍然由 GUI 模式进一步完成。

---

## 11. 一张表看清四类边界

| 层 | 主要目录 | 主要技术 | 核心职责 | 不负责什么 |
| --- | --- | --- | --- | --- |
| 核心基础层 | `src\Base` | C++、Python C API、PyCXX | 解释器、绑定基础设施、基础数学/工具 | GUI 框架、3D 交互界面 |
| 核心应用层 | `src\App` | C++ | 文档模型、属性系统、Feature、事务、应用对象 | Qt 控件、视图布局 |
| GUI 框架层 | `src\Gui` | Qt/C++、Coin3D、Quarter | 主窗口、工作台、3D viewer、selection、命令系统 | 核心几何对象定义 |
| Python 扩展层 | `src\Mod\*`、`Init.py`、`InitGui.py` | Python | 工作台、命令装配、业务扩展、脚本化流程 | 不能替代 C++ 核心对象模型 |

再单独看 PySide：

| 技术 | 在 FreeCAD 中的定位 |
| --- | --- |
| PySide2 / PySide6 | Qt 对象桥接层、UI loader、Python 操作 QWidget/QAction 等 |
| FreeCAD 自身 wrapper | FreeCAD 核心对象绑定主体系 |

---

## 12. 对你关心问题的直接回答

## 12.1 QtC++ 与 PySide6 的边界是什么？

边界是：

- **Qt/C++**：主 GUI 骨架、主窗口、视图、3D viewer、命令系统、工作台容器
- **PySide6**：让 Python 可以接入 Qt 对象与 `.ui` 体系的桥接层

PySide6 不是 FreeCAD 主 GUI 的主实现语言，而是 **Python 侧拿 Qt 对象的桥**。

## 12.2 Python wrapper 与 PySide6 的边界是什么？

- **FreeCAD wrapper**：把 `Document`、`ViewProvider`、`Selection`、`Vector`、`Placement` 等 FreeCAD 对象导出给 Python
- **PySide wrapper**：把 `QObject/QWidget/QAction/QImage/...` 导出给 Python

前者是 FreeCAD 领域对象绑定，后者是 Qt 对象绑定。

## 12.3 核心功能和工作台边界是什么？

- 核心功能在 `Base/App/Gui` 里定义基础能力
- 工作台在 `Mod/*` 和 `InitGui.py` 里按需挂接
- 工作台能扩展菜单、工具栏、命令、上下文菜单，但通常不拥有主程序底座

## 12.4 OpenGL 渲染区域技术栈是什么？

从外到内是：

```text
Qt MDI/Widget
  -> View3DInventor
    -> View3DInventorViewer
      -> SoQTQuarterAdaptor
        -> QuarterWidget(QGraphicsView)
          -> QOpenGLWidget viewport
            -> SoRenderManager / SoGLRenderAction
              -> Coin3D/Open Inventor scene graph
                -> FreeCAD 自定义节点与 render action
                  -> OpenGL
```

## 12.5 select 拾取是怎么处理的？

从技术路径看：

```text
Qt 输入
  -> Quarter/Coin3D SoEvent
  -> SoFCUnifiedSelection handleEvent / getPickedList
  -> SoRayPickAction 获取命中结果
  -> SelectionSingleton 更新全局 selection/preselection
  -> SelectionChanges 广播
  -> View3DInventorViewer::onSelectionChanged
  -> SoFCSelectionAction / SoFCPreselectionAction
  -> 3D 场景高亮、选中、置顶显示更新
```

因此 FreeCAD 的选择不是“viewer 内部孤立逻辑”，而是 **场景图内 pick + 应用级 selection state + 视图回放 action** 的三段式设计。

---

## 13. 对 OpenGeoLab 参考实现的启发

如果你是为了给 OpenGeoLab 参考 FreeCAD 架构，这里有几个非常值得借鉴的点：

1. **Core / Gui 明确分层**
   - 让模型和几何层不依赖 GUI，后续 CLI、批处理、脚本自动化会很舒服。

2. **选择状态统一中心化**
   - 把“谁被选中”收口到一个全局服务，再让 3D / Tree / PropertyView 去观察它，而不是每个视图各管各的。

3. **工作台 / 模块延迟装配**
   - 启动先建框架，再扫描和挂接模块，能很好控制体量与扩展性。

4. **Qt 对象桥接与领域对象绑定分离**
   - 不要把“Qt for Python”误当作“整个系统的 Python API”；领域对象绑定应该有自己的稳定模型。

5. **3D Viewer 不直接绑死 OpenGL 细节**
   - FreeCAD 把 Qt 宿主、场景图、选择动作、渲染动作分层，这比把所有交互揉进一个 `QOpenGLWidget` 更可维护。

---

## 14. 关键证据文件索引

### 分层 / 启动

- `src\CMakeLists.txt`
- `src\Main\MainGui.cpp`
- `src\Main\MainCmd.cpp`
- `src\Main\MainPy.cpp`
- `src\Main\FreeCADGuiPy.cpp`

### App / Gui 初始化脚本

- `src\App\FreeCADInit.py`
- `src\Gui\FreeCADGuiInit.py`

### Python 绑定与桥接

- `src\Base\Interpreter.h/.cpp`
- `src\Base\PyObjectBase.h`
- `src\Base\GeometryPyCXX.h/.cpp`
- `src\Gui\Application.cpp`
- `src\Gui\PythonWrapper.cpp`
- `src\Gui\UiLoader.cpp`
- `cMake\FreeCAD_Helpers\SetupShibokenAndPyside.cmake`
- `cMake\FindPySide6.cmake`
- `src\Gui\CMakeLists.txt`

### 3D / 渲染

- `src\Gui\View3DInventor.cpp/.h`
- `src\Gui\View3DInventorViewer.cpp/.h`
- `src\Gui\Quarter\QuarterWidget.h/.cpp`
- `src\Gui\Quarter\SoQTQuarterAdaptor.h`

### 选择 / 拾取

- `src\Gui\Selection\Selection.h/.cpp`
- `src\Gui\Selection\SoFCUnifiedSelection.h`
- `src\Gui\View3DInventorSelection.cpp/.h`
- `src\Gui\View3DInventorViewer.cpp`

---

## 15. 最终总结

FreeCAD 的真实边界可以浓缩成一句话：

> **C++ 负责核心模型、GUI 骨架、3D viewer 与选择系统；Python 负责模块装配与工作台扩展；PySide 负责 Qt 对象桥接；Coin3D/Quarter 负责把 Qt 事件系统与 Open Inventor/OpenGL 场景图连接起来。**

如果只看表层，很容易误以为 FreeCAD 是 “Qt + Python workbench + PySide” 的应用；  
但真正支撑它长期演进的，是更深一层的分治：

- `Base/App` 保持核心对象稳定；
- `Gui` 把视图、命令、3D、selection 做成框架；
- `Mod` 在框架之上增量挂接；
- `PySide` 只做 Qt bridge，不吞掉领域模型绑定。

这也是 FreeCAD 作为大型桌面 CAD 系统仍能保持高度可扩展性的关键。

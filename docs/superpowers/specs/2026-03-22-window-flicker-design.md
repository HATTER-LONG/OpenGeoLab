# 修复窗口拖拽缩放黑色闪烁

## 问题

Windows + Qt 6 OpenGL 后端，拖拽缩放主窗口时出现黑色闪烁。
根因：未配置 `QSurfaceFormat`，OS 通过 `WM_ERASEBKGND` 用黑色清空客户区。

## 方案

在 `main.cpp` 中 `QApplication` 创建之前，设置全局 `QSurfaceFormat`：

```cpp
QSurfaceFormat fmt;
fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffering);
fmt.setSwapInterval(1);
QSurfaceFormat::setDefaultFormat(fmt);
```

## 受影响文件

- `src/app/src/main.cpp`（添加 4 行，增加 1 个 include）

## 验证

- 构建通过
- 手动验证：拖拽窗口边框缩放，无黑色闪烁

# 中文切换支持 — Qt 翻译基础设施

## 问题陈述

当前 QML 界面已使用 `qsTr()` 包裹所有用户可见字符串，菜单中有"Switch to Chinese"按钮，但缺少 Qt 翻译基础设施（无 `.ts` 文件、无 `QTranslator` 加载、无 `retranslate` 调用），按钮点击后语言不会真正切换。

## 设计方案

### 1. TranslationManager C++ 类

新增 `TranslationManager`，作为 QML singleton 暴露：

```cpp
class TranslationManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString currentLanguage READ currentLanguage
               WRITE switchLanguage NOTIFY currentLanguageChanged)

public:
    /// Qt QML singleton 工厂方法，由引擎自动调用
    static TranslationManager* create(QQmlEngine* engine, QJSEngine*);

    QString currentLanguage() const;
    Q_INVOKABLE void switchLanguage(const QString& locale);

signals:
    void currentLanguageChanged();

private:
    explicit TranslationManager(QQmlEngine* engine, QObject* parent = nullptr);

    QQmlEngine* m_engine;
    QTranslator m_translator;
    QString m_currentLanguage{"en_US"};
};
```

**生命周期说明：** 使用 `static create()` 工厂方法配合 `QML_SINGLETON`，由 QML 引擎自动创建实例，无需在 main.cpp 中手动构造或注册。构造函数为 private。

`switchLanguage()` 流程：
1. 移除旧 translator
2. 加载对应 `.qm` 文件
3. 安装新 translator
4. 调用 `m_engine->retranslate()`
5. 发射 `currentLanguageChanged()`

### 2. CMake 翻译集成

使用 `qt_add_translations()` 管理 `.ts` → `.qm` 编译：

```cmake
qt_add_translations(opengeolab_app
    TS_FILES resource/translations/opengeolab_zh_CN.ts
    RESOURCE_PREFIX "/translations"
)
```

首次生成 `.ts` 文件：`cmake --build build --target update_translations`

### 3. QML 改动

**Main.qml：**
- 删除 `property string currentLanguage`
- 删除 `function toggleLanguage()`
- 改为 `TranslationManager.currentLanguage`
- `toggleLanguage` 调用改为 `TranslationManager.switchLanguage(...)`

**HeaderMenuPanel.qml / AppHeader.qml：**
- `currentLanguage` 绑定改为读取 `TranslationManager.currentLanguage`
- `requestLanguageToggle` 信号链路简化

### 4. 翻译内容（约 40 条）

提供完整的中文翻译，覆盖所有 `qsTr()` 字符串。

### 5. Instructions 更新

在 `.github/instructions/qt-qml.instructions.md` 中新增 i18n 规则段落。

## 受影响文件

| 文件 | 改动类型 |
|------|---------|
| `src/app/include/opengeolab/app/translation_manager.h` | 新增 |
| `src/app/src/translation_manager.cpp` | 新增 |
| `src/app/src/main.cpp` | 无需改动（singleton 由引擎自动创建） |
| `src/app/CMakeLists.txt` | 修改：添加源文件、qt_add_translations |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | 新增 |
| `src/app/resource/qml/Main.qml` | 修改：使用 TranslationManager |
| `src/app/resource/qml/sections/AppHeader.qml` | 修改：简化语言传递 |
| `src/app/resource/qml/sections/HeaderMenuPanel.qml` | 修改：简化语言传递 |
| `.github/instructions/qt-qml.instructions.md` | 修改：追加 i18n 规则 |

## 不在范围内

- 第三语言支持（只做 en_US ↔ zh_CN 切换基础）
- 语言持久化（重启后恢复默认英文）
- 日期/数字本地化格式

## 验证策略

1. `cmake --build build --config RelWithDebInfo --parallel 4` — 构建通过
2. `ctest --test-dir build -C RelWithDebInfo --output-on-failure` — 回归通过
3. 手动验证：启动应用 → 点击"Switch to Chinese" → 所有界面文本切换为中文
4. 再点击"切换到英文" → 恢复英文

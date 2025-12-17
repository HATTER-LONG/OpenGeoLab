# Doxygen Comment Guidelines

> **OpenGeoLab Documentation Style**  
> 目标：
> - 注释可生成文档
> - 接口语义清晰
> - 不增加长期维护成本

---

## 1. 总体原则

### 1.1 注释三原则

所有注释**只解决以下问题**：

1. 这是干什么的？
2. 怎么用？
3. 有什么约束 / 风险 / 设计原因？

不能回答以上问题的注释，应当删除。

---

### 1.2 禁止行为

❌ 注释复述代码本身含义

```cpp
// increment i
i++;
```

❌ 为显而易见的 getter / setter 写注释

---

## 2. 文件级注释规范

### 2.1 Header 文件（必须）

```cpp
/**
 * @file logger.h
 * @brief Global logging interface for OpenGeoLab
 */
```

要求：
- 必须包含 `@file`
- 必须包含 `@brief`
- 简洁描述文件职责

---

### 2.2 Source 文件（推荐）

```cpp
/**
 * @file logger.cpp
 * @brief Implementation of the global logger
 *
 * Uses Meyers Singleton for thread-safe initialization.
 */
```

仅在以下情况推荐：
- 有全局状态 / 单例
- 有非直观实现策略
- 有线程 / IO 副作用

---

## 3. 类注释规范

### 3.1 适用范围

必须注释：
- 对外可见类
- 跨模块使用的类

可省略：
- 仅在 `.cpp` 内部使用的私有实现类

---

### 3.2 最小类注释模板

```cpp
/**
 * @brief Thread-safe global logger
 *
 * Provides formatted logging with multiple severity levels.
 * Implemented as a Meyers Singleton.
 */
class Logger {
```

规则：
- 不复述类名
- 描述职责 + 设计选择

---

## 4. 函数注释规范

### 4.1 何时必须写函数注释

满足任一条件：
- 对外 API
- 有副作用（IO / 全局状态 / 线程）
- 参数存在约束
- 返回值非显而易见

---

### 4.2 最小函数注释模板

```cpp
/**
 * @brief Log a message with given severity
 *
 * @param level Logging level
 * @param message UTF-8 encoded log message
 */
void log(LogLevel level, std::string_view message);
```

---

### 4.3 带返回值函数

```cpp
/**
 * @brief Load mesh from file
 *
 * @param path Input file path
 * @return True if loading succeeds, false otherwise
 */
bool loadMesh(const std::string& path);
```

---

### 4.4 约束 / 风险说明（强烈推荐）

```cpp
/**
 * @brief Initialize logging system
 *
 * @note Must be called before any logging happens
 * @warning Not thread-safe
 */
void initialize();
```

---

## 5. Doxygen 标签使用格式规范（重点）

### 5.1 注释块格式

统一使用 `/** ... */`：

```cpp
/**
 * @brief Brief description
 *
 * Detailed description (optional).
 */
```

- 每行以 `*` 开头
- `@brief` 放在首行
- 空行用于分隔语义块

---

### 5.2 标签书写规范

#### 5.2.1 @brief

- 必须是**完整句子或短语**
- 使用第三人称

✅ 正确：
```cpp
@brief Load mesh data from disk
```

❌ 错误：
```cpp
@brief This function loads mesh
```

---

#### 5.2.2 @param

格式：

```cpp
@param <name> <description>
```

规范：
- 参数名必须与函数签名一致
- 只描述**非显然信息**

示例：
```cpp
@param timeout Timeout in milliseconds, must be > 0
```

---

#### 5.2.3 @return

- 仅在返回值**非 void 或非显然**时使用

```cpp
@return True on success, false on failure
```

---

#### 5.2.4 @note

用于补充说明、使用建议：

```cpp
@note This function does not allocate memory
```

---

#### 5.2.5 @warning

用于强调风险、误用后果：

```cpp
@warning Not thread-safe. Caller must ensure synchronization.
```

---

### 5.3 枚举与行尾注释

```cpp
/**
 * @brief Logging severity levels
 */
enum class LogLevel {
    Trace,   ///< Very detailed diagnostic output
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};
```

规则：
- 使用 `///<` 作为枚举值行尾注释
- 简短描述含义

---

## 6. 成员变量注释规范

### 6.1 何时需要

仅在以下情况允许：
- 状态机变量
- 多线程共享数据
- 位标志 / 掩码

```cpp
/// Guards access to the output stream
std::mutex m_mutex;
```

---

## 7. 实现动机注释（.cpp 推荐）

```cpp
// Use function-local static to avoid static initialization order issues
static Logger instance;
```

此类注释用于解释：
- 为什么这样实现
- 为什么不用更直观的写法

---

## 8. 推荐使用的最小 Doxygen 标签集

| 标签 | 用途 |
|------|------|
| `@file` | 文件说明 |
| `@brief` | 简要描述 |
| `@param` | 参数 |
| `@return` | 返回值 |
| `@note` | 备注 |
| `@warning` | 风险说明 |

禁止引入未统一的标签。

---

## 9. 一句话规范总结

> **只为接口、约束和设计原因写注释；  
> 不为显而易见的代码写注释。**

---

*This document is intended to evolve with the OpenGeoLab codebase.*


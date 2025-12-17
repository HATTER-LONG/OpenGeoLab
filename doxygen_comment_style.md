# Doxygen Comment Guidelines

> **OpenGeoLab – Documentation Style**  
> Goals:
> - Generate useful documentation
> - Make interfaces self-explanatory
> - Avoid unnecessary maintenance overhead

---

## 1. General Principles

### 1.1 The Three Questions Rule

Every comment must answer **at least one** of the following:

1. What does this do?
2. How should it be used?
3. What constraints, risks, or design decisions exist?

If a comment answers none of these, it should be removed.

---

### 1.2 Forbidden Comments

❌ Restating what the code already says

```cpp
// increment i
i++;
```

❌ Commenting obvious getters / setters

---

## 2. File-Level Comment Rules

### 2.1 Header Files (Required)

```cpp
/**
 * @file logger.h
 * @brief Global logging interface for OpenGeoLab
 */
```

Requirements:
- `@file` is mandatory
- `@brief` is mandatory
- Description must clearly state the file responsibility

---

### 2.2 Source Files (Recommended)

```cpp
/**
 * @file logger.cpp
 * @brief Implementation of the global logger
 *
 * Uses Meyers Singleton for thread-safe initialization.
 */
```

Recommended only when:
- Global state or singletons are involved
- Non-obvious implementation strategies are used
- Threading or IO side effects exist

---

## 3. Class Comment Rules

### 3.1 Scope

Must be documented:
- Public or exported classes
- Classes shared across modules

May be omitted:
- Private implementation details
- Classes local to a `.cpp` file

---

### 3.2 Minimal Class Comment Template

```cpp
/**
 * @brief Thread-safe global logger
 *
 * Provides formatted logging with multiple severity levels.
 * Implemented as a Meyers Singleton.
 */
class Logger {
```

Rules:
- Do not repeat the class name
- Focus on responsibility and design choices

---

## 4. Function Comment Rules

### 4.1 When Function Comments Are Required

Document a function if **any** of the following applies:
- It is part of the public API
- It has side effects (IO, global state, threading)
- Parameters have constraints
- The return value is non-obvious

---

### 4.2 Minimal Function Comment Template

```cpp
/**
 * @brief Log a message with the given severity
 *
 * @param level Logging level
 * @param message UTF-8 encoded log message
 */
void log(LogLevel level, std::string_view message);
```

---

### 4.3 Functions with Return Values

```cpp
/**
 * @brief Load mesh from file
 *
 * @param path Input file path
 * @return True on success, false otherwise
 */
bool loadMesh(const std::string& path);
```

---

### 4.4 Constraints and Risks (Strongly Recommended)

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

## 5. Doxygen Tag Usage and Formatting (Important)

### 5.1 Comment Block Format

Always use `/** ... */`:

```cpp
/**
 * @brief Brief description
 *
 * Optional detailed description.
 */
```

Rules:
- Each line starts with `*`
- `@brief` comes first
- Blank lines separate logical sections

---

### 5.2 Tag Formatting Rules

#### 5.2.1 @brief

- Must be a complete phrase or sentence
- Use third-person present tense

✅ Correct:
```cpp
@brief Load mesh data from disk
```

❌ Incorrect:
```cpp
@brief This function loads mesh
```

---

#### 5.2.2 @param

Format:

```cpp
@param <name> <description>
```

Rules:
- Parameter name must exactly match the function signature
- Describe only **non-obvious** semantics or constraints

Example:
```cpp
@param timeout Timeout in milliseconds, must be > 0
```

---

#### 5.2.3 @return

- Use only when the return value is non-void or non-obvious

```cpp
@return True on success, false on failure
```

---

#### 5.2.4 @note

Use for additional usage notes or guarantees:

```cpp
@note This function does not allocate memory
```

---

#### 5.2.5 @warning

Use to highlight risks or incorrect usage:

```cpp
@warning Not thread-safe. Caller must ensure synchronization.
```

---

### 5.3 Enums and Trailing Comments

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

Rules:
- Use `///<` for trailing enum value comments
- Keep descriptions short and precise

---

## 6. Member Variable Comment Rules

### 6.1 When Member Comments Are Allowed

Member variable comments are allowed only when:
- The variable represents a state machine
- The variable is shared across threads
- The variable is a bitmask or flag

```cpp
/// Guards access to the output stream
std::mutex m_mutex;
```

---

## 7. Implementation Rationale Comments (.cpp Recommended)

```cpp
// Use function-local static to avoid static initialization order issues
static Logger instance;
```

These comments should explain:
- Why this implementation was chosen
- Why a more obvious approach was avoided

---

## 8. Approved Minimal Doxygen Tag Set

| Tag | Purpose |
|-----|---------|
| `@file` | File description |
| `@brief` | Brief description |
| `@param` | Function parameters |
| `@return` | Return value |
| `@note` | Notes |
| `@warning` | Warnings |

Do not introduce additional tags without team agreement.

---

## 9. One-Sentence Guideline Summary

> **Write comments for interfaces, constraints, and design decisions —  
> not for obvious code behavior.**

---

*This document is intended to evolve with the OpenGeoLab codebase.*

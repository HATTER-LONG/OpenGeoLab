# Doxygen Comment Guidelines

**OpenGeoLab Documentation Style** – Generate documentation, clarify interfaces, minimize maintenance.

---

## 1. Core Principles

**Comment Purpose** – Answer one or more:
1. What does this do?
2. How to use?
3. Constraints / risks / design rationale?

**Forbidden:**
- Restating obvious code (`i++; // increment i`)
- Commenting trivial getters/setters

---

## 2. File Comments

**Header files (required):**
- `@file <filename>`
- `@brief <responsibility>`

**Source files (optional):**
- Add only if: global state, non-obvious strategy, threading/IO effects

---

## 3. Class Comments

**Required for:**
- Public/exported classes
- Cross-module classes

**Format:**
```cpp
/**
 * @brief <responsibility>
 * <design choices if non-obvious>
 */
```

**Skip for:** Private implementation classes in `.cpp` files

---

## 4. Function Comments

**Required when:**
- Public API
- Has side effects (IO/global state/threading)
- Parameters have constraints
- Return value non-obvious

**Template:**
```cpp
/**
 * @brief <action>
 * @param <name> <constraint or semantic info>
 * @return <condition>
 * @note <usage notes>
 * @warning <risks>
 */
```

---

## 5. Doxygen Tags

**Block format:** `/** ... */` with `*` prefix per line

**Tag rules:**

| Tag | Usage | Format |
|-----|-------|--------|
| `@file` | File name | Required for headers |
| `@brief` | Summary | Third-person, complete phrase |
| `@param` | Parameter | `@param <name> <non-obvious info>` |
| `@return` | Return value | Use only if non-void/non-obvious |
| `@note` | Usage notes | Optional clarifications |
| `@warning` | Risks | Highlight thread-safety, preconditions |

**Enum comments:** Use `///<` for trailing member docs

---

## 6. Member Variables

**Comment only when:**
- State machine variable
- Thread-shared data
- Bitmask/flag

**Format:** `/// <purpose>`

---

## 7. Implementation Notes (.cpp)

Explain **why** (not what) for non-obvious implementations:
```cpp
// Rationale for this approach vs. alternatives
```

---

## 8. Approved Tag Set

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

/**
 * @file EmbeddedPythonRuntime.hpp
 * @brief In-process Python runtime used to drive application state through a scripted API.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>

namespace OGL::App {

/**
 * @brief Owns the embedded interpreter and executes scripts against a request-processing callback.
 */
class EmbeddedPythonRuntime {
public:
    using ProcessRequestHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    explicit EmbeddedPythonRuntime(ProcessRequestHandler processRequestHandler);
    ~EmbeddedPythonRuntime();

    EmbeddedPythonRuntime(const EmbeddedPythonRuntime&) = delete;
    auto operator=(const EmbeddedPythonRuntime&) -> EmbeddedPythonRuntime& = delete;
    EmbeddedPythonRuntime(EmbeddedPythonRuntime&&) = delete;
    auto operator=(EmbeddedPythonRuntime&&) -> EmbeddedPythonRuntime& = delete;

    /**
     * @brief Execute a Python script against the built-in application API.
     * @param script Python source code.
     * @return Combined stdout and stderr captured during execution.
     */
    auto executeScript(const std::string& script) -> std::string;

    /**
     * @brief Execute one line of Python as a REPL-style expression or statement.
     * @param commandLine Python expression or statement.
     * @return Captured expression result or combined stdout and stderr.
     */
    auto executeCommandLine(const std::string& commandLine) -> std::string;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OGL::App

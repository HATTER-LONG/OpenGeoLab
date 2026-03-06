#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32

inline std::wstring readWindowsEnvironmentVariable(const wchar_t* name) {
    const DWORD required_size = GetEnvironmentVariableW(name, nullptr, 0);
    if(required_size == 0) {
        return L"";
    }
    std::wstring value(required_size - 1, L'\0');
    GetEnvironmentVariableW(name, value.data(), required_size);
    return value;
}

inline std::wstring readWindowsCurrentDirectory() {
    const DWORD required_size = GetCurrentDirectoryW(0, nullptr);
    if(required_size == 0) {
        return L"";
    }
    std::wstring value(required_size - 1, L'\0');
    GetCurrentDirectoryW(required_size, value.data());
    return value;
}

inline std::wstring readWindowsDllDirectory() {
    const DWORD required_size = GetDllDirectoryW(0, nullptr);
    if(required_size == 0) {
        return L"";
    }
    std::wstring value(required_size, L'\0');
    const DWORD written = GetDllDirectoryW(required_size, value.data());
    value.resize(written);
    return value;
}

class WindowsProcessPathGuard {
public:
    WindowsProcessPathGuard()
        : m_currentDirectory(readWindowsCurrentDirectory()),
          m_dllDirectory(readWindowsDllDirectory()),
          m_path(readWindowsEnvironmentVariable(L"PATH")) {}

    ~WindowsProcessPathGuard() { restore(); }

    WindowsProcessPathGuard(const WindowsProcessPathGuard&) = delete;
    WindowsProcessPathGuard& operator=(const WindowsProcessPathGuard&) = delete;

private:
    void restore() const {
        if(!m_currentDirectory.empty()) {
            SetCurrentDirectoryW(m_currentDirectory.c_str());
        }
        if(m_dllDirectory.empty()) {
            SetDllDirectoryW(nullptr);
        } else {
            SetDllDirectoryW(m_dllDirectory.c_str());
        }
        if(!m_path.empty()) {
            SetEnvironmentVariableW(L"PATH", m_path.c_str());
        }
    }

    std::wstring m_currentDirectory;
    std::wstring m_dllDirectory;
    std::wstring m_path;
};

#else

struct WindowsProcessPathGuard {
    WindowsProcessPathGuard() = default;
};

#endif

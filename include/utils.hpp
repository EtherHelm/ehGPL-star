#pragma once
#include <string>
#include <filesystem>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// Get the directory of the current module (DLL or SO)
static std::string getDllDir()
{
#if defined(_WIN32) || defined(_WIN64)
    wchar_t dllPath[MAX_PATH] = {0};
    HMODULE hModule = nullptr;
    // Get the current module handle
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)getDllDir, &hModule);
    GetModuleFileNameW(hModule, dllPath, MAX_PATH);
    std::wstring wpath(dllPath);
    size_t pos = wpath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        wpath = wpath.substr(0, pos + 1);
    else
        wpath.clear();
    // Convert to UTF-8 string
    int len = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string path(len > 1 ? len - 1 : 0, 0);
    if (len > 1)
        WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &path[0], len, nullptr, nullptr);
    return path;
#else
    Dl_info info;
    if (dladdr((void*)&getDllDir, &info)) {
        std::string fullPath = info.dli_fname;
        std::filesystem::path absPath = std::filesystem::absolute(fullPath);
        size_t pos = absPath.string().find_last_of("/\\");
        if (pos != std::string::npos)
            return absPath.string().substr(0, pos + 1);
    }
    return "";
#endif
}

static std::string hashString(const std::string& input) {
    std::hash<std::string> hasher;
    size_t hashed = hasher(input);
    return std::to_string(hashed);
}

static std::string getAbsolutePath(const std::string& path) {
    std::string basePath = getDllDir();
    if (std::filesystem::path(path).is_absolute()) {
        return path;
    } else {
        std::filesystem::path absPath = std::filesystem::path(basePath) / path;
        return absPath.lexically_normal().string();
    }
}
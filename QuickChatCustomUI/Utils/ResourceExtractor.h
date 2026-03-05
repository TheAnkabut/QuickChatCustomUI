#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <windows.h>

// Extract an embedded RCDATA resource to a file (skips if already on disk)
// Returns the path on success, empty string on failure
inline std::string ExtractResource(HMODULE hModule, int resourceId, const std::string& destPath)
{
    if (std::filesystem::exists(destPath)) return destPath;

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return "";

    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) return "";

    std::ofstream file(destPath, std::ios::binary);
    if (!file) return "";

    file.write((const char*)LockResource(hData), SizeofResource(hModule, hRes));
    return destPath;
}

// Get the HMODULE of the DLL that contains this code
inline HMODULE GetCurrentModule()
{
    HMODULE hModule = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetCurrentModule,
        &hModule
    );
    return hModule;
}

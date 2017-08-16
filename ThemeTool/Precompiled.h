#pragma once

#include "targetver.h"

#include <string>
#include <vector>

#include <windows.h>
#include <gdiplus.h>

// Utilities

namespace Path
{
    std::wstring Combine(const std::wstring& path1, const std::wstring& path2);
    bool IsRelative(const std::wstring& path);
    std::wstring GetFullName(const std::wstring& path);
    std::wstring GetDirectory(const std::wstring& path);
    std::wstring GetFileName(const std::wstring& path);
    std::wstring GetFileNameWithoutExtension(const std::wstring& path);
    std::wstring GetExtension(const std::wstring& path);
    DWORD GetAttributes(const std::wstring& path);
    bool FileExists(const std::wstring& path);
    bool DirectoryExists(const std::wstring& path);
}

// Tasks

void Unpack(const std::wstring& metadata, const std::wstring& output);
void Pack(const std::wstring& metadata, const std::wstring& source);

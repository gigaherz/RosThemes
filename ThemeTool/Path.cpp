#include "Precompiled.h"

#include <shlwapi.h>

using namespace std;

wstring Path::Combine(const wstring& path1, const wstring& path2)
{
    auto combined = wstring(path1);
    if (*(combined.end() - 1) != L'\\')
    {
        combined.append(1, L'\\');
    }
    combined.append(path2);
    return combined;
}

bool Path::IsRelative(const wstring& path)
{
    return ::PathIsRelativeW(path.c_str());
}

wstring Path::GetFullName(const wstring& path)
{
    vector<wchar_t> text;

    auto needed = ::GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);

    if (needed == 0)
        return L"";

    text.resize(needed);
    ::GetFullPathNameW(path.c_str(), needed, text.data(), nullptr);

    wstring s;
    s.assign(text.begin(), text.end());
    return s;
}

wstring Path::GetDirectory(const wstring& path)
{
    WCHAR drive[_MAX_DRIVE];
    WCHAR directory[_MAX_DIR];
    _wsplitpath(path.c_str(), drive, directory, nullptr, nullptr);
    wstring result;
    result.append(drive);
    result.append(directory);
    return result;
}

wstring Path::GetFileName(const wstring& path)
{
    WCHAR file[_MAX_FNAME];
    WCHAR extension[_MAX_EXT];
    _wsplitpath(path.c_str(), nullptr, nullptr, file, extension);
    wstring result;
    result.append(file);
    result.append(extension);
    return result;
}

wstring Path::GetFileNameWithoutExtension(const wstring& path)
{
    WCHAR file[_MAX_FNAME];
    _wsplitpath(path.c_str(), nullptr, nullptr, file, nullptr);
    return wstring(file);
}

wstring Path::GetExtension(const wstring& path)
{
    WCHAR extension[_MAX_EXT];
    _wsplitpath(path.c_str(), nullptr, nullptr, nullptr, extension);
    return wstring(extension);
}

DWORD Path::GetAttributes(const wstring& path)
{
    return ::GetFileAttributesW(path.c_str());
}

bool Path::FileExists(const wstring& path)
{
    auto attrs = GetAttributes(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Path::DirectoryExists(const wstring& path)
{
    auto attrs = GetAttributes(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

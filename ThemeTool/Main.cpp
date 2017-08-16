#include "Precompiled.h"

wstring PathCombineW(const wstring& path1, const wstring& path2)
{
    auto combined = wstring(path1);
    if (*(path1.end() - 1) != L'\\')
    {
        combined.append(1, L'\\');
    }
    combined.append(path2);
    return combined;
}

BOOL PathIsRelativeW(const wstring& path)
{
    return PathIsRelativeW(path.c_str());
}

wstring PathGetDirectory(const wstring& path)
{
    WCHAR fullpath[MAX_PATH];
    WCHAR drive[_MAX_DRIVE];
    WCHAR directory[_MAX_DIR];
    GetFullPathName(path.c_str(), _countof(fullpath), fullpath, nullptr);
    _wsplitpath(fullpath, drive, directory, nullptr, nullptr);

    wstring result;
    result.append(drive);
    result.append(directory);
    return result;
}

wstring PathGetFileNameWithoutExtension(const wstring& path)
{
    WCHAR file[_MAX_FNAME];
    _wsplitpath(path.c_str(), nullptr, nullptr, file, nullptr);
    return wstring(file);
}

wstring PathGetExtension(const wstring& path)
{
    WCHAR extension[_MAX_EXT];
    _wsplitpath(path.c_str(), nullptr, nullptr, nullptr, extension);
    return wstring(extension);
}

BOOL FileExists(const wstring& path)
{
    auto attrs = GetFileAttributes(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL DirectoryExists(const wstring& path)
{
    auto attrs = GetFileAttributes(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

void DisplayHelp(wstring arg0)
{
    wcout << L"Command Line: " << arg0 << L" <mode> <pack file> [<output folder>] [<options>]" << endl;
    wcout << L"Options:" << endl;
    wcout << L"\tNothing here yet." << endl;
}

void wmain(int argc, PCWSTR* argv)
{
    wstring packFile = L"";
    wstring bitmapsFolder = L".";

    int mode = -1;
    int values = 0;

    for (int i = 1; i < argc; i++)
    {
        wstring arg = argv[i];
        if (arg.size() > 0 && (arg[0] == L'-' || arg[0] == L'/'))
        {
            if (wcscmp(arg.c_str() + 1, L"U") == 0 || wcscmp(arg.c_str() + 1, L"unpack") == 0)
            {
                if (mode >= 0)
                {
                    wcerr << L"Only one mode flag allowed." << endl << endl;
                    DisplayHelp(argv[0]);
                    return;
                }
                mode = 0;
            }
            else
            if (wcscmp(arg.c_str() + 1, L"P") == 0 || wcscmp(arg.c_str() + 1, L"pack") == 0)
            {
                if (mode >= 0)
                {
                    wcerr << L"Only one mode flag allowed." << endl << endl;
                    DisplayHelp(argv[0]);
                    return;
                }
                mode = 1;
            }
            else
            {
                wcerr << L"Unrecognized option: " << arg << endl << endl;
                DisplayHelp(argv[0]);
                return;
            }
        }
        else
        {
            values++;
            switch (values)
            {
            case 1: packFile = arg; break;
            case 2: bitmapsFolder = arg; break;
            }
        }
    }

    if (values < 1)
    {
        wcerr << L"Pack filename required." << endl << endl;
        DisplayHelp(argv[0]);
        return;
    }

    if (mode < 0)
    {
        wcerr << L"Mode flag required." << endl << endl;
        DisplayHelp(argv[0]);
        return;
    }

    // Initialize GDI+
    ULONG_PTR gdiplus_token;
    GdiplusStartupInput startup_input;

    GdiplusStartup(&gdiplus_token, &startup_input, NULL);

    if (mode == 0)
    {
        Unpack(packFile, bitmapsFolder);
    }
    else
    {
        Pack(packFile, bitmapsFolder);
    }

    GdiplusShutdown(gdiplus_token);
}

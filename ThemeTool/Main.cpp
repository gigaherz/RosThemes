#include "Precompiled.h"

#include <iostream>

using namespace std;
using namespace Gdiplus;

void DisplayHelp(wstring arg0)
{
    wcout << L"Command Line: " << Path::GetFileName(arg0) << L" <mode> <pack file> [<folder>] [<options>]" << endl;
    wcout << L"Arguments:" << endl;
    wcout << L"\t<mode>        One of the mode flags below." << endl;
    wcout << L"\t<pack file>   Metadata file with the instructions on how to unpack." << endl;
    wcout << L"\t<folder>      The folder to scan (pack) or extract to (unpack)." << endl;
    wcout << L"Modes:" << endl;
    wcout << L"\t-u, -unpack   Unpack combined image into separate bitmaps." << endl;
    wcout << L"\t-p, -pack     Pack separate bitmaps into combined image." << endl;
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
            if (wcscmp(arg.c_str() + 1, L"u") == 0 || wcscmp(arg.c_str() + 1, L"unpack") == 0)
            {
                if (mode >= 0)
                {
                    wcerr << L"Only one mode flag allowed." << endl << endl;
                    DisplayHelp(argv[0]);
                    return;
                }
                mode = 0;
            }
            else if (wcscmp(arg.c_str() + 1, L"p") == 0 || wcscmp(arg.c_str() + 1, L"pack") == 0)
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

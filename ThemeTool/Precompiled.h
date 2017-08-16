#pragma once

#include "targetver.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <regex>
#include <fstream>
#include <set>
#include <memory>

#include <windows.h>
#include <shlwapi.h>
#include <gdiplus.h>

using namespace std;
using namespace Gdiplus;

// Utilities

wstring PathCombineW(const wstring& path1, const wstring& path2);
BOOL PathIsRelativeW(const wstring& path);
wstring PathGetDirectory(const wstring& path);
wstring PathGetFileNameWithoutExtension(const wstring& path);
wstring PathGetExtension(const wstring& path);
BOOL FileExists(const wstring& path);
BOOL DirectoryExists(const wstring& path);

// Mode implementations

void Unpack(const wstring& metadata, const wstring& output);
void Pack(const wstring& metadata, const wstring& source);

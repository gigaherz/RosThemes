// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <gdiplus.h>

#include <string>
#include <iostream>

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

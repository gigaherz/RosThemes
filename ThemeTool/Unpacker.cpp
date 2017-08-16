#include "stdafx.h"

using namespace std;
using namespace Gdiplus;

static bool begins_with(const wstring& input, PCWSTR match)
{
    auto matchLen = wcslen(match);
    return input.size() >= matchLen
        && wcsncmp(input.c_str(), match, matchLen) == 0;
}

static size_t indexof(const vector<ARGB>& collection, ARGB element)
{
    auto ptr = collection.data();
    auto size = collection.size();
    auto val = element;
    for (size_t i=0;i<size;i++)
    {
        if (ptr[i] == val)
            return i;
    }
    return -1;
}

static PixelFormat ParsePixelFormat(wstring& text)
{
    if (wcscmp(text.c_str(), L"Format1bppIndexed") == 0)
        return PixelFormat1bppIndexed;

    if (wcscmp(text.c_str(), L"Format4bppIndexed") == 0)
        return PixelFormat4bppIndexed;

    if (wcscmp(text.c_str(), L"Format8bppIndexed") == 0)
        return PixelFormat8bppIndexed;

    if (wcscmp(text.c_str(), L"Format32bppArgb") == 0)
        return PixelFormat32bppARGB;

    return PixelFormat32bppRGB;
}

static int FindBestPixelFormat(Bitmap& source, Rect rect, PixelFormat fmt, ColorPalette* pal, vector<ARGB>& palette, vector<ARGB>& pixels)
{
    palette.clear();
    pixels.clear();

    auto colors = set<ARGB>();

    for (int y = 0; y < rect.Height; y++)
    {
        for (int x = 0; x < rect.Width; x++)
        {
            Color color;
            source.GetPixel(x + rect.GetLeft(), rect.GetBottom() - 1 - y, &color);
            auto argb = color.GetValue();
            colors.insert(argb);
            pixels.push_back(argb);
        }
    }

    if (colors.size() > 256 || (fmt & PixelFormatAlpha))
    {
        return 32;
    }

    for(auto color : colors)
        palette.push_back(color);

    if (palette.size() > 16)
        return 8;

    if (palette.size() > 2)
        return 4;

    return 1;
}

static void SaveCustomBmp(wstring fileName, Bitmap& source, Rect rect, PixelFormat fmt, ColorPalette* pal)
{
    auto alpha = fmt & PixelFormatAlpha;
    if (alpha)
        return;

    vector<ARGB> palette;
    vector<ARGB> pixels;
    WORD bitsPerPixel = FindBestPixelFormat(source, rect, fmt, pal, palette, pixels);

    DWORD palLength = palette.size() == (1 << bitsPerPixel) ? 0 : palette.size();
    
    DWORD palSize = palette.size() * 4;
    DWORD pixelsSize = ((rect.Width * bitsPerPixel + 31) / 32) * 4 * rect.Height;

    DWORD palOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    DWORD pixelsOffset = palOffset + palSize;
    DWORD totalSize = pixelsOffset + pixelsSize;

    char* buffer = new char[totalSize];

    // File Header
    *reinterpret_cast<BITMAPFILEHEADER*>(buffer) = {
        0x4D42,
        totalSize,
        0,
        0,
        pixelsOffset
    };

    *reinterpret_cast<BITMAPINFOHEADER*>(buffer + sizeof(BITMAPFILEHEADER)) = {
        40,
        rect.Width,
        rect.Height,
        1,
        bitsPerPixel,
        0,
        0,
        0x0B12,
        0x0B12,
        palLength,
        palLength
    };

    if (palSize > 0)
    {
        RtlCopyMemory(buffer + palOffset, palette.data(), palSize);
    }

    if (palette.size() > 0)
    {
        if (bitsPerPixel == 1)
        {
            bool flush = (rect.Width & 3) != 0;
            for (int y = 0, i=0, o=0; y < rect.Height; y++)
            {
                int c = 0;
                int b = 0;
                for (int x = 0; x < rect.Width; x++, i++)
                {
                    auto color = pixels[i];
                    auto index = indexof(palette, color);
                    switch (x & 7)
                    {
                    case 0: c = index; break;
                    case 1: c = (c << 1) | index; break;
                    case 2: c = (c << 1) | index; break;
                    case 3: c = (c << 1) | index; break;
                    case 4: c = (c << 1) | index; break;
                    case 5: c = (c << 1) | index; break;
                    case 6: c = (c << 1) | index; break;
                    default:
                        c = (c << 1) | index;
                        buffer[pixelsOffset + o++] = c;
                        b++;
                        break;
                    }
                }
                if (flush)
                {
                    buffer[pixelsOffset + o++] = c;
                    b++;
                }
                while ((b++ % 4) != 0)
                    o++;
            }
        }
        else if (bitsPerPixel < 8)
        {
            bool flush = (rect.Width & 1) != 0;
            for (int y = 0, i=0, o=0; y < rect.Height; y++)
            {
                int c = 0;
                int b = 0;
                for (int x = 0; x < rect.Width; x++, i++)
                {
                    auto color = pixels[i];
                    auto index = indexof(palette, color);
                    if ((x & 1) == 0)
                    {
                        c = index << 4;
                    }
                    else
                    {
                        c = c | index;
                        buffer[pixelsOffset + o++] = c;
                        b++;
                    }
                }
                if (flush)
                {
                    buffer[pixelsOffset + o++] = c;
                    b++;
                }
                while ((b++ % 4) != 0)
                    o++;
            }
        }
        else
        {
            for (int y = 0, i=0, o=0; y < rect.Height; y++)
            {
                int x;
                for (x = 0; x < rect.Width; x++, i++)
                {
                    auto color = pixels[i];
                    int index = indexof(palette, color);
                    buffer[pixelsOffset + o++] = index;
                }
                while ((x++ % 4) != 0)
                    o++;
            }
        }
    }
    else // assumes 32bit
    {
        RtlCopyMemory(buffer + pixelsOffset, pixels.data(), pixels.size() * 4);
    }

    auto outfile = ofstream(fileName.c_str(), ios::out | ios::binary);
    outfile.write(buffer, totalSize);
    outfile.close();
}

void Unpack(const wstring& metadata, const wstring& output)
{
    if (!DirectoryExists(output))
        CreateDirectory(output.c_str(), nullptr);

    Bitmap* image = nullptr;

    wregex line_regex(L"^([^=]+)=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(.*)$",
        regex_constants::ECMAScript | regex_constants::icase);

    wregex meta_regex(L"^#([^=]+)=(.*)$",
        regex_constants::ECMAScript | regex_constants::icase);

    wifstream infile(metadata);
    wstring line;
    while (getline(infile, line))
    {
        int commentChar = line.find(L'#');
        if (commentChar == 0)
        {
            wcmatch results;
            auto matched = regex_match(line.c_str(), results, meta_regex);
            if (matched)
            {
                auto propName = wstring(results[1].first, results[1].second);
                auto metaValue = wstring(results[2].first, results[2].second);
                if (propName == wstring(L"Bitmap"))
                {
                    if (!image)
                    {
                        wstring path = metaValue;
                        if (PathIsRelative(path))
                            path = PathCombine(PathGetDirectory(metadata), metaValue);
                        image = new Bitmap(path.c_str(), FALSE);
                        if (!image || FAILED(image->GetLastStatus()))
                        {
                            wcerr << "Bitmap path could not be loaded: " << path << endl;
                            return;
                        }
                    }
                }
            }
            continue;
        }

        if (!image)
        {
            wcerr << "Bitmap path not present in the pack file." << endl;
            return;
        }

        if (commentChar > 0)
            line = line.substr(0, commentChar);

        //line = line.trim();

        if (line.size() > 0)
        {
            wcmatch results;
            auto matched = regex_match(line.c_str(), results, line_regex);

            if (!matched)
            {
                wcerr << L"Invalid line found in file: " << line << endl;
                return;
            }

            auto fileName = wstring(results[1].first, results[1].second);
            int x = stoi(wstring(results[2].first, results[2].second));
            int y = stoi(wstring(results[3].first, results[3].second));
            int w = stoi(wstring(results[4].first, results[4].second));
            int h = stoi(wstring(results[5].first, results[5].second));
            auto fmt = ParsePixelFormat(wstring(results[6].first, results[6].second));

#if false
            using (auto bb = new Bitmap(new FileStream(Path.Combine(orig, fileName), FileMode.Open, FileAccess.Read, FileShare.Read)))
                SaveCustomBmp(Path.Combine(output, fileName), b, new Rectangle(x, y, w, h), fmt, bb.Palette);
#else
            SaveCustomBmp(PathCombine(output, fileName), *image, Rect(x, y, w, h), fmt, nullptr);
#endif
        }
    }

    delete image;
}

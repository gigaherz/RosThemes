#include "Precompiled.h"
#include <vector>
#include <algorithm>
#include <regex>
#include <fstream>
#include <shlwapi.h>
#include <memory>
#include <queue>

class Entry
{
public:
    Entry(const wstring& fileName, const wstring& label, Bitmap* image)
        : FileName(fileName), Label(label), Image(image)
    {
    }

    wstring LineNumber;
    wstring FileName;
    wstring Label;

    Bitmap* Image;

    int X;
    int Y;
    int W;
    int H;

    int RealW() const { return Image->GetWidth(); }
    int RealH() const { return Image->GetHeight(); }

    bool Vertical() const { return RealH() >= RealW(); }

    RectF LabelSize;
    RectF LineNumberSize;
};

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = nullptr;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = static_cast<ImageCodecInfo*>(malloc(size));
    if (pImageCodecInfo == nullptr)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

static bool Replace(wstring& str, const wstring& from, const wstring& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

static vector<Entry> Scan(const wstring& path)
{
    auto entries = vector<Entry>();

    WIN32_FIND_DATAW fd;

    wstring pattern = path;
    pattern.append(L"\\*.bmp");

    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            wstring fullName = PathCombine(path, fd.cFileName);
            
            Bitmap* src = new Bitmap(fullName.c_str());
            
            auto name = PathGetFileNameWithoutExtension(fd.cFileName);

            auto underscore = name.find_first_of('_');
            if (underscore >= 0)
                name = name.substr(underscore + 1);

            Replace(name, L"RADIOBUTTON", L"[Rb]");
            Replace(name, L"CHECKBOX", L"[Cb]");
            Replace(name, L"HORIZONTAL", L"[H]");
            Replace(name, L"VERTICAL", L"[V]");
            Replace(name, L"BUTTON", L"[Btn]");
            Replace(name, L"EXPLORER", L"[Xp]");
            Replace(name, L"TOOLBAR", L"[Tb]");
            Replace(name, L"TASKBAR", L"[Tkb]");
            Replace(name, L"START", L"[S]");
            Replace(name, L"BACKGROUND", L"[Bg]");

            entries.push_back(Entry(fd.cFileName, name, src));
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    //entries.Sort((a, b) = > string.CompareOrdinal(a.FileName, b.FileName));
    for (auto i = 0; i < entries.size(); i++)
    {
        auto& item = entries[i];
        item.LineNumber = to_wstring(i + 1);
    }

    return entries;
}

static bool ProcessLayout(vector<Entry>& entries, Size size)
{
    auto freeAreas = vector<Rect>();
    
    freeAreas.push_back(Rect(5, 5, size.Width - 10, size.Height - 10));

#if false
    auto itemsSorting = new vector<Entry>(entries);
    itemsSorting.Sort((a, b) = >
    {
        auto r = Math.Sign(b.W * b.H - a.W * a.H);
        if (r != 0) return r;
        auto w = Math.Sign(b.W - a.W);
        if (w != 0) return w;
        auto h = Math.Sign(b.H - a.H);
        if (h != 0) return h;
        return 0;
    });
    auto items = new Queue<Entry>(itemsSorting);
    while (items.size() > 0)
    {
        auto item = items.pop();
#else
    for (auto& item : entries)
    {
#endif
        auto iw = item.W;
        auto ih = item.H;

        auto best = freeAreas.end();
        auto direction = 0;
        auto diff = INT_MAX;
        auto diffOther = INT_MAX;
        for(auto it = freeAreas.begin(); it != freeAreas.end(); ++it)
        {
            auto& rect = *it;
            auto w = rect.Width;
            auto h = rect.Height;

            if (iw > w || ih > h)
                continue;

            auto dw = w - iw;
            auto dh = h - ih;
            if (dw < dh)
            {
                if (dw < diff || (abs(dw - diff) < FLT_EPSILON && dh < diffOther))
                {
                    best = it;
                    diff = dw;
                    diffOther = dh;
                    direction = 1;
                }
            }
            else
            {
                if (dh < diff || (abs(dh - diff) < FLT_EPSILON && dw < diffOther))
                {
                    best = it;
                    diff = dh;
                    diffOther = dw;
                    direction = 2;
                }
            }
        }

        if (best == freeAreas.end())
        {
            //wcerr << L"ERROR: I wasn't able to find a rectangle to put this in!" << endl;
            return false;
        }

        auto chosen = *best;
        freeAreas.erase(best);

        item.X = chosen.X;
        item.Y = chosen.Y;

        if (direction == 1)
        {
            auto t1 = diff * chosen.Height;
            auto t2 = chosen.Width * diffOther;

            int merge = 0;
            if (t1 > t2)
            {
                if (diff > 0) merge = 1;
            }
            else
            {
                if (diffOther > 0) merge = 2;
            }

            if (merge == 1)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y, diff, chosen.Height));
            }
            else if (diff > 0)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y, diff, ih));
            }

            if (merge == 2)
            {
                freeAreas.push_back(Rect(chosen.X, chosen.Y + ih, chosen.Width, diffOther));
            }
            else if (diffOther > 0)
            {
                freeAreas.push_back(Rect(chosen.X, chosen.Y + ih, iw, diffOther));
            }

            if (merge == 0 && diff > 0 && diffOther > 0)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y + ih, diff, diffOther));
            }
        }
        else
        {
            auto t1 = diffOther * chosen.Height;
            auto t2 = chosen.Width * diff;

            int merge = 0;
            if (t1 > t2)
            {
                if (diffOther > 0) merge = 1;
            }
            else
            {
                if (diff > 0) merge = 2;
            }

            if (merge == 1)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y, diffOther, chosen.Height));
            }
            else if (diffOther > 0)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y, diffOther, ih));
            }

            if (merge == 2)
            {
                freeAreas.push_back(Rect(chosen.X, chosen.Y + ih, chosen.Width, diff));
            }
            else if (diff > 0)
            {
                freeAreas.push_back(Rect(chosen.X, chosen.Y + ih, iw, diff));
            }

            if (merge == 0 && diff > 0 && diffOther > 0)
            {
                freeAreas.push_back(Rect(chosen.X + iw, chosen.Y + ih, diffOther, diff));
            }
        }

        /*freeAreas.sort((a, b) = >
        {
            auto a1 = a.Value.Width * a.Value.Height;
            auto b1 = b.Value.Width * b.Value.Height;
            return Math.Sign(b1 - a1);
        });*/
    }

    return true;
}

static int SizesToTry[] = { 1024, 1536, 2048, 3072, 4096 };
static void RenderAndSave(vector<Entry>& entries, const wstring& outputFile)
{
    int wSize = 0;
    int hSize = 0;

    auto size = Size(SizesToTry[wSize], SizesToTry[hSize]);

    auto fn = PathGetFileNameWithoutExtension(outputFile);
    fn.append(L".png");

    auto bitmap = PathCombine(PathGetDirectory(outputFile), fn);

    bool measured = false;
    while (true)
    {
        Bitmap * target = new Bitmap(size.Width, size.Height, PixelFormat32bppARGB);
        Graphics* g = Graphics::FromImage(target);

        g->SetCompositingQuality(CompositingQualityAssumeLinear);
        g->SetInterpolationMode(InterpolationModeNearestNeighbor);
        g->SetPixelOffsetMode(PixelOffsetModeNone);
        g->SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);
        g->SetSmoothingMode(SmoothingModeNone);

        Font font(FontFamily::GenericSansSerif(), 9);

        if (!measured)
        {
            measured = true;
            for (auto& item : entries)
            {
                g->MeasureString(item.Label.c_str(), item.Label.size(), &font, RectF(), &item.LabelSize);
                g->MeasureString(item.LineNumber.c_str(), item.LineNumber.size(), &font, RectF(), &item.LineNumberSize);

                if (item.Vertical())
                {
                    item.LabelSize = RectF(0, 0, item.LabelSize.Height, item.LabelSize.Width);
                    item.W = (int)ceil(max(item.RealW() + item.LabelSize.Width, item.LineNumberSize.Width)) + 10;
                    item.H = (int)ceil(max(item.RealH() + item.LineNumberSize.Height, item.LabelSize.Height)) + 10;
                }
                else
                {
                    item.LineNumberSize = RectF(0, 0, item.LineNumberSize.Height, item.LineNumberSize.Width);
                    item.W = (int)ceil(max(item.RealW() + item.LineNumberSize.Width, item.LabelSize.Width)) + 10;
                    item.H = (int)ceil(max(item.RealH() + item.LabelSize.Height, item.LineNumberSize.Height)) + 10;
                }

            }
        }

        if (!ProcessLayout(entries, size))
        {
            if (size.Height > size.Width)
                wSize++;
            else
                hSize++;
            if (wSize >= _countof(SizesToTry) || hSize >= _countof(SizesToTry))
            {
                // FAIL!
                delete target;
                target = nullptr;
                return;
            }
            size = Size(SizesToTry[wSize], SizesToTry[hSize]);

            delete target;
            target = nullptr;
            continue;
        }

        SolidBrush blackBrush(Color(255, 0, 0, 0));
        for (auto& item : entries)
        {
            const wstring& label1 = item.Vertical() ? item.LineNumber : item.Label;
            g->DrawImage(item.Image, Rect(item.X, item.Y, item.RealW(), item.RealH()), 0, 0, item.RealW(), item.RealH(), UnitPixel);
            g->DrawString(label1.c_str(), label1.size(), &font, PointF(item.X, item.Y + item.RealH() + 1), &blackBrush);

            const RectF& size1 = item.Vertical() ? item.LabelSize : item.LineNumberSize;
            g->TranslateTransform(item.X + item.RealW() + size1.Width + 1, item.Y);
            g->RotateTransform(90);

            const wstring& label2 = item.Vertical() ? item.Label : item.LineNumber;
            g->DrawString(label2.c_str(), label2.size(), &font, PointF(), &blackBrush);
            g->ResetTransform();
        }

        g->Flush();
        delete g;

        CLSID pngClsid;
        GetEncoderClsid(L"image/png", &pngClsid);
        target->Save(bitmap.c_str(), &pngClsid);
        delete target;
        break;
    }

    auto output = wofstream(outputFile.c_str(), ios::out);

    output << L"#Bitmap=" << PathGetFileNameWithoutExtension(outputFile) << L".png" << endl;

    for (auto& entry : entries)
    {
        output << entry.FileName << L"=" << entry.X << L"," << entry.Y << L"," << entry.RealW() << L"," << entry.RealH() << L"," << entry.Image->GetPixelFormat() << endl;
    }

    output.close();
}

void Pack(const wstring& outputFile, const wstring& bitmapPath)
{
    auto entries = Scan(bitmapPath);
    RenderAndSave(entries, outputFile);
    for (auto& entry : entries)
    {
        delete entry.Image;
    }
}

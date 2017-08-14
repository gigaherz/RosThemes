using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;

namespace ThemePacker
{
    class Program
    {
        static void Main(string[] args)
        {
            string bitmapPath = null;
            string outputFile = ".";
            int values = 0;
            foreach (var arg in args)
            {
                if (arg.StartsWith("-") || arg.StartsWith("/"))
                {
                    // TODO
                    Console.WriteLine($"Unrecognized option: {arg}");
                    Console.WriteLine();
                    DisplayHelp();
                    return;
                }

                values++;
                switch (values)
                {
                    case 1:
                        bitmapPath = arg;
                        outputFile = bitmapPath + ".pack";
                        break;
                    case 2: outputFile = arg; break;
                }
            }

            if (values < 1)
            {
                DisplayHelp();
                return;
            }

            var entries = Scan(bitmapPath);
            RenderAndSave(entries, outputFile);
        }

        private static void DisplayHelp()
        {
            Console.WriteLine($"Command Line: {AppDomain.CurrentDomain.FriendlyName} <bitmap folder> [<pack file>] [<options>]");
            Console.WriteLine("Options:");
            Console.WriteLine("\tNothing here yet.");
        }

        public static List<Entry> Scan(string path)
        {
            var entries = new List<Entry>();

            var dir = new DirectoryInfo(path);
            foreach (var file in dir.EnumerateFiles())
            {
                if (file.Extension.ToLowerInvariant() != ".bmp")
                    continue;

                Bitmap src = new Bitmap(file.FullName);

                Debug.WriteLine($"{file.FullName}: {src.Width}x{src.Height} {src.PixelFormat}");

                var name = Path.GetFileNameWithoutExtension(file.Name);

                if (name.IndexOf('_') >= 0)
                    name = name.Substring(name.IndexOf('_') + 1);

                name = name.Replace("RADIOBUTTON", "[Rb]");
                name = name.Replace("CHECKBOX", "[Cb]");
                name = name.Replace("HORIZONTAL", "[H]");
                name = name.Replace("VERTICAL", "[V]");
                name = name.Replace("BUTTON", "[Btn]");
                name = name.Replace("EXPLORER", "[Xp]");
                name = name.Replace("TOOLBAR", "[Tb]");
                name = name.Replace("TASKBAR", "[Tkb]");
                name = name.Replace("START", "[S]");
                name = name.Replace("BACKGROUND", "[Bg]");

                entries.Add(new Entry
                {
                    FileName = file.Name,
                    Label = name,
                    Image = src
                });
            }

            entries.Sort((a, b) => string.CompareOrdinal(a.FileName, b.FileName));
            for (var i = 0; i < entries.Count; i++)
            {
                var item = entries[i];
                item.LineNumber = (i + 1).ToString();
            }

            return entries;
        }

        private static bool ProcessLayout(List<Entry> entries, Size size)
        {
            var freeAreas = new List<Rectangle?> { new Rectangle(5, 5, size.Width-10, size.Height-10) };

#if false
            var itemsSorting = new List<Entry>(entries);
            itemsSorting.Sort((a, b) =>
            {
                var r = Math.Sign(b.W * b.H - a.W * a.H);
                if (r != 0) return r;
                var w = Math.Sign(b.W - a.W);
                if (w != 0) return w;
                var h = Math.Sign(b.H - a.H);
                if (h != 0) return h;
                return 0;
            });
            var items = new Queue<Entry>(itemsSorting);
#else
            var items = new Queue<Entry>(entries);
#endif

            int processed = 0;
            while (items.Count > 0)
            {
                processed++;

                var item = items.Dequeue();

                var iw = item.W;
                var ih = item.H;

                Rectangle? best = null;
                var direction = 0;
                var diff = int.MaxValue;
                var diffOther = int.MaxValue;
                foreach (var rect in freeAreas)
                {
                    var w = rect.Value.Width;
                    var h = rect.Value.Height;

                    if (iw > w || ih > h)
                        continue;

                    var dw = w - iw;
                    var dh = h - ih;
                    if (dw < dh)
                    {
                        if (dw < diff || (Math.Abs(dw - diff) < float.Epsilon && dh < diffOther))
                        {
                            best = rect;
                            diff = dw;
                            diffOther = dh;
                            direction = 1;
                        }
                    }
                    else
                    {
                        if (dh < diff || (Math.Abs(dh - diff) < float.Epsilon && dw < diffOther))
                        {
                            best = rect;
                            diff = dh;
                            diffOther = dw;
                            direction = 2;
                        }
                    }
                }

                if (!best.HasValue)
                {
                    Console.WriteLine("ERROR: I wasn't able to find a rectangle to put this in!");
                    return false;
                }
                else
                {
                    var chosen = best.Value;
                    freeAreas.Remove(best);

                    item.X = chosen.X;
                    item.Y = chosen.Y;

                    if (direction == 1)
                    {
                        var t1 = diff * chosen.Height;
                        var t2 = chosen.Width * diffOther;

                        int merge = 0;
                        if (t1 > t2) merge = 1;
                        else merge = 2;

                        if (merge == 1)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y,diff, chosen.Height));
                        }
                        else if (diff > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y,diff, ih));
                        }

                        if (merge == 2)
                        {
                            freeAreas.Add(new Rectangle(chosen.X, chosen.Y + ih,chosen.Width, diffOther));
                        }
                        else if (diffOther > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X, chosen.Y + ih,iw, diffOther));
                        }

                        if (merge == 0 && diff > 0 && diffOther > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y + ih,diff, diffOther));
                        }
                    }
                    else
                    {
                        var t1 = diffOther * chosen.Height;
                        var t2 = chosen.Width * diff;

                        int merge = 0;
                        if (t1 > t2) merge = 1;
                        else merge = 2;

                        if (merge == 1)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y,diffOther, chosen.Height));
                        }
                        else if (diffOther > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y,diffOther, ih));
                        }
                        if (merge == 2)
                        {
                            freeAreas.Add(new Rectangle(chosen.X, chosen.Y + ih,chosen.Width, diff));
                        }
                        else if (diff > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X, chosen.Y + ih,iw, diff));
                        }
                        if (merge == 0 && diff > 0 && diffOther > 0)
                        {
                            freeAreas.Add(new Rectangle(chosen.X + iw, chosen.Y + ih,diffOther, diff));
                        }
                    }
                }

                freeAreas.Sort((a, b) =>
                {
                    var a1 = a.Value.Width * a.Value.Height;
                    var b1 = b.Value.Width * b.Value.Height;
                    return Math.Sign(b1 - a1);
                });
            }

            return true;
        }

        private static readonly int[] SizesToTry = { 1024, 1536, 2048, 3072, 4096 };
        private static void RenderAndSave(List<Entry> entries, string outputFile)
        {
            int wSize = 0;
            int hSize = 0;

            var size = new Size(SizesToTry[0], SizesToTry[0]);

            bool measured = false;
            Bitmap target;
            while (true)
            {
                target = new Bitmap(size.Width, size.Height, PixelFormat.Format32bppArgb);
                using (Graphics g = Graphics.FromImage(target))
                {
                    g.CompositingQuality = CompositingQuality.AssumeLinear;
                    g.InterpolationMode = InterpolationMode.NearestNeighbor;
                    g.PixelOffsetMode = PixelOffsetMode.None;
                    g.TextRenderingHint = TextRenderingHint.SingleBitPerPixelGridFit;
                    g.SmoothingMode = SmoothingMode.None;

                    using (var font = new Font(FontFamily.GenericSansSerif, 9))
                    {
                        if (!measured)
                        {
                            measured = true;
                            foreach (var item in entries)
                            {
                                item.LabelSize = g.MeasureString(item.Label, font);
                                item.LineNumberSize = g.MeasureString(item.LineNumber, font);

                                if (item.Vertical)
                                {
                                    item.LabelSize = new SizeF(item.LabelSize.Height, item.LabelSize.Width);
                                    item.W = (int)Math.Ceiling(Math.Max(item.RealW + item.LabelSize.Width, item.LineNumberSize.Width)) + 10;
                                    item.H = (int)Math.Ceiling(Math.Max(item.RealH + item.LineNumberSize.Height, item.LabelSize.Height)) + 10;
                                }
                                else
                                {
                                    item.LineNumberSize = new SizeF(item.LineNumberSize.Height, item.LineNumberSize.Width);
                                    item.W = (int)Math.Ceiling(Math.Max(item.RealW + item.LineNumberSize.Width, item.LabelSize.Width)) + 10;
                                    item.H = (int)Math.Ceiling(Math.Max(item.RealH + item.LabelSize.Height, item.LineNumberSize.Height)) + 10;
                                }

                            }
                        }

                        if (!ProcessLayout(entries, size))
                        {
                            if (size.Height > size.Width)
                                wSize++;
                            else
                                hSize++;
                            size = new Size(SizesToTry[wSize], SizesToTry[hSize]);

                            target.Dispose();
                            continue;
                        }

                        foreach (var item in entries)
                        {
                            g.DrawImage(item.Image, new Rectangle(item.X, item.Y, item.RealW, item.RealH), 0, 0, item.RealW, item.RealH, GraphicsUnit.Pixel);
                            g.DrawString(item.Vertical ? item.LineNumber : item.Label, font, Brushes.Black, item.X, item.Y + item.RealH + 1);

                            g.TranslateTransform(item.X + item.RealW + (item.Vertical?item.LabelSize:item.LineNumberSize).Width + 1, item.Y);
                            g.RotateTransform(90);
                            g.DrawString(item.Vertical ? item.Label : item.LineNumber, font, Brushes.Black, 0, 0);
                            g.ResetTransform();
                        }

                        break;
                    }
                }
            }

            var bitmap = Path.Combine(Path.GetDirectoryName(outputFile), Path.GetFileNameWithoutExtension(outputFile) + ".png");
            target.Save(bitmap);
            File.WriteAllLines(outputFile,  new[] {
                    $"#Bitmap={Path.GetFileNameWithoutExtension(outputFile) + ".png"}",
                }.Concat(entries.Select(b => $"{b.FileName}={b.X},{b.Y},{b.RealW},{b.RealH},{b.Image.PixelFormat}")));
        }
    }

    public class Entry
    {
        public string LineNumber { get; set; }
        public string FileName { get; set; }
        public string Label { get; set; }

        public Bitmap Image { get; set; }

        public int X { get; set; }
        public int Y { get; set; }
        public int W { get; set; }
        public int H { get; set; }

        public int RealW => Image.Width;
        public int RealH => Image.Height;

        public bool Vertical => RealH >= RealW;

        public SizeF LabelSize { get; set; }
        public SizeF LineNumberSize { get; set; }

        public override string ToString()
        {
            return $"({X},{Y},{W},{H})";
        }
    }
}

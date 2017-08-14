using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace ThemeUnpacker
{
    class Program
    {
        static void Main(string[] args)
        {
            string metadataFile = null;
            string outputFolder = ".";
            int values = 0;
            foreach(var arg in args)
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
                switch(values)
                {
                    case 1: metadataFile = arg; break;
                    case 2: outputFolder = arg; break;
                }
            }

            if (values < 1)
            {
                DisplayHelp();
                return;
            }

            Unpack(metadataFile, outputFolder);
        }

        private static void DisplayHelp()
        {
            Console.WriteLine($"Command Line: {AppDomain.CurrentDomain.FriendlyName} <pack file> [<output folder>] [<options>]" );
            Console.WriteLine("Options:");
            Console.WriteLine("\tNothing here yet.");
        }

        static void Unpack(string metadata, string output)
        { 
            var file = new FileInfo(metadata);
            var dir = new DirectoryInfo(output);

            if (!dir.Exists)
                dir.Create();

#if false
            string orig = null;
#endif
            Bitmap image = null;

            try
            {
                var lines = File.ReadAllLines(metadata);
                var regex = new Regex("^(?<filename>[^=]+)=(?<x>[0-9]+),(?<y>[0-9]+),(?<w>[0-9]+),(?<h>[0-9]+),(?<fmt>.*)$", RegexOptions.CultureInvariant);
                var meta = new Regex("^#(?<prop>[^=]+)=(?<meta>.*)$", RegexOptions.CultureInvariant);
                foreach (var ln in lines)
                {
                    string line = ln;
                    int commentChar = line.IndexOf('#');
                    if (commentChar == 0)
                    {
                        var metaMatch = meta.Match(line);
                        if (metaMatch.Success)
                        {
                            var metaValue = metaMatch.Groups["meta"].Value;
                            switch (metaMatch.Groups["prop"].Value)
                            {
#if false
                                case "SourceFolder":
                                    orig = metaValue;
                                    break;
#endif
                                case "Bitmap":
                                    if (image == null)
                                    {
                                        string path = metaValue;
                                        if (!Path.IsPathRooted(path))
                                            path = Path.Combine(file.DirectoryName ?? ".", metaValue);
                                        image = new Bitmap(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read));
                                    }
                                    break;
                            }
                        }
                        continue;
                    }

                    if (commentChar > 0)
                        line = line.Substring(0, commentChar);

                    line = line.Trim();

                    if (line.Length == 0)
                        continue;
                    
                    var match = regex.Match(line);
                    var fileName = match.Groups["filename"].Value;
                    if (!int.TryParse(match.Groups["x"].Value, out var x)) return;
                    if (!int.TryParse(match.Groups["y"].Value, out var y)) return;
                    if (!int.TryParse(match.Groups["w"].Value, out var w)) return;
                    if (!int.TryParse(match.Groups["h"].Value, out var h)) return;
                    if (!Enum.TryParse(match.Groups["fmt"].Value, true, out PixelFormat fmt)) return;

#if false
                    using (var bb = new Bitmap(new FileStream(Path.Combine(orig, fileName), FileMode.Open, FileAccess.Read, FileShare.Read)))
                        SaveCustomBmp(Path.Combine(output, fileName), b, new Rectangle(x, y, w, h), fmt, bb.Palette);
#else
                    SaveCustomBmp(Path.Combine(output, fileName), image, new Rectangle(x, y, w, h), fmt, null);
#endif
                }
            }
            finally
            {
                image?.Dispose();
            }
        }

        static int FindBestPixelFormat(Bitmap source, Rectangle rect, PixelFormat fmt, ColorPalette pal, out Color[] palette, out Color[,] pixels)
        {
            var colors = new HashSet<Color>(new ColorEquality());
            pixels = new Color[rect.Width, rect.Height];

            for (int y = 0; y < rect.Height; y++)
            {
                for (int x = 0; x < rect.Width; x++)
                {
                    var color = source.GetPixel(x + rect.Left, rect.Bottom - 1 - y);
                    colors.Add(color);
                    pixels[x, y] = color;
                }
            }

            if (colors.Count > 256 || fmt.HasFlag(PixelFormat.Alpha))
            {
                palette = null;
                return 32;
            }

            if (pal == null || colors.Count > pal.Entries.Length)
                palette = colors.ToArray();
            else
                palette = pal.Entries.ToArray();

            if (palette.Length > 16)
                return 8;

            if (palette.Length > 2)
                return 4;

            return 1;
        }

        static void SaveCustomBmp(string fileName, Bitmap source, Rectangle rect, PixelFormat fmt, ColorPalette pal)
        {
            var alpha = fmt.HasFlag(PixelFormat.Alpha);
            if (alpha)
                return;

            var bitsPerPixel = FindBestPixelFormat(source, rect, fmt, pal, out var palette, out var pixels);

            var bitmapData = new List<byte>();

            // File Header
            bitmapData.AddUShort(0x4D42);
            bitmapData.AddUInt(0); // placeholder for the bmp file size in bytes
            bitmapData.AddUShort(0); // reserved
            bitmapData.AddUShort(0); // reserved
            bitmapData.AddUInt(0); // placeholder for the pixel data start

            // Info Header
            bitmapData.AddUInt(40);
            bitmapData.AddInt(rect.Width);
            bitmapData.AddInt(rect.Height);
            bitmapData.AddUShort(1);
            bitmapData.AddUShort((ushort)bitsPerPixel);
            bitmapData.AddUInt(0);
            bitmapData.AddUInt(0); // can be 0 for uncompressed
            bitmapData.AddInt(0x0B12);
            bitmapData.AddInt(0x0B12);
            if (palette != null)
            {
                var palLength = palette.Length == (1 << bitsPerPixel) ? 0 : palette.Length;
                bitmapData.AddInt(palLength);
                bitmapData.AddInt(palLength);
                foreach (var color in palette)
                {
                    bitmapData.AddColor(color, alpha);
                }
            }
            else
            {
                bitmapData.AddInt(0);
                bitmapData.AddInt(0);
            }

            // Go back and set the bitmap data offset
            bitmapData.SetInt(10, bitmapData.Count);

            if (palette != null)
            {
                if (bitsPerPixel == 1)
                {
                    bool flush = (rect.Width & 3) != 0;
                    for (int y = 0; y < rect.Height; y++)
                    {
                        int c = 0;
                        int b = 0;
                        for (int x = 0; x < rect.Width; x++)
                        {
                            var color = pixels[x, y];
                            var index = Array.IndexOf(palette, color);
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
                                    bitmapData.Add((byte)c);
                                    b++;
                                    break;
                            }
                        }
                        if (flush)
                        {
                            bitmapData.Add((byte)c);
                            b++;
                        }
                        while ((b++ % 4) != 0)
                            bitmapData.Add(0);
                    }
                }
                else if (bitsPerPixel < 8)
                {
                    bool flush = (rect.Width & 1) != 0;
                    for (int y = 0; y < rect.Height; y++)
                    {
                        int c = 0;
                        int b = 0;
                        for (int x = 0; x < rect.Width; x++)
                        {
                            var color = pixels[x, y];
                            var index = Array.IndexOf(palette, color);
                            if ((x & 1) == 0)
                            {
                                c = index << 4;
                            }
                            else
                            {
                                c = c | index;
                                bitmapData.Add((byte)c);
                                b++;
                            }
                        }
                        if (flush)
                        {
                            bitmapData.Add((byte)c);
                            b++;
                        }
                        while ((b++ % 4) != 0)
                            bitmapData.Add(0);
                    }
                }
                else
                {
                    for (int y = 0; y < rect.Height; y++)
                    {
                        int x;
                        for (x = 0; x < rect.Width; x++)
                        {
                            var color = pixels[x, y];
                            var index = Array.IndexOf(palette, color);
                            bitmapData.Add((byte)index);
                        }
                        while ((x++ % 4) != 0)
                            bitmapData.Add(0);
                    }
                }
            }
            else // assumes 32bit
            {
                for (int y = 0; y < rect.Height; y++)
                {
                    for (int x = 0; x < rect.Width; x++)
                    {
                        var color = pixels[x, y];
                        bitmapData.AddColor(color, alpha);
                    }
                }
            }

            // Go back and set the bitmap data offset
            bitmapData.SetInt(2, bitmapData.Count);

            File.WriteAllBytes(fileName, bitmapData.ToArray());
        }
    }

    internal class ColorEquality : IEqualityComparer<Color>
    {
        public bool Equals(Color x, Color y)
        {
            return x.ToArgb() == y.ToArgb();
        }

        public int GetHashCode(Color obj)
        {
            return obj.ToArgb();
        }
    }

    internal static class ListExtensions
    {
        public static void AddUShort(this List<byte> list, ushort number)
        {
            list.Add((byte)number);
            list.Add((byte)(number >> 8));
        }

        public static void AddUInt(this List<byte> list, uint number)
        {
            list.Add((byte)number);
            list.Add((byte)(number >> 8));
            list.Add((byte)(number >> 16));
            list.Add((byte)(number >> 24));
        }

        public static void AddInt(this List<byte> list, int number)
        {
            AddUInt(list, (uint)number);
        }

        public static void AddColor(this List<byte> list, Color color, bool alpha)
        {
            list.Add(color.B);
            list.Add(color.G);
            list.Add(color.R);
            list.Add(alpha ? color.A : (byte)0);
        }

        public static void SetUInt(this List<byte> list, int index, uint number)
        {
            list[index + 0] = (byte)number;
            list[index + 1] = (byte)(number >> 8);
            list[index + 2] = (byte)(number >> 16);
            list[index + 3] = (byte)(number >> 24);
        }

        public static void SetInt(this List<byte> list, int index, int number)
        {
            SetUInt(list, index, (uint)number);
        }

    }
}

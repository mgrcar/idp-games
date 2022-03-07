using System;
using System.Linq;
using System.Collections.Generic;
using System.Drawing;
using System.IO;

namespace Ptn.Tools.PrepNyan
{
    class Program
    {
        static readonly Color black 
            = Color.FromArgb(255, 0, 0, 0);
        //static readonly Color green
        //    = Color.FromArgb(255, 0, 172, 0);
        static readonly Color darkGreen
            = Color.FromArgb(255, 0, 100, 0);
        static readonly Color white
            = Color.FromArgb(255, 255, 255, 255);

        static void ProcessBlock(Bitmap image, Bitmap imagePrev, int x, int y, Dictionary<int, dynamic> changes, Dictionary<int, dynamic> prev)
        {
            var colors = new HashSet<Color>();
            var colorsPrev = new HashSet<Color>();
            byte val = 0, valPrev = 0;
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    var pxPrev = imagePrev?.GetPixel(x + i, y + j) ?? black;
                    var px = image.GetPixel(x + i, y + j);
                    if (!colors.Contains(px)) { colors.Add(px); }
                    if (!colorsPrev.Contains(pxPrev)) { colorsPrev.Add(pxPrev); }
                    if (px != black)
                    {
                        // bit number is computed as 8 - (j * 2 + i)
                        val = (byte)(val | (1 << (5 - (j * 2 + i))));
                    }
                    if (pxPrev != black)
                    { 
                        valPrev = (byte)(valPrev | (1 << (5 - (j * 2 + i))));
                    }
                }
            }
            // check if there are only two colors and one of them is black
            if (!(colors.Count <= 2 && (colors.Contains(black) || colors.Count == 1)))
            {
                throw new Exception($"Block at {x},{y} fails the check.");
            }
            // generate code
            byte attr = 0, attrPrev = 0;
            if (colors.Any(x => x != black)) 
            {
                var color = colors.First(x => x != black);
                if (color == white) { attr = 0x40 >> 2; }
                //else if (color == green) { attr = 0x00; }
                else if (color == darkGreen) { attr = 0x80 >> 2; }
            }
            if (colorsPrev.Any(x => x != black))
            {
                var color = colorsPrev.First(x => x != black);
                if (color == white) { attrPrev = 0x40 >> 2; }
                //else if (color == green) { attrPrev = 0x00; }
                else if (color == darkGreen) { attrPrev = 0x80 >> 2; }
            }
            int row = y / 3;
            int col = x / 2;
            int addr = row * 100 + col;
            prev.Add(addr, new { addr, row, col, val = valPrev, attr = attrPrev });
            if (val != valPrev || attr != attrPrev) // if changed...
            {
                if (attr == 0x20) { val = (byte)(~val & 63); } // invert if dimmed
                changes.Add(addr, new { addr, row, col, val, attr = attr | 128 /* flag as dirty */ });
                if (changes.TryGetValue(addr & 0xFFF8, out dynamic item))
                {
                    changes[addr & 0xFFF8] = new { item.addr, item.row, item.col, item.val, attr = item.attr | 64 /* flag block as dirty */ };
                }
                else 
                {
                    var prevItem = prev[addr & 0xFFF8];
                    changes.Add(addr & 0xFFF8, new { prevItem.addr, prevItem.row, prevItem.col, prevItem.val, attr = prevItem.attr | 64 /* flag block as dirty */ }); 
                }
            }
        }

        static void Main(string[] args)
        {
            var fileNamePrev = @"..\..\..\..\..\res\Ptn.Tools.PrepNyan\nyanidp-3.png";
            var fileName = @"..\..\..\..\..\res\Ptn.Tools.PrepNyan\nyanidp-1.png";
            var changes = new Dictionary<int, dynamic>();
            var prev = new Dictionary<int, dynamic>();
            //Bitmap imagePrev = null;
            using (var imagePrev = new Bitmap(Image.FromFile(fileNamePrev)))
            using (var image = new Bitmap(Image.FromFile(fileName)))
            {
                Console.WriteLine($"Processing {Path.GetFullPath(fileName)}...");
                for (int y = 0; y < 120; y += 3)
                {
                    for (int x = 0; x < 200; x += 2)
                    {
                        ProcessBlock(image, imagePrev, x, y, changes, prev);
                    }
                }
            }
            foreach (var item in changes.Values.OrderBy(x => x.addr)) 
            {
                Console.Write($"{{ {item.row}, {item.addr & 255}, {item.addr >> 8}, {item.val}, {item.attr} }}, ");
            }
            Console.WriteLine(changes.Count);
        }
    }
}

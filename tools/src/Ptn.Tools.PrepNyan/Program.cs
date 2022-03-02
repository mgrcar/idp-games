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

        static int count
            = 0;

        static void ProcessBlock(Bitmap image, int x, int y)
        {
            var colors = new HashSet<Color>();
            byte val = 0;
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    var px = image.GetPixel(x + i, y + j);
                    if (!colors.Contains(px)) { colors.Add(px); }
                    if (px != black)
                    {
                        // bit number is computed as 8 - (j * 2 + i)
                        val = (byte)(val | (1 << (5 - (j * 2 + i))));
                    }
                }
            }
            // check if there are only two colors and one of them is black
            if (!(colors.Count <= 2 && (colors.Contains(black) || colors.Count == 1)))
            {
                throw new Exception($"Block at {x},{y} fails the check.");
            }
            // generate code
            byte attr = 0;
            if (colors.Any(x => x != black)) 
            {
                var color = colors.First(x => x != black);
                if (color == white) { attr = 0x40 >> 2; }
                //else if (color == green) { attr = 0x00; }
                else if (color == darkGreen) { attr = 0x80 >> 2; }
            }
            int row = y / 3;
            int col = x / 2;
            //if (val != 0)
            if (x >= 82 && x <= 118)
            {
                Console.Write($"{{ {row}, {col}, {val}, {attr} }}, ");
                count++;
            }
        }

        static void Main(string[] args)
        {
            var fileName = @"..\..\..\..\..\res\Ptn.Tools.PrepNyan\nyanidp-2.png";
            using (var image = new Bitmap(Image.FromFile(fileName)))
            {
                Console.WriteLine($"Processing {Path.GetFullPath(fileName)}...");
                for (int y = 0; y < 120; y += 3)
                {
                    for (int x = 0; x < 200; x += 2)
                    {
                        ProcessBlock(image, x, y);
                    }
                }
            }
            Console.WriteLine(count);
        }
    }
}

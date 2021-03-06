/*
  Entry point for bm-font-import
  Copyright (C) 2012  Morten Hustveit <morten.hustveit@gmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>
#include <locale.h>

#include "font.h"
#include "glyph.h"

static int fi_printVersion;
static int fi_printHelp;
static const char *fi_fontName = "DejaVu Sans";
static const char *fi_format = "binary";
static int fi_fontWeight = 200;
static int fi_fontSize = 13;

static struct option long_options[] =
{
  { "font" ,    required_argument, 0,                'f' },
  { "size" ,    required_argument, 0,                's' },
  { "weight" ,  required_argument, 0,                'w' },
  { "format",   required_argument, 0,                'F' },
  { "version",        no_argument, &fi_printVersion, 1 },
  { "help",           no_argument, &fi_printHelp,    1 },
  { 0, 0, 0, 0 }
};

static struct FONT_Data *fi_font;

static void
fi_LoadGlyph (wint_t character)
{
  struct FONT_Glyph *glyph;

  if (!(glyph = FONT_GlyphForCharacter (fi_font, character)))
    errx (EXIT_FAILURE, "Failed to get glyph for character %d", character);

  GLYPH_Add (character, glyph);

  free (glyph);
}

int
main (int argc, char **argv)
{
  int i;
  char *endptr;

  setlocale(LC_ALL, "en_US.UTF-8");

  while ((i = getopt_long (argc, argv, "f:s:w:", long_options, 0)) != -1)
    {
      switch (i)
        {
        case 'f':

          fi_fontName = optarg;

          break;

        case 's':

          fi_fontSize = strtol (optarg, &endptr, 0);

          if (*endptr)
            errx (EXIT_FAILURE, "Invalid size \"%s\".  Expected positive integer", optarg);

          if (fi_fontSize <= 0)
            errx (EXIT_FAILURE, "Invalid size %d.  Expected positive integer", fi_fontSize);

          break;

        case 'w':

          fi_fontWeight = strtol (optarg, &endptr, 0);

          if (*endptr)
            errx (EXIT_FAILURE, "Invalid weight \"%s\".  Expected positive integer", optarg);

          if (fi_fontWeight <= 0)
            errx (EXIT_FAILURE, "Invalid weight %d.  Expected positive integer", fi_fontWeight);

          break;

        case 'F':

          fi_format = optarg;

          break;

        case 0:

          break;

        case '?':

          fprintf (stderr, "Try `%s --help' for more information.\n", argv[0]);

          return EXIT_FAILURE;
        }
    }

  if (fi_printHelp)
    {
      printf ("Usage: %s [OPTION]...\n"
             "\n"
             "  -f, --font=FONT            set font name\n"
             "  -s, --size=SIZE            set font size\n"
             "  -w, --weight=WEIGHT        set font weight\n"
             "      --help     display this help and exit\n"
             "      --version  display version information\n"
             "\n"
             "Report bugs to <morten.hustveit@gmail.com>\n", argv[0]);

      return EXIT_SUCCESS;
    }

  if (fi_printVersion)
    {
      fprintf (stdout, "%s\n", PACKAGE_STRING);

      return EXIT_SUCCESS;
    }

  FONT_Init ();
  GLYPH_Init ();

  if (!(fi_font = FONT_Load (fi_fontName, fi_fontSize, fi_fontWeight)))
    errx (EXIT_FAILURE, "Failed to load font `%s' of size %u, weight %u", fi_fontName, fi_fontSize, fi_fontWeight);

  /* ASCII */
  for (i = ' '; i <= '~'; ++i)
    fi_LoadGlyph (i);

  /* ISO-8859-1 */
  for (i = 0xa1; i <= 0xff; ++i)
    fi_LoadGlyph (i);

  GLYPH_Export (fi_format, stdout);

  return EXIT_SUCCESS;
}

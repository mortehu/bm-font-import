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
static int fi_fontWeight = 200;
static int fi_fontSize = 13;

static struct option long_options[] =
{
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

  setlocale(LC_ALL, "en_US.UTF-8");

  while ((i = getopt_long (argc, argv, "", long_options, 0)) != -1)
    {
      switch (i)
        {
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

  return EXIT_SUCCESS;
}

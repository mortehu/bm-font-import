#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define FR_MAX_GLYPHS 256

struct fr_GlyphInfo
{
  int16_t ch;
  int16_t xOffset;
  int16_t width;
  int16_t height;
  int16_t x;
  int16_t y;
  int16_t u;
  int16_t v;
};

struct fr_Font
{
  int atlasSize;
  uint8_t *bitmap;

  struct fr_GlyphInfo glyphs[FR_MAX_GLYPHS];
  size_t glyphCount;
};

static int16_t
fr_ReadS16 (FILE *input)
{
  uint16_t result;

  result = fgetc (input);
  result |= fgetc (input) << 8;

  return (int16_t) result;
}

static void
fr_LoadFont (struct fr_Font *font, FILE *input)
{
  memset (font, 0, sizeof (*font));

  font->atlasSize = fr_ReadS16 (input);

  font->bitmap = calloc (4, font->atlasSize * font->atlasSize);

  fread (font->bitmap, 4, font->atlasSize * font->atlasSize, input);

  while (!feof (input) && font->glyphCount < FR_MAX_GLYPHS)
    {
      struct fr_GlyphInfo glyph;

      glyph.ch =      fr_ReadS16 (input);
      glyph.xOffset = fr_ReadS16 (input);
      glyph.width =   fr_ReadS16 (input);
      glyph.height =  fr_ReadS16 (input);
      glyph.x =       fr_ReadS16 (input);
      glyph.y =       fr_ReadS16 (input);
      glyph.u =       fr_ReadS16 (input);
      glyph.v =       fr_ReadS16 (input);

      font->glyphs[font->glyphCount++] = glyph;
    }
}

static struct fr_GlyphInfo *
fr_FindGlyph (struct fr_Font *font, wint_t ch)
{
  size_t first = 0, half, middle, count;

  count = font->glyphCount;

  while (count > 0)
    {
      half = count / 2;
      middle = first + half;

      if (font->glyphs[middle].ch == ch)
        return &font->glyphs[middle];

      if (font->glyphs[middle].ch < ch)
        {
          first = middle + 1;
          count -= half + 1;
        }
      else
        count = half;
    }

  return NULL;
}

static void
fr_PutRGB (uint8_t *rgb)
{
  static const char intensity[] = "%@&#$=+<;:-. ";
  static const struct
    {
      uint8_t r, g, b;
      const char *str;
    }
  palette[] =
    {
        { 0x00, 0x00, 0x00, "\033[23;30m" },
        { 0x18, 0x18, 0xc2, "\033[23;34m" },
        { 0x18, 0xc2, 0x18, "\033[23;32m" },
        { 0x18, 0xc2, 0xc2, "\033[23;36m" },
        { 0xc2, 0x18, 0x18, "\033[23;31m" },
        { 0xc2, 0x18, 0xc2, "\033[23;35m" },
        { 0xc2, 0xc2, 0x18, "\033[23;33m" },
        { 0xc2, 0xc2, 0xc2, "\033[23;37m" },
        { 0x68, 0x68, 0x68, "\033[3;30m" },
        { 0x74, 0x74, 0xff, "\033[3;34m" },
        { 0x54, 0xff, 0x54, "\033[3;32m" },
        { 0x54, 0xff, 0xff, "\033[3;36m" },
        { 0xff, 0x54, 0x54, "\033[3;31m" },
        { 0xff, 0x54, 0xff, "\033[3;35m" },
        { 0xff, 0xff, 0x54, "\033[3;33m" },
        { 0xff, 0xff, 0xff, "\033[3;37m" }
    };

  uint8_t r, g, b;
  unsigned int nearest = 0, nearestScore = 768, i;

  r = rgb[0];
  g = rgb[1];
  b = rgb[2];

  for (i = 0; i < sizeof (palette) / sizeof (palette[0]); ++i)
    {
      unsigned int score, dr, dg, db;

      if (r > palette[i].r || g > palette[i].g || b > palette[i].b)
        continue;

      dr = (palette[i].r - r);
      dg = (palette[i].g - g);
      db = (palette[i].b - b);

      score = (dr > dg && dr > db) ? dr
            : (dg > db) ? dg
            : db;

      if (score < nearestScore)
        {
          nearest = i;
          nearestScore = score;
        }
    }

  printf ("%s%c", palette[nearest].str, intensity[nearestScore * (sizeof (intensity) - 1) / 256]);
}

static void
fr_RenderString (struct fr_Font *font, const char *string)
{
  const char *ch;
  unsigned char *target;
  unsigned int x, y, width, height;
  int left = 0, right = 0, top = 0, bottom = 0;

  for (x = 0, ch = string; *ch; ++ch)
    {
      struct fr_GlyphInfo *glyph;

      if (!(glyph = fr_FindGlyph (font, *ch)))
        continue;

      if (x - glyph->x < left)
        left = glyph->x;

      if (x + glyph->width - glyph->x > right)
        right = x + glyph->width - glyph->x;

      if (-glyph->y < top)
        top = -glyph->y;

      if (glyph->height - glyph->y > bottom)
        bottom = glyph->height - glyph->y;

      x += glyph->xOffset;
    }

  width = right - left;
  height = bottom - top;

  target = calloc (4, width * height);

  for (x = 0, ch = string; *ch; ++ch)
    {
      struct fr_GlyphInfo *glyph;
      unsigned int row;

      if (!(glyph = fr_FindGlyph (font, *ch)))
        continue;

      y = -glyph->y - top;

      for (row = 0; row < glyph->height; ++row, ++y)
        {
          memcpy (target + (y * width + x - glyph->x) * 4,
                  font->bitmap + ((glyph->v + row) * font->atlasSize + glyph->u) * 4,
                  glyph->width * 4);
        }

      x += glyph->xOffset;
    }

  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        fr_PutRGB (&target[(y * width + x) * 4]);

      putchar ('\n');
    }
}

int
main (int argc, char **argv)
{
  struct fr_Font font;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <STRING>\n", argv[0]);

      return EXIT_FAILURE;
    }

  fr_LoadFont (&font, stdin);

  fr_RenderString (&font, argv[1]);

  return EXIT_SUCCESS;
}

/*
  Texture atlas generator
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
#include <string.h>

#include <err.h>

#include "glyph.h"

struct glyph_Data
{
  uint16_t width, height;
  int16_t  x, y;
  int16_t  xOffset, yOffset;
  int16_t  u, v;
};

static uint32_t *bitmap;
static struct glyph_Data glyphs[65536]; /* 1 MB */
static uint32_t loadedGlyphs[65536 / 32];
static unsigned int top[GLYPH_ATLAS_SIZE];
static int glyph_dirty;

void
GLYPH_Init (void)
{
  bitmap = calloc (sizeof (*bitmap), GLYPH_ATLAS_SIZE * GLYPH_ATLAS_SIZE);
}

void
GLYPH_Add (unsigned int code, struct FONT_Glyph *glyph)
{
  if (code >= sizeof (glyphs) / sizeof (glyphs[0]))
    return;

  loadedGlyphs[code >> 5] |= (1 << (code & 31));

  if (glyph->width && glyph->height)
    {
      unsigned int best_u, best_v, u, k, v_max;

      best_u = GLYPH_ATLAS_SIZE;
      best_v = GLYPH_ATLAS_SIZE;

      for(u = 0; u < GLYPH_ATLAS_SIZE - glyph->width + 1; ++u)
        {
          v_max = top[u];

          for(k = 1; k < glyph->width && v_max < best_v; ++k)
            {
              if (top[u + k] > v_max)
                v_max = top[u + k];
            }

          if (v_max < best_v)
            {
              best_v = v_max;
              best_u = u;
            }
        }

      if (best_u == GLYPH_ATLAS_SIZE || best_v + glyph->height > GLYPH_ATLAS_SIZE)
        {
          errx (EXIT_FAILURE, "Atlas is full: No room for glyph of size %ux%u", glyph->width, glyph->height);

          return;
        }

      glyphs[code].u = best_u;
      glyphs[code].v = best_v;

      for (k = 0; k < glyph->height; ++k)
        {
          memcpy (bitmap + (best_v + k)* GLYPH_ATLAS_SIZE + best_u,
                  glyph->data + (k * glyph->width) * 4,
                  glyph->width * 4);
        }

      for (k = 0; k < glyph->width; ++k)
        top[best_u + k] = best_v + glyph->height;
    }

  glyphs[code].width = glyph->width;
  glyphs[code].height = glyph->height;
  glyphs[code].x = glyph->x;
  glyphs[code].y = glyph->y;
  glyphs[code].xOffset = glyph->xOffset;
  glyphs[code].yOffset = glyph->yOffset;

  glyph_dirty = 1;
}

int
GLYPH_IsLoaded (unsigned int code)
{
  if (code >= sizeof (glyphs) / sizeof (glyphs[0]))
    return 1;

  return (loadedGlyphs[code >> 5] & (1 << (code & 31)));
}

void
GLYPH_Get (unsigned int code, struct FONT_Glyph *glyph,
           uint16_t *u, uint16_t *v)
{
  if (code >= sizeof (glyphs) / sizeof (glyphs[0]))
    {
      memset (glyph, 0, sizeof (*glyph));
      *u = 0.0f;
      *v = 0.0f;

      return;
    }

  glyph->width = glyphs[code].width;
  glyph->height = glyphs[code].height;
  glyph->x = glyphs[code].x;
  glyph->y = glyphs[code].y;
  glyph->xOffset = glyphs[code].xOffset;
  glyph->yOffset = glyphs[code].yOffset;

  *u = glyphs[code].u;
  *v = glyphs[code].v;
}

static void
glyph_WriteS16 (FILE *output, int v)
{
  fputc (v & 0xff, output);
  fputc ((v & 0xff00) >> 8, output);
}

void
GLYPH_Export (const char* format, FILE *output)
{
  size_t i;

  if (!strcmp(format, "binary"))
    {
      glyph_WriteS16 (output, GLYPH_ATLAS_SIZE);

      fwrite (bitmap, sizeof (*bitmap), GLYPH_ATLAS_SIZE * GLYPH_ATLAS_SIZE, output);

      for (i = 0; i < sizeof (glyphs) / sizeof (glyphs[0]); ++i)
        {
          if (!(loadedGlyphs[i >> 5] & (1 << (i & 31))))
            continue;

          if (glyphs[i].width <= 0 || glyphs[i].height <= 0)
            continue;

          glyph_WriteS16 (output, i);
          glyph_WriteS16 (output, glyphs[i].xOffset);
          glyph_WriteS16 (output, glyphs[i].width);
          glyph_WriteS16 (output, glyphs[i].height);
          glyph_WriteS16 (output, glyphs[i].x);
          glyph_WriteS16 (output, glyphs[i].y);
          glyph_WriteS16 (output, glyphs[i].u);
          glyph_WriteS16 (output, glyphs[i].v);
        }
    }
  else if (!strcmp(format, "c"))
    {
      fprintf (output, "struct Glyph glyphs[256] = {\n");

      for (i = 0; i < 256; ++i)
        {
          if (!(loadedGlyphs[i >> 5] & (1 << (i & 31)))
              || glyphs[i].width <= 0 || glyphs[i].height <= 0)
            {
              fprintf (output, "  { 0, 0, 0, 0, 0, 0, 0 },\n");
              continue;
            }

          fprintf (output, "  { %d, %d, %d, %d, %d, %d, %d },\n",
                   glyphs[i].xOffset, glyphs[i].width, glyphs[i].height,
                   glyphs[i].x, glyphs[i].y, glyphs[i].u, glyphs[i].v);
        }
      fprintf (output, "};\n\n");
      fprintf (output, "const unsigned char bitmap[] = {");

      for (i = 0; i < GLYPH_ATLAS_SIZE * GLYPH_ATLAS_SIZE; ++i)
        {
          if (!(i % 4))
            fprintf (output, "\n ");
          fprintf(output, " 0x%02x, 0x%02x, 0x%02x, 0x%02x,",
                  (bitmap[i] >> 24) & 0xff, (bitmap[i] >> 16) & 0xff,
                  (bitmap[i] >> 8) & 0xff, bitmap[i] & 0xff);
        }

      fprintf(output, "\n};\n");
    }
  else
    {
      errx(EXIT_FAILURE, "Unknown output format '%s'", format);
    }
}

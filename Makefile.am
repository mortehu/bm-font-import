bin_PROGRAMS = bm-font-import
noinst_PROGRAMS = bm-font-render

AM_CFLAGS = -g -Wall -std=c99 $(PACKAGES_CFLAGS)

bm_font_import_SOURCES = font-import.c glyph.h font.h font.c glyph.c 
bm_font_import_LDFLAGS = $(PACKAGES_LIBS)

bm_font_render_SOURCES = font-render.c

/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include "gfxutil.h"
#include "colors.h"
#include "cave.h"
#include "cavedb.h"
#include "caveset.h"
#include "caveobject.h"
#include "gtkgfx.h"
#include "c64_gfx.h"    /* char c64_gfx[] with (almost) original graphics */
#include "settings.h"
#include "util.h"

#include "c64_png_colors.h"

static GdkPixbuf *cells_pb[NUM_OF_CELLS];
static GdkPixbuf *combo_pb[NUM_OF_CELLS];

static GdkPixmap *cells_game[NUM_OF_CELLS*3], *cells_editor[NUM_OF_CELLS*3];
int gd_cell_size_game, gd_cell_size_editor;

GdkPixbuf *gd_pixbuf_for_builtin_theme;    /* this stores a player image, which is the pixbuf for the settings window */




static GdColor color0, color1, color2, color3, color4, color5;    /* currently used cell colors */
static guint8 *c64_custom_gfx=NULL;
static gboolean using_png_gfx;

/* to be called at application start. creates a pixbuf,
 * which represents the builtin theme in the preferences window.
 */
void gd_create_pixbuf_for_builtin_theme()
{
    /* use gdash palette */
    gd_select_pixbuf_colors(GD_GDASH_BLACK, GD_GDASH_MIDDLEBLUE, GD_GDASH_LIGHTRED, GD_GDASH_WHITE, GD_GDASH_WHITE, GD_GDASH_WHITE);
    gd_pixbuf_for_builtin_theme=gdk_pixbuf_copy(cells_pb[ABS(gd_elements[O_PLAYER].image_game)]);
}


/* used by the editor for the element pick dialog */
GdColor
gd_current_background_color()
{
    if (color0!=GD_COLOR_INVALID)
        return color0;
    return GD_GDASH_BLACK;
}


/* wrapper around scale2x defined in gfxutil.c */
static GdkPixbuf *
scale2x(GdkPixbuf *src)
{
       GdkPixbuf *dst;

       g_assert(gdk_pixbuf_get_colorspace(src)==GDK_COLORSPACE_RGB);
       g_assert(gdk_pixbuf_get_has_alpha(src));
       g_assert(gdk_pixbuf_get_n_channels(src)==4);
       g_assert(gdk_pixbuf_get_bits_per_sample(src)==8);

    dst=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 2*gdk_pixbuf_get_width(src), 2*gdk_pixbuf_get_height(src));

    gd_scale2x_raw(gdk_pixbuf_get_pixels(src), gdk_pixbuf_get_width(src), gdk_pixbuf_get_height(src), gdk_pixbuf_get_rowstride(src), gdk_pixbuf_get_pixels(dst), gdk_pixbuf_get_rowstride(dst));

    return dst;
}

/* wrapper around scale3x defined in gfxutil.c */
static GdkPixbuf *
scale3x(GdkPixbuf *src)
{
       GdkPixbuf *dst;

       g_assert(gdk_pixbuf_get_colorspace(src)==GDK_COLORSPACE_RGB);
       g_assert(gdk_pixbuf_get_has_alpha(src));
       g_assert(gdk_pixbuf_get_n_channels(src)==4);
       g_assert(gdk_pixbuf_get_bits_per_sample(src)==8);

    dst=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 3*gdk_pixbuf_get_width(src), 3*gdk_pixbuf_get_height(src));

    gd_scale3x_raw(gdk_pixbuf_get_pixels(src), gdk_pixbuf_get_width(src), gdk_pixbuf_get_height(src), gdk_pixbuf_get_rowstride(src), gdk_pixbuf_get_pixels(dst), gdk_pixbuf_get_rowstride(dst));

    return dst;
}

/* scale4x is essentially a scale2x applied twice */
static GdkPixbuf *
scale4x(GdkPixbuf *src)
{
       GdkPixbuf *dst2x, *dst;

       g_assert(gdk_pixbuf_get_colorspace(src)==GDK_COLORSPACE_RGB);
       g_assert(gdk_pixbuf_get_has_alpha(src));
       g_assert(gdk_pixbuf_get_n_channels(src)==4);
       g_assert(gdk_pixbuf_get_bits_per_sample(src)==8);

    dst2x=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 2*gdk_pixbuf_get_width(src), 2*gdk_pixbuf_get_height(src));
    dst=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 4*gdk_pixbuf_get_width(src), 4*gdk_pixbuf_get_height(src));

    gd_scale2x_raw(gdk_pixbuf_get_pixels(src), gdk_pixbuf_get_width(src), gdk_pixbuf_get_height(src), gdk_pixbuf_get_rowstride(src), gdk_pixbuf_get_pixels(dst2x), gdk_pixbuf_get_rowstride(dst2x));

    gd_scale2x_raw(gdk_pixbuf_get_pixels(dst2x), gdk_pixbuf_get_width(dst2x), gdk_pixbuf_get_height(dst2x), gdk_pixbuf_get_rowstride(dst2x), gdk_pixbuf_get_pixels(dst), gdk_pixbuf_get_rowstride(dst));
    
    g_object_unref(dst2x);

    return dst;
}




/* scales a pixbuf with the appropriate scaling type. */
GdkPixbuf *
gd_pixbuf_scale(GdkPixbuf *orig, GdScalingType type)
{
    GdkPixbuf *pixbuf=NULL;

    switch (type) {
        case GD_SCALING_ORIGINAL:
            pixbuf=gdk_pixbuf_copy(orig);
            break;

        case GD_SCALING_2X:
            pixbuf=gdk_pixbuf_scale_simple(orig, 2*gdk_pixbuf_get_width(orig), 2*gdk_pixbuf_get_height(orig), GDK_INTERP_NEAREST);
            break;

        case GD_SCALING_2X_BILINEAR:
            pixbuf=gdk_pixbuf_scale_simple(orig, 2*gdk_pixbuf_get_width(orig), 2*gdk_pixbuf_get_height(orig), GDK_INTERP_BILINEAR);
            break;

        case GD_SCALING_2X_SCALE2X:
            pixbuf=scale2x(orig);
            break;

        case GD_SCALING_3X:
            pixbuf=gdk_pixbuf_scale_simple(orig, 3*gdk_pixbuf_get_width(orig), 3*gdk_pixbuf_get_height(orig), GDK_INTERP_NEAREST);
            break;

        case GD_SCALING_3X_BILINEAR:
            pixbuf=gdk_pixbuf_scale_simple(orig, 3*gdk_pixbuf_get_width(orig), 3*gdk_pixbuf_get_height(orig), GDK_INTERP_BILINEAR);
            break;

        case GD_SCALING_3X_SCALE3X:
            pixbuf=scale3x(orig);
            break;

        case GD_SCALING_4X:
            pixbuf=gdk_pixbuf_scale_simple(orig, 4*gdk_pixbuf_get_width(orig), 4*gdk_pixbuf_get_height(orig), GDK_INTERP_NEAREST);
            break;

        case GD_SCALING_4X_BILINEAR:
            pixbuf=gdk_pixbuf_scale_simple(orig, 4*gdk_pixbuf_get_width(orig), 4*gdk_pixbuf_get_height(orig), GDK_INTERP_BILINEAR);
            break;

        case GD_SCALING_4X_SCALE4X:
            pixbuf=scale4x(orig);
            break;

        case GD_SCALING_MAX:
            /* to avoid compiler warning */
            g_assert_not_reached();
            break;
    }

    return pixbuf;
}

/*
    draw an element - usually an arrow or something like that
    over another one.

    the destination element's editor drawing will be used.
    the source element will be a game element.
*/
static void
add_arrow_to_cell(GdElement dest, GdElement src, GdElement arrow, GdkPixbufRotation rotation)
{
    int pixbuf_cell_size=gdk_pixbuf_get_height(cells_pb[0]);
    GdkPixbuf *arrow_pb=gdk_pixbuf_rotate_simple(cells_pb[gd_elements[arrow].image], rotation);    /* arrow */

    if (gd_elements[dest].image<NUM_OF_CELLS_X*NUM_OF_CELLS_Y) {
        g_critical("destination index %d<NUM_OF_CELLS_X*NUM_OF_CELLS_Y, element %s", dest, gd_elements[dest].name);
        g_assert_not_reached();
    }

    if (gd_elements[dest].image>=NUM_OF_CELLS) {
        g_critical("destination index %d>=NUM_OF_CELLS, element %s", dest, gd_elements[dest].name);
        g_assert_not_reached();
    }
    if (cells_pb[gd_elements[dest].image]!=NULL) {
        g_critical("destination index %d!=NULL, element %s", dest, gd_elements[dest].name);
        g_assert_not_reached();
    }

    /* editor image <- game image */
    cells_pb[gd_elements[dest].image]=gdk_pixbuf_copy(cells_pb[ABS(gd_elements[src].image_game)]);
    /* composite arrow to copy */
    gdk_pixbuf_composite (arrow_pb, cells_pb[gd_elements[dest].image], 0, 0, pixbuf_cell_size, pixbuf_cell_size, 0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
    g_object_unref (arrow_pb);
}

static void
copy_cell(int dest, int src)
{
    g_assert(cells_pb[dest]==NULL);
    g_assert(src<NUM_OF_CELLS);
    cells_pb[dest]=gdk_pixbuf_copy(cells_pb[src]);
}

/*
    composite two elements.
*/
static void
create_composite_cell_pixbuf(GdElement dest, GdElement src1, GdElement src2)
{
    int pixbuf_cell_size=gdk_pixbuf_get_height (cells_pb[0]);

    g_assert(gd_elements[dest].image<NUM_OF_CELLS);
    g_assert(cells_pb[gd_elements[dest].image]==NULL);

    /* destination image=source1 */
    cells_pb[gd_elements[dest].image]=gdk_pixbuf_copy(cells_pb[gd_elements[src1].image]);
    /* composite source2 to destination */
    gdk_pixbuf_composite(cells_pb[gd_elements[src2].image], cells_pb[gd_elements[dest].image], 0, 0, pixbuf_cell_size, pixbuf_cell_size, 0, 0, 1, 1, GDK_INTERP_NEAREST, 85);
}

/* check if pixbuf is suitable for a theme.
   report_error=TRUE, then it will send warning messages if it isn't.
 */
const char *
gd_is_pixbuf_ok_for_theme(GdkPixbuf *pixbuf)
{
    static char *error=NULL;
    int width, height;

    g_assert(pixbuf!=NULL);

    g_free(error);
    error=NULL;

    width=gdk_pixbuf_get_width(pixbuf);
    height=gdk_pixbuf_get_height(pixbuf);

    if ((width % NUM_OF_CELLS_X != 0) || (height % NUM_OF_CELLS_Y != 0) || (width / NUM_OF_CELLS_X != height / NUM_OF_CELLS_Y)) {
        error=g_strdup_printf("Image should contain %d cells in a row and %d in a column!", NUM_OF_CELLS_X, NUM_OF_CELLS_Y);
        return error;
    }
    if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
        error=g_strdup_printf("Image should have an alpha channel!");
        return error;
    }

    /* passes tests */
    return NULL;
}

/* check if file is suitable for a theme.
   report_error=TRUE, then it will send warning messages if it isn't.
 */
const char *
gd_is_image_ok_for_theme(const char *filename)
{
    /* load from file */
    GdkPixbuf *pixbuf;
    GError *error=NULL;
    static char *error_msg=NULL;
    const char *error_from_pixbuf;

    g_assert(filename!=NULL);

    g_free(error_msg);
    error_msg=NULL;

    /* open file */
    pixbuf=gdk_pixbuf_new_from_file (filename, &error);
    if (error) {
        error_msg=g_strdup(error->message);
        g_error_free(error);
        return error_msg;
    }
    error_from_pixbuf=gd_is_pixbuf_ok_for_theme(pixbuf);
    g_object_unref(pixbuf);

    if (error_from_pixbuf) {
        error_msg=g_strdup(error_from_pixbuf);
        return error_msg;
    }

    return NULL;    /* passed tests */
}

/* remove pixmaps from x server */
static void
free_pixmaps()
{
    int i;

    /* if cells already loaded, unref them */
    for (i=0; i<G_N_ELEMENTS(cells_game); i++) {
        if (cells_game[i])
            g_object_unref(cells_game[i]);
        cells_game[i]=NULL;
    }
    /* if cells already loaded, unref them */
    for (i=0; i<G_N_ELEMENTS(cells_editor); i++) {
        if (cells_editor[i])
            g_object_unref(cells_editor[i]);
        cells_editor[i]=NULL;
    }
}

/* returns true, if the given pixbuf seems to be a c64 imported image. */
static gboolean
check_if_pixbuf_c64_png (GdkPixbuf *pixbuf)
{
    int width, height, rowstride, n_channels;
    guchar *pixels, *p;
    int x, y;

    n_channels=gdk_pixbuf_get_n_channels (pixbuf);

    g_assert(gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
    g_assert(gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
    g_assert(gdk_pixbuf_get_has_alpha (pixbuf));
    g_assert(n_channels == 4);

    width=gdk_pixbuf_get_width (pixbuf);
    height=gdk_pixbuf_get_height (pixbuf);

    rowstride=gdk_pixbuf_get_rowstride (pixbuf);
    pixels=gdk_pixbuf_get_pixels (pixbuf);

    for (y=0; y<height; y++) {
        p=pixels + y * rowstride;
        for (x=0; x<width*n_channels; x++)
            if (p[x]!=0 && p[x]!=255)
                return FALSE;
    }
    return TRUE;
}

static void
check_pixbuf(int i, const char *name)
{
    if (i>=0) {
        if (cells_pb[i]==NULL)
            g_critical("no pixbuf for %s", name);
    } else {
        int x;
        
        for (x=0; x<8; x++)
            if (cells_pb[ABS(i)+x]==NULL)
                g_critical("no pixbuf for %s", name);
    }
}


/* load cells, eg. create cells_pb and combo_pb
   from a big pixbuf.
*/
static void
loadcells_from_pixbuf(GdkPixbuf *cells_pixbuf)
{
    static gboolean checked_cells=FALSE;
    int i;
    int pixbuf_cell_size;

    /* now that we have the pixbuf, we can start freeing old graphics. */
    for (i=0; i<G_N_ELEMENTS(cells_pb); i++) {
        if (cells_pb[i]) {
            g_object_unref (cells_pb[i]);
            cells_pb[i]=NULL;
        }
        /* scaled cells for editor combo boxes. created by editor, but we free them if we load a new theme */
        if (combo_pb[i]) {
            g_object_unref (combo_pb[i]);
            combo_pb[i]=NULL;
        }
    }
    /* if we have scaled pixmaps, remove them */
    free_pixmaps();

    /* 8 (NUM_OF_CELLS_X) cells in a row, so divide by it and we get the size of a cell in pixels */
    pixbuf_cell_size=gdk_pixbuf_get_width(cells_pixbuf) / NUM_OF_CELLS_X;

    /* make individual cell pixbufs */
    for (i=0; i<NUM_OF_CELLS_Y*NUM_OF_CELLS_X; i++)
        /* copy one cell */
        cells_pb[i]=gdk_pixbuf_new_subpixbuf(cells_pixbuf, (i%NUM_OF_CELLS_X) * pixbuf_cell_size, (i/NUM_OF_CELLS_X) * pixbuf_cell_size, pixbuf_cell_size, pixbuf_cell_size);

    /* set cell sizes */
    gd_cell_size_game=pixbuf_cell_size*gd_scaling_scale[gd_cell_scale_game];
    gd_cell_size_editor=pixbuf_cell_size*gd_scaling_scale[gd_cell_scale_editor];

    /* draw some elements, combining them with arrows and the like */
    add_arrow_to_cell(O_STEEL_EATABLE, O_STEEL, O_EATABLE, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_BRICK_EATABLE, O_BRICK, O_EATABLE, GDK_PIXBUF_ROTATE_NONE);
    create_composite_cell_pixbuf(O_BRICK_NON_SLOPED, O_STEEL, O_BRICK);

    create_composite_cell_pixbuf(O_WALLED_KEY_1, O_KEY_1, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_KEY_2, O_KEY_2, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_KEY_3, O_KEY_3, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_DIAMOND, O_DIAMOND, O_BRICK);

    add_arrow_to_cell(O_FIREFLY_1, O_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_FIREFLY_2, O_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_FIREFLY_3, O_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_FIREFLY_4, O_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_ALT_FIREFLY_1, O_ALT_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_ALT_FIREFLY_2, O_ALT_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_ALT_FIREFLY_3, O_ALT_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_ALT_FIREFLY_4, O_ALT_FIREFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_H_EXPANDING_WALL, O_H_EXPANDING_WALL, O_LEFTRIGHT_ARROW, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_V_EXPANDING_WALL, O_V_EXPANDING_WALL, O_LEFTRIGHT_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_EXPANDING_WALL, O_EXPANDING_WALL, O_EVERYDIR_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_H_EXPANDING_STEEL_WALL, O_H_EXPANDING_STEEL_WALL, O_LEFTRIGHT_ARROW, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_V_EXPANDING_STEEL_WALL, O_V_EXPANDING_STEEL_WALL, O_LEFTRIGHT_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_EXPANDING_STEEL_WALL, O_EXPANDING_STEEL_WALL, O_EVERYDIR_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_BUTTER_1, O_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_BUTTER_2, O_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_BUTTER_3, O_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_BUTTER_4, O_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_DRAGONFLY_1, O_DRAGONFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_DRAGONFLY_2, O_DRAGONFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_DRAGONFLY_3, O_DRAGONFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_DRAGONFLY_4, O_DRAGONFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_COW_1, O_COW_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_COW_2, O_COW_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_COW_3, O_COW_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_COW_4, O_COW_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_COW_ENCLOSED_1, O_COW_1, O_GLUED, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_ALT_BUTTER_1, O_ALT_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_ALT_BUTTER_2, O_ALT_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_ALT_BUTTER_3, O_ALT_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_ALT_BUTTER_4, O_ALT_BUTTER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_PLAYER_GLUED, O_PLAYER, O_GLUED, 0);
    add_arrow_to_cell(O_PLAYER, O_PLAYER, O_EXCLAMATION_MARK, 0);
    add_arrow_to_cell(O_STONE_GLUED, O_STONE, O_GLUED, 0);
    add_arrow_to_cell(O_DIAMOND_GLUED, O_DIAMOND, O_GLUED, 0);
    add_arrow_to_cell(O_DIRT_GLUED, O_DIRT, O_GLUED, 0);
    add_arrow_to_cell(O_STONE_F, O_STONE, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_FLYING_STONE_F, O_FLYING_STONE, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_MEGA_STONE_F, O_MEGA_STONE, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_DIAMOND_F, O_DIAMOND, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_FLYING_DIAMOND_F, O_FLYING_DIAMOND, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_NUT_F, O_NUT, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_FALLING_WALL, O_BRICK, O_EXCLAMATION_MARK, 0);
    add_arrow_to_cell(O_FALLING_WALL_F, O_BRICK, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_TIME_PENALTY, O_GRAVESTONE, O_EXCLAMATION_MARK, 0);
    add_arrow_to_cell(O_NITRO_PACK_F, O_NITRO_PACK, O_DOWN_ARROW, 0);
    add_arrow_to_cell(O_NITRO_PACK_EXPLODE, O_NITRO_PACK, O_EXCLAMATION_MARK, 0);
    add_arrow_to_cell(O_CONVEYOR_LEFT, O_CONVEYOR_LEFT, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_CONVEYOR_RIGHT, O_CONVEYOR_RIGHT, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);

    add_arrow_to_cell(O_STONEFLY_1, O_STONEFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);
    add_arrow_to_cell(O_STONEFLY_2, O_STONEFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_STONEFLY_3, O_STONEFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_STONEFLY_4, O_STONEFLY_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_BITER_1, O_BITER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
    add_arrow_to_cell(O_BITER_2, O_BITER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
    add_arrow_to_cell(O_BITER_3, O_BITER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_BITER_4, O_BITER_1, O_DOWN_ARROW, GDK_PIXBUF_ROTATE_CLOCKWISE);

    add_arrow_to_cell(O_PRE_INVIS_OUTBOX, O_OUTBOX_CLOSED, O_GLUED, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_PRE_OUTBOX, O_OUTBOX_OPEN, O_GLUED, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_INVIS_OUTBOX, O_OUTBOX_CLOSED, O_OUT, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_OUTBOX, O_OUTBOX_OPEN, O_OUT, GDK_PIXBUF_ROTATE_NONE);

    add_arrow_to_cell(O_UNKNOWN, O_STEEL, O_QUESTION_MARK, GDK_PIXBUF_ROTATE_NONE);
    add_arrow_to_cell(O_WAITING_STONE, O_STONE, O_EXCLAMATION_MARK, GDK_PIXBUF_ROTATE_NONE);

    /* blinking outbox: helps editor, drawing the cave is more simple */
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+0, gd_elements[O_OUTBOX_OPEN].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+1, gd_elements[O_OUTBOX_OPEN].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+2, gd_elements[O_OUTBOX_OPEN].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+3, gd_elements[O_OUTBOX_OPEN].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+4, gd_elements[O_OUTBOX_CLOSED].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+5, gd_elements[O_OUTBOX_CLOSED].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+6, gd_elements[O_OUTBOX_CLOSED].image_game);
    copy_cell(ABS(gd_elements[O_PRE_OUTBOX].image_simple)+7, gd_elements[O_OUTBOX_CLOSED].image_game);
    
    /* check if all cells have been generated; if not,
       some add_arrows or copy_cells are missing for a
       newly added element. do this only once per game run. */
    if (!checked_cells) {
        checked_cells=TRUE;
        for (i=0; gd_elements[i].element!=-1; i++) {
            check_pixbuf(gd_elements[i].image, gd_elements[i].name);
            check_pixbuf(gd_elements[i].image_simple, gd_elements[i].name);
            check_pixbuf(gd_elements[i].image_game, gd_elements[i].name);
        }
    }
}

static guint32
rgba_pixel_from_color(GdColor col, guint8 a)
{
    guint8 r=gd_color_get_r(col);
    guint8 g=gd_color_get_g(col);
    guint8 b=gd_color_get_b(col);
#if G_BYTE_ORDER==G_LITTLE_ENDIAN
    return r+(g<<8)+(b<<16)+(a<<24);
#else
    return (r<<24)+(g<<16)+(b<<8)+a;
#endif
}

static void
loadcells_c64_with_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
    const guchar *gfx;    /* currently used graphics, will point to c64_gfx or c64_custom_gfx */
    GdkPixbuf *cells_pixbuf;
    guint32 cols[9];    /* holds rgba for color indexes internally used */
    int rowstride, n_channels;
    guchar *pixels;
    int pos, x, y;

    gfx=c64_custom_gfx?c64_custom_gfx:c64_gfx;

    cols[0]=rgba_pixel_from_color(0, 0);
    cols[1]=rgba_pixel_from_color(c0, 0xff); /* c64 background */
    cols[2]=rgba_pixel_from_color(c1, 0xff); /* foreg1 */
    cols[3]=rgba_pixel_from_color(c2, 0xff); /* foreg2 */
    cols[4]=rgba_pixel_from_color(c3, 0xff); /* foreg3 */
    cols[5]=rgba_pixel_from_color(c4, 0xff); /* amoeba */
    cols[6]=rgba_pixel_from_color(c5, 0xff); /* slime */
    cols[7]=rgba_pixel_from_color(0, 0xff);    /* black, opaque*/
    cols[8]=rgba_pixel_from_color(0xffffff, 0xff);    /* white, opaque*/

    cells_pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, NUM_OF_CELLS_X*gfx[0], NUM_OF_CELLS_Y*gfx[0]);
    n_channels=gdk_pixbuf_get_n_channels (cells_pixbuf);
    rowstride=gdk_pixbuf_get_rowstride (cells_pixbuf);    /* bytes / row */
    pixels=gdk_pixbuf_get_pixels (cells_pixbuf);    /* pointer to pixbuf memory */

    pos=1;    /* index to gfx array */
    /* create colored pixbuf from c64 graphics, using c0, c1, c2, c3 */
    for (y=0; y<NUM_OF_CELLS_Y*gfx[0]; y++) {
        guint32 *p=(guint32*) (pixels + y * rowstride);    /* write 32bits at once - faster than writing bytes */

        for (x=0; x<NUM_OF_CELLS_X*gfx[0]; x++)
            p[x]=cols[(int) gfx[pos++]];
    }

    /* from here, same as any other png */
    loadcells_from_pixbuf(cells_pixbuf);
    g_object_unref(cells_pixbuf);
}



static guchar *
c64_gfx_data_from_pixbuf(GdkPixbuf *pixbuf)
{
    int width, height, rowstride, n_channels;
    guchar *pixels;
    int x, y;
    guchar *data;
    int out;

    g_assert(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB);
    g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8);
    g_assert(gdk_pixbuf_get_has_alpha(pixbuf));

    n_channels=gdk_pixbuf_get_n_channels (pixbuf);
    width=gdk_pixbuf_get_width (pixbuf);
    height=gdk_pixbuf_get_height (pixbuf);
    rowstride=gdk_pixbuf_get_rowstride (pixbuf);
    pixels=gdk_pixbuf_get_pixels (pixbuf);

    g_assert (n_channels == 4);

    data=g_new(guchar, width*height+1);
    out=0;
    data[out++]=width/NUM_OF_CELLS_X;

    for (y=0; y<height; y++)
        for (x=0; x<width; x++) {
            int r, g, b, a;

            guchar *p=pixels + y * rowstride + x * n_channels;
            r=p[0];
            g=p[1];
            b=p[2];
            a=p[3];
            data[out++]=c64_png_colors(r, g, b, a);
        }
    return data;
}



gboolean
gd_loadcells_file(const char *filename)
{
    GdkPixbuf *cells_pixbuf;
    GError *error=NULL;
    const char *error_msg;

    /* load cell graphics */
    /* load from file */
    cells_pixbuf=gdk_pixbuf_new_from_file (filename, &error);
    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
        return FALSE;
    }
    /* check if file has the properties which we need for a theme */
    error_msg=gd_is_pixbuf_ok_for_theme(cells_pixbuf);
    if (error_msg) {
        g_object_unref(cells_pixbuf);
        g_warning("%s", error_msg);
        return FALSE;
    }

    if (check_if_pixbuf_c64_png(cells_pixbuf)) {
        /* c64 pixbuf with a small number of colors which can be changed */
        g_free(c64_custom_gfx);
        c64_custom_gfx=c64_gfx_data_from_pixbuf(cells_pixbuf);
        using_png_gfx=FALSE;
    } else {
        /* normal, "truecolor" pixbuf */
        g_free(c64_custom_gfx);
        c64_custom_gfx=NULL;
        loadcells_from_pixbuf(cells_pixbuf);
        using_png_gfx=TRUE;
    }
    g_object_unref(cells_pixbuf);
    color0=GD_COLOR_INVALID;    /* so that pixbufs will be recreated */

    return TRUE;
}

void
gd_loadcells_default()
{
    g_free(c64_custom_gfx);
    c64_custom_gfx=NULL;
    using_png_gfx=FALSE;
    color0=GD_COLOR_INVALID;    /* so that pixbufs will be recreated */
    
    /* size of array (in bytes), -1 which is the cell size */
    g_assert(sizeof(c64_gfx)-1 == NUM_OF_CELLS_X*NUM_OF_CELLS_Y*c64_gfx[0]*c64_gfx[0]);
}

/* wrapper */
/* exported for settings window */
void
gd_pal_emulate_pixbuf(GdkPixbuf *pixbuf)
{
#if G_BYTE_ORDER == G_BIG_ENDIAN
    guint32 rshift=24;
    guint32 gshift=16;
    guint32 bshift=8;
    guint32 ashift=0;
#else
    guint32 rshift=0;
    guint32 gshift=8;
    guint32 bshift=16;
    guint32 ashift=24;
#endif
    g_assert(gdk_pixbuf_get_has_alpha(pixbuf));

    gd_pal_emulate_raw(gdk_pixbuf_get_pixels(pixbuf), gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), gdk_pixbuf_get_rowstride(pixbuf), rshift, gshift, bshift, ashift);
}

/* create pixmap for image number 'index', using cell size. */
/* a pixmap * array is to be given; that may be cells_game or cells_editor */
/* pal emulation does just that. */
/* if render_selected is true, it will render the blue cell, too - that is used only in the editor. */
static void
create_pixmap(GdkPixmap **cells, int index, int cell_size, GdScalingType type, gboolean render_selected, gboolean pal_emul)
{
    GdkPixbuf *selected, *normal, *element;
    GdkWindow *window;

    g_assert(index>=0 && index<NUM_OF_CELLS);
    g_assert(cells_pb[index]!=NULL);

    window=gdk_get_default_root_window();

    /* scale the cell.
     * scale every cell on its own, or else some pixels might be merged on borders */
    normal=gd_pixbuf_scale(cells_pb[index], type);
    if (pal_emul)
        gd_pal_emulate_pixbuf(normal);
    /* create colored cells. */
    /* here no scaling is done, so interp_nearest is ok. */

    /* create pixmap containing pixbuf */
    /* draw the pixbufs to the new pixmaps */
    cells[index]=gdk_pixmap_new (window, cell_size, cell_size, -1);
    gdk_draw_pixbuf(cells[index], NULL, normal, 0, 0, 0, 0, cell_size, cell_size, GDK_RGB_DITHER_MAX, 0, 0);

    element=gdk_pixbuf_composite_color_simple(normal, cell_size, cell_size, GDK_INTERP_NEAREST, 128, 1, gd_flash_color, gd_flash_color);
    cells[NUM_OF_CELLS + index]=gdk_pixmap_new (window, cell_size, cell_size, -1);
    gdk_draw_pixbuf(cells[NUM_OF_CELLS + index], NULL, element, 0, 0, 0, 0, cell_size, cell_size, GDK_RGB_DITHER_MAX, 0, 0);
    g_object_unref(element);

    if (render_selected) {
        selected=gdk_pixbuf_composite_color_simple(normal, cell_size, cell_size, GDK_INTERP_NEAREST, 128, 1, gd_select_color, gd_select_color);
        cells[2 * NUM_OF_CELLS + index]=gdk_pixmap_new (window, cell_size, cell_size, -1);
        gdk_draw_pixbuf(cells[2 * NUM_OF_CELLS + index], NULL, selected, 0, 0, 0, 0, cell_size, cell_size, GDK_RGB_DITHER_MAX, 0, 0);
        g_object_unref(selected);
    }

    /* forget scaled pixbufs, as only the pixmaps are needed */
    g_object_unref(normal);
}

/* return a pixmap scaled for the game */
GdkPixmap *
gd_game_pixmap(int index)
{
    int i;

    g_assert(index<2*NUM_OF_CELLS);    /* make sure that no blue/"selected" pixbuf is requested for a game */
    i=index%NUM_OF_CELLS;    /* might request a colored, so we do a % NUM_OF_CELLS */
    if (cells_game[i]==NULL)    /* check if pixmap already exists */
        create_pixmap(cells_game, i, gd_cell_size_game, gd_cell_scale_game, FALSE, gd_pal_emulation_game);
    return cells_game[index];
}

/* return a pixmap scaled for the editor */
GdkPixmap *
gd_editor_pixmap(int index)
{
    int i;

    i=index%NUM_OF_CELLS;    /* might request a colored, so we do a % NUM_OF_CELLS */
    if (cells_editor[i]==NULL)    /* check if pixmap already exists */
        create_pixmap(cells_editor, i, gd_cell_size_editor, gd_cell_scale_editor, TRUE, gd_pal_emulation_editor);
    return cells_editor[index];
}

/* set pixbuf colors. */
/* (if png graphics is used, it does nothing. */
void
gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
    /* if non-c64 gfx, nothing to do */
    if (using_png_gfx)
        return;

    /* convert to rgb value */
    c0=gd_color_get_rgb(c0);
    c1=gd_color_get_rgb(c1);
    c2=gd_color_get_rgb(c2);
    c3=gd_color_get_rgb(c3);
    c4=gd_color_get_rgb(c4);
    c5=gd_color_get_rgb(c5);

    /* and compare rgb values! */
    if (c0!=color0 || c1!=color1 || c2!=color2 || c3!=color3 || c4!=color4 || c5!=color5) {
        /* if not the same colors as requested before */
        color0=c0;
        color1=c1;
        color2=c2;
        color3=c3;
        color4=c4;
        color5=c5;

        loadcells_c64_with_colors(c0, c1, c2, c3, c4, c5);
    }
}


static GdkPixbuf *
get_element_pixbuf_with_border(int index)
{
    if (!combo_pb[index]) {
        /* create small size pixbuf if needed. */
        int x, y;
        GdkPixbuf *pixbuf, *pixbuf_border;

        /* scale pixbuf to that specified by gtk */
        gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &x, &y);
        pixbuf=gdk_pixbuf_scale_simple(cells_pb[index], x, y, GDK_INTERP_BILINEAR);
        /* draw a little black border around image, makes the icons look much better */
        pixbuf_border=gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha (pixbuf), 8, x+2, y+2);
        gdk_pixbuf_fill(pixbuf_border, 0x000000ff);    /* RGBA: opaque black */
        gdk_pixbuf_copy_area(pixbuf, 0, 0, x, y, pixbuf_border, 1, 1);
        g_object_unref(pixbuf);
        combo_pb[index]=pixbuf_border;
    }
    return combo_pb[index];
}

/*
    returns a cell pixbuf, scaled to gtk icon size.
    it also adds a little black border, which makes them look much better
*/
GdkPixbuf *
gd_get_element_pixbuf_with_border(GdElement element)
{
    int index;
    /* which pixbuf to show? */
    index=ABS(gd_elements[element].image);
    return get_element_pixbuf_with_border(index);
}

/*
    returns a cell pixbuf, scaled to gtk icon size.
    it also adds a little black border, which makes them look much better
*/
GdkPixbuf *
gd_get_element_pixbuf_simple_with_border (GdElement element)
{
    int index;
    /* which pixbuf to show? */
    index=ABS(gd_elements[element].image_simple);
    return get_element_pixbuf_with_border(index);
}

/*
    creates a pixbuf, which shows the cave.
    if width and height are given (nonzero),
    scale pixbuf proportionally, so it fits in width*height
    pixels. otherwise return in original size.
    up to the caller to unref the returned pixbuf.
    also up to the caller to call this function only for rendered caves.
*/
GdkPixbuf *
gd_drawcave_to_pixbuf(const GdCave *cave, const int width, const int height, const gboolean game_view, const gboolean border)
{
    int x, y;
    int cell_size;
    GdkPixbuf *pixbuf, *scaled;
    float scale;
    int x1, y1, x2, y2;
    int borderadd=border?4:0, borderpos=border?2:0;

    g_assert(cave->map!=NULL);
    if (game_view) {
        /* if showing the visible part only */
        x1=cave->x1;
        y1=cave->y1;
        x2=cave->x2;
        y2=cave->y2;
    } else {
        /* showing entire cave - for example, overview in editor */
        x1=0;
        y1=0;
        x2=cave->w-1;
        y2=cave->h-1;
    }

    gd_select_pixbuf_colors(cave->color0, cave->color1, cave->color2, cave->color3, cave->color4, cave->color5);

    /* get size of one cell in the original pixbuf */
    cell_size=gdk_pixbuf_get_width (cells_pb[0]);

    /* add two pixels black border: +4 +4 for width and height */
    pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha (cells_pb[0]), 8, (x2-x1+1)*cell_size+borderadd, (y2-y1+1)*cell_size+borderadd);
    if (border)
        gdk_pixbuf_fill(pixbuf, 0x000000ff);    /* fill with opaque black, so border is black */

    /* take visible part into consideration */
    for (y=y1; y<=y2; y++)
        for (x=x1; x<=x2; x++) {
            GdElement element=cave->map[y][x]&O_MASK;
            int draw;

            if (game_view) {
                /* visual effects */
                switch(element) {
                    case O_DIRT:
                        element=cave->dirt_looks_like;
                        break;
                    case O_EXPANDING_WALL:
                    case O_H_EXPANDING_WALL:
                    case O_V_EXPANDING_WALL:
                        /* only change the view, if it is not brick wall (the default value). */
                        /* so arrows remain - as well as they always remaing for the steel expanding wall,
                           which has no visual effect. */
                        if (cave->expanding_wall_looks_like!=O_BRICK)
                            element=cave->expanding_wall_looks_like;
                        break;
                    case O_AMOEBA_2:
                        element=cave->amoeba_2_looks_like;
                        break;
                    default:
                        /* we check that this element has no visual effect. */
                        /* otherwise, we should have handled the element explicitely above! */
                        g_assert((gd_elements[element].properties & P_VISUAL_EFFECT) == 0);
                        break;
                }
                draw=ABS(gd_elements[element].image_simple);                /* pixbuf like in the editor */
            }
            else
                draw=gd_elements[element].image;                /* pixbuf like in the editor */
            gdk_pixbuf_copy_area (cells_pb[draw], 0, 0, cell_size, cell_size, pixbuf, (x-x1)*cell_size+borderpos, (y-y1)*cell_size+borderpos);
        }

    /* if requested size is 0, return unscaled */
    if (width == 0 || height == 0)
        return pixbuf;

    /* decide which direction fits in rectangle */
    /* cells are squares... no need to know cell_size here */
    if ((float) gdk_pixbuf_get_width (pixbuf) / (float) gdk_pixbuf_get_height (pixbuf) >= (float) width / (float) height)
        scale=width / ((float) gdk_pixbuf_get_width (pixbuf));
    else
        scale=height / ((float) gdk_pixbuf_get_height (pixbuf));

    /* scale to specified size */
    scaled=gdk_pixbuf_scale_simple (pixbuf, gdk_pixbuf_get_width (pixbuf)*scale, gdk_pixbuf_get_height (pixbuf)*scale, GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);

    return scaled;
}


GdkPixbuf *
gd_pixbuf_load_from_data(guchar *data, int length)
{
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    GError *error=NULL;
    
    /* push the data into a pixbuf loader */
    loader=gdk_pixbuf_loader_new();
    if (!gdk_pixbuf_loader_write(loader, data, length, &error) || !gdk_pixbuf_loader_close(loader, &error)) {
        g_warning("%s", error->message);
        g_error_free(error);
        g_object_unref(loader);
        return NULL;
    }
    pixbuf=gdk_pixbuf_loader_get_pixbuf(loader);
    g_object_ref(pixbuf);
    g_object_unref(loader);
    return pixbuf;
}

GdkPixbuf *
gd_pixbuf_load_from_base64(gchar *base64)
{
    guchar *data;
    gsize length;
    GdkPixbuf *pixbuf;
    
    data=g_base64_decode(base64, &length);
    pixbuf=gd_pixbuf_load_from_data(data, length);
    g_free(data);

    return pixbuf;
}




/* create a pixbuf of the caveset-specific title image. */
/* if there is no such image, return null. */
GdkPixbuf *
gd_create_title_image()
{
    guchar *data;
    gsize length;
    GdkPixbuf *screen, *tile, *tile_black, *bigone;
    int w, h;    /* screen (large image) width and height */
    int tw, th;    /* tile (small image) width and height */
    int x, y;
    
    if (gd_caveset_data->title_screen->len==0)
        return NULL;
    
    data=g_base64_decode(gd_caveset_data->title_screen->str, &length);
    screen=gd_pixbuf_load_from_data(data, length);
    g_free(data);
    if (!screen) {
        g_warning("Invalid title image stored in caveset.");
        g_string_assign(gd_caveset_data->title_screen, "");    /* forget if it had some error... */
        return NULL;
    }

    tile=NULL;
    if (gd_caveset_data->title_screen_scroll->len!=0) {
        /* there is a tile, so load that also. */
        data=g_base64_decode(gd_caveset_data->title_screen_scroll->str, &length);
        tile=gd_pixbuf_load_from_data(data, length);
        g_free(data);
        if (!tile) {
            g_warning("Invalid title image scrolling background stored in caveset.");
            g_string_assign(gd_caveset_data->title_screen_scroll, "");    /* forget if it had some error... */
            return NULL;
        }
    }
        
    w=gdk_pixbuf_get_width(screen);
    h=gdk_pixbuf_get_height(screen);
    
    if (!tile) {
        /* one-row pixbuf, so no animation. */
        tile=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, 1);
        gdk_pixbuf_fill(tile, 0x000000FFU);    /* opaque black */
    }
    
    tw=gdk_pixbuf_get_width(tile);
    th=gdk_pixbuf_get_height(tile);

    /* either because the tile has no alpha channel, or because it cannot be transparent anymore... */
    /* also needed because the "bigone" pixbuf will have an alpha channel, and pixbuf_copy would not work otherwise. */
    tile_black=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, tw, th);
    gdk_pixbuf_fill(tile_black, 0x000000FFU);    /* fill with opaque black, as even the tile may have an alpha channel */
    gdk_pixbuf_composite(tile, tile_black, 0, 0, tw, th, 0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
    g_object_unref(tile);
    tile=tile_black;

    /* create a big image, which is one tile larger than the title image size */
    bigone=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    /* and fill it with the tile. */
    for (y=0; y<h; y+=th)
        for (x=0; x<w; x+=tw) {
            int cw, ch;    /* copied width and height */
            
            /* check if out of bounds, as gdk does not clip rather sends errors */
            if (x+tw>w)
                cw=w-x;
            else
                cw=tw;
            if (y+th>h)
                ch=h-y;
            else
                ch=th;
            gdk_pixbuf_copy_area(tile, 0, 0, cw, ch, bigone, x, y);
        }
    g_object_unref(tile);
    gdk_pixbuf_composite(screen, bigone, 0, 0, w, h, 0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
    g_object_unref(screen);

    /* the image is now loaded and ready. */
    return bigone;
}


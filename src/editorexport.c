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

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "util.h"
#include "cave.h"
#include "cavedb.h"
#include "caveobject.h"
#include "caveset.h"
#include "c64import.h"
#include "gtkgfx.h"

static int crli_steel_code=0x38;    /* magic value: the code of steel wall in crli */


static int element_to_crli(GdElement e, GHashTable *unknown)
{
    int i;
    int code=-1;

    /* hack: there is no separate horizontal and vertical growing wall in crli. */
    /* only the switch can determine the direction. */
    if (e==O_V_EXPANDING_WALL)
        e=O_H_EXPANDING_WALL;
            
    /* 128 is the number of elements in gd_crazylight_import_table */
    for (i=0; i<128; i++)
        if (gd_crazylight_import_table[i]==e) {
            code=i;
            break;    /* if found, exit loop */
        }
    if (code==-1 && (e&SCANNED)) {
        e=e & ~SCANNED;
        
        /* try once again, without the "scanned" flag, as the element we use may not have a delayed state originally */
        for (i=0; i<128; i++)
            if (gd_crazylight_import_table[i]==e) {
                code=i;
                break;    /* if found, exit loop */
            }
    }
        
    if (code==-1) {
        g_assert(gd_crazylight_import_table[crli_steel_code]==O_STEEL);

        if (g_hash_table_lookup(unknown, GINT_TO_POINTER(code))==NULL) {
            g_warning("the element '%s' can not be saved in crli, saving steel wall instead", gd_elements[e].name);
            g_hash_table_insert(unknown, GINT_TO_POINTER(e), GINT_TO_POINTER(e));
        }
        
        code=crli_steel_code;
    }
    
    return code;
}

static int
crli_export(GdCave *to_convert, const int level, guint8 *compressed)
{
    guint8 output[0x3b0];
    int x, y, i;
    GdCave *cave;
    /* for rle */
    guint8 prev;
    int count;
    int out;
    int prob;
    GHashTable *unknown;
    gboolean has_horizontal, has_vertical;
    
    /* render cave with seed=0 */
    cave=gd_cave_new_rendered(to_convert, level, 0);
    gd_error_set_context(to_convert->name);
    unknown=g_hash_table_new(g_direct_hash, g_direct_equal);    /* hash table to remember unconvertable elements */

    /* do some checks */
    if (!cave->lineshift)
        g_warning("crli only supports line shifting map wraparound");
    if (!cave->pal_timing || cave->scheduling!=GD_SCHEDULING_PLCK)
        g_warning("only applicable timing settings for crli are pal timing=true, scheduling=plck");
    if (cave->amoeba_timer_started_immediately)
        g_warning("crli amoeba timer is only started when the amoeba is let free!");
    if (!cave->amoeba_timer_wait_for_hatching) {
        cave->amoeba_time+=2;
        g_message("crli amoeba timer waits for hatching; added 2 seconds for correction");
    }
    if (!cave->voodoo_dies_by_stone || !cave->voodoo_collects_diamonds || cave->voodoo_disappear_in_explosion)
        g_warning("crli voodoo dies by stone hit, can collect diamonds and can't be destroyed");
    if (cave->short_explosions)
        g_warning("crli explosions are slower than original");
    if (!cave->magic_timer_wait_for_hatching) {
        cave->magic_wall_time+=2;
        g_message("crli magic wall timer waits for hatching; added 2 seconds for correction");
    }
    if (!cave->slime_predictable)
        g_warning("crli only supports predictable slime");

    /* fill data bytes with some defaults */
    g_assert(gd_crazylight_import_table[crli_steel_code]==O_STEEL);    /* check magic value */
    for (i=0; i<40*22; i++)    /* fill map with steel wall */
        output[i]=crli_steel_code;
    for (i=40*22; i<G_N_ELEMENTS(output); i++)    /* fill properties with zero */
        output[i]=0;

    /* check cave sizes */
    if (cave->w!=40 || cave->h!=22)
        g_warning("cave sizes out of range, should be 40x22 instead of %dx%d", cave->w, cave->h);
    gd_cave_correct_visible_size(cave);
    if (cave->intermission) {    /* visible size for intermissions */
        if (cave->x1!=0 || cave->y1!=0 || cave->x2!=19 || cave->y2!=11)
            g_warning("for intermissions, the upper left 20x12 elements should be visible");
    } else {    /* visible size for normal caves */
        if (cave->x1!=0 || cave->y1!=0 || cave->x2!=cave->w-1 || cave->y2!=cave->h-1)
            g_warning("for normal caves, the whole cave should be visible");
    }

    /* convert map */
    has_horizontal=FALSE;
    has_vertical=FALSE;
    for (y=0; y<cave->h && y<22; y++)
        for (x=0; x<cave->w && x<40; x++) {
            output[y*40+x]=element_to_crli(cave->map[y][x], unknown);

            if (cave->map[y][x]==O_H_EXPANDING_WALL)
                has_horizontal=TRUE;
            if (cave->map[y][x]==O_V_EXPANDING_WALL)
                has_vertical=TRUE;
        }

    output[0x370]=cave->level_time[level]/100%10;
    output[0x371]=cave->level_time[level]/10%10;
    output[0x372]=cave->level_time[level]/1%10;

    output[0x373]=cave->level_diamonds[level]/100%10;
    output[0x374]=cave->level_diamonds[level]/10%10;
    output[0x375]=cave->level_diamonds[level]/1%10;

    output[0x376]=cave->extra_diamond_value/100%10;
    output[0x377]=cave->extra_diamond_value/10%10;
    output[0x378]=cave->extra_diamond_value/1%10;

    output[0x379]=cave->diamond_value/100%10;
    output[0x37a]=cave->diamond_value/10%10;
    output[0x37b]=cave->diamond_value/1%10;
    
    output[0x37c]=cave->amoeba_time/256;
    output[0x37d]=cave->amoeba_time%256;

    output[0x37e]=cave->magic_wall_time/256;
    output[0x37f]=cave->magic_wall_time%256;
    
    if (cave->creatures_direction_auto_change_time) {
        output[0x380]=1;
        output[0x381]=cave->creatures_direction_auto_change_time;
    }
    
    prob=(int)(4E6/cave->amoeba_growth_prob-1+0.5);     /* 4.0/(prob/1E6) */
    if (prob!=0 && prob!=1 && prob!=3 && prob!=7 && prob!=15 && prob!=31 && prob!=63 && prob!=127)
        g_warning("cannot precisely export amoeba slow growth probability, %d", prob);
    output[0x382]=prob;
    prob=(int)(4E6/cave->amoeba_fast_growth_prob-1+0.5);
    if (prob!=0 && prob!=1 && prob!=3 && prob!=7 && prob!=15 && prob!=31 && prob!=63 && prob!=127)
        g_warning("cannot precisely export amoeba fast growth probability");
    output[0x383]=prob;
    
    output[0x384]=gd_color_get_c64_index_try(cave->colorb);    
    output[0x385]=gd_color_get_c64_index_try(cave->color0);    
    output[0x386]=gd_color_get_c64_index_try(cave->color1);    
    output[0x387]=gd_color_get_c64_index_try(cave->color2);
    x=gd_color_get_c64_index_try(cave->color3);
    if (x>7)
        g_warning("allowed colors for color3 are Black..Yellow, but it is %d", x);
    output[0x388]=x|8;
    
    output[0x389]=cave->intermission?1:0;
    output[0x38a]=cave->level_ckdelay[level];
    
    output[0x38b]=cave->slime_permeability_c64;
    output[0x38c]=0;    /* always "normal" intermission, scrolling intermission is said to be buggy */
    output[0x38d]=0xf1;    /* magic wall sound on */
    
    /* if changed direction, we swap the flags here. */
    /* effects are already converted by element_to_crli */
    if (cave->expanding_wall_changed) {
        gboolean temp;
        
        temp=has_horizontal;
        has_horizontal=has_vertical;
        has_vertical=temp;
    }
    output[0x38e]=0x2e;
    if (has_vertical && !has_horizontal)
        output[0x38e]=0x2f;
    if (has_horizontal && has_vertical)
        g_warning("a crli map cannot contain horizontal and vertical growing walls at the same time");
    output[0x38f]=cave->creatures_backwards?0x2d:0x2c;
    
    output[0x390]=cave->amoeba_max_count/256;
    output[0x391]=cave->amoeba_max_count%256;
    
    output[0x392]=cave->time_bonus;
    output[0x393]=cave->time_penalty;
    output[0x394]=cave->biter_delay_frame;
    output[0x395]=cave->magic_wall_stops_amoeba?0:1;    /* inverted! */
    
    output[0x396]=element_to_crli(cave->bomb_explosion_effect|SCANNED, unknown);
    output[0x397]=element_to_crli(cave->explosion_effect|SCANNED, unknown);
    if (cave->stone_falling_effect!=O_STONE_F)
        g_warning("crli does not support 'falling stone to' effect");
    output[0x398]=element_to_crli(cave->stone_bouncing_effect|SCANNED, unknown);
    output[0x399]=element_to_crli(cave->diamond_birth_effect|SCANNED, unknown);
    output[0x39a]=element_to_crli(cave->magic_diamond_to|SCANNED, unknown);
    if (cave->diamond_bouncing_effect!=O_DIAMOND)
        g_warning("crli does not support 'bouncing diamond turns to' effect");
    output[0x39b]=element_to_crli(cave->bladder_converts_by, unknown);
    output[0x39c]=element_to_crli(cave->diamond_falling_effect|SCANNED, unknown);
    if (cave->diamond_bouncing_effect!=O_DIAMOND)
        g_warning("crli does not support 'bouncing diamond turns to' effect");
    output[0x39d]=element_to_crli(cave->biter_eat, unknown);
    output[0x39e]=element_to_crli(cave->slime_eats_1, unknown);
    if (element_to_crli(cave->slime_eats_1, unknown)+3!=element_to_crli(cave->slime_converts_1|SCANNED, unknown))
        g_warning("cannot convert slime setting: %s to %s", gd_elements[cave->slime_eats_1].name, gd_elements[cave->slime_converts_1].name);
    output[0x39f]=element_to_crli(cave->slime_eats_2, unknown);
    if (element_to_crli(cave->slime_eats_2, unknown)+3!=element_to_crli(cave->slime_converts_2|SCANNED, unknown))
        g_warning("cannot convert slime setting: %s to %s", gd_elements[cave->slime_eats_2].name, gd_elements[cave->slime_converts_2].name);

    output[0x3a0]='V';    /* version number */
    output[0x3a1]='3';
    output[0x3a2]='.';
    output[0x3a3]='0';
    
    output[0x3a4]=cave->diagonal_movements?1:0;
    output[0x3a6]=element_to_crli(cave->amoeba_too_big_effect|SCANNED, unknown);
    output[0x3a7]=element_to_crli(cave->amoeba_enclosed_effect|SCANNED, unknown);
    output[0x3a8]=cave->acid_spread_ratio*255.0/1E6+0.5;    /* /1E6 as it is a probability */
    output[0x3a9]=element_to_crli(cave->acid_eats_this, unknown);
    output[0x3ab]=element_to_crli(cave->expanding_wall_looks_like, unknown);
    output[0x3ac]=element_to_crli(cave->dirt_looks_like, unknown);
    
    prev=output[0];
    i=1;
    out=0;
    count=1;
    while (i<G_N_ELEMENTS(output)) {
        if (output[i]==prev) {            /* same as previous */
            /* if it would be too much for the length to be fit in a byte, write now */
            if (count>253) {
                compressed[out++]=0xbf;
                compressed[out++]=prev;    /* which is == 0xbf */
                compressed[out++]=count;
                
                count=0;
            }
            count++;
        } else {
            /* not the same as the previous ones, so write out those */
            if (count==1 && prev!=0xbf)    /* output previous character */
                compressed[out++]=prev;
            else if (count==1 && prev==0xbf) { /* output previous character, but it is accidentally the escape code */
                compressed[out++]=0xbf;
                compressed[out++]=prev;    /* which is == 0xbf */
                compressed[out++]=1;
            } else if (count==2) {
                /* count=2 is not written as escape, byte, count, as it would make it longer */
                if (prev!=0xbf) {
                    /* if the byte to write is not the escape byte */
                    compressed[out++]=prev;    /* which is == 0xbf */
                    compressed[out++]=prev;    /* which is == 0xbf */
                } else {
                    /* we have two escape bytes */
                    compressed[out++]=0xbf;
                    compressed[out++]=prev;    /* which is == 0xbf */
                    compressed[out++]=2;
                }
            } else {
                /* count > 2 */
                compressed[out++]=0xbf;
                compressed[out++]=prev;
                compressed[out++]=count;
            }
            
            /* and process this one. */
            prev=output[i];
            count=1;
        }
        
        i++;    /* next byte to compress */
    }
    /* process remaining bytes; always write them as compressed with escape byte */
    compressed[out++]=0xbf;
    compressed[out++]=prev;
    compressed[out++]=count;
    
    gd_cave_free(cave);
    g_hash_table_destroy(unknown);
    
    /* return number of bytes */
    return out;
}

void
gd_export_cave_to_crli_cavefile(GdCave *cave, int level, const char *filename)
{
    guint8 data[1024];
    int size;
    GError *error=NULL;
    
    data[0x0]=0x00;
    data[0x1]=0xc4;
    data[0x2]='D';
    data[0x3]='L';
    data[0x4]='P';
    size=crli_export(cave, level, data+5);
    gd_error_set_context(NULL);
    if (!g_file_set_contents(filename, (gchar *)data, size+5, &error)) {
        /* could not save properly */
        g_warning("%s: %s", filename, error->message);
        g_error_free(error);
    }
}


void
gd_export_cave_list_to_crli_cavepack(GList *caveset, int level, const char *filename)
{
    GError *error=NULL;
    const int start=0x6ffa;
    guint8 out[0xcc00-start+1024];    /* max number of cavepack + 1024 so dont worry about buffer overrun :P */
    int pos;
    GList *iter;
    int i;
    
    /* start address */
    out[0]=0xfc;
    out[1]=0x6f;
    /* cavepack version number */
    out[0x6ffc - start]='V';
    out[0x6ffd - start]='3';
    out[0x6ffe - start]='.';
    out[0x6fff - start]='0';
    
    for (i=0; i<48; i++) {
        out[0x7000-start+i]=0xff;
        out[0x7030-start+i]=0xff;
        out[0x7060-start+i]=0xff;    /* no cave present */
    }
    out[0x7090-start]=0xff;
    out[0x7091-start]=0x60;
    out[0x7092-start]=0x60;
    out[0x7093-start]=0x60;
    pos=0x70a2;    /* first available byte; before that we have space for the name */
    
    for (iter=caveset, i=0; iter!=NULL; iter=iter->next, i++) {
        GdCave *cave=iter->data;
        int bytes;
        int c;
        gunichar ch;
        gchar *namepos;
        gboolean exportname;
        
        if (i>=48) {
            g_warning("maximum of 48 caves in a crli cavepack");
            break;
        }
        
        bytes=crli_export(cave, level, out+pos-start);
        
        if (pos+bytes>0xcbff) {
            g_warning("run out of data space, not writing this cave");
            break;
        }
        
        out[0x7000-start+i]=pos%256;    /* lsb */
        out[0x7030-start+i]=pos/256;    /* msb */
        out[0x7060-start+i]=cave->selectable?0:1;    /* selection table */
        
        /* write name */
        for (c=0; c<14; c++)    /* fill with space */
            out[pos-start-14+c]=0x20;

        exportname=TRUE;
        namepos=cave->name;
        c=0;
        ch=g_utf8_get_char(namepos);
        while (c<14 && ch!=0) {
            int j;
            gboolean succ=FALSE;
            
            out[pos-start-14+c]=' ';    /* space is default, also for unknown characters */
            if (ch==' ')        /* treat space as different, as gd_bd_internal_chars has lots of spaces at unknown character positions */
                succ=TRUE;
            else
            if (ch<256) {
                for (j=0; j<strlen(gd_bd_internal_chars); j++)
                    if (gd_bd_internal_chars[j]==g_ascii_toupper(ch)) {    /* search for the character */
                        out[pos-start-14+c]=j;
                        succ=TRUE;
                    }
            }

            if (!succ)
                exportname=FALSE;

            c++;
            namepos=g_utf8_next_char(namepos);
            ch=g_utf8_get_char(namepos);
        }
        if (!exportname)
            g_warning("couldn't export cave name properly");
        
        pos+=bytes+14;    /* jump number of bytes + place for next cave name */
    }
    pos-=14;    /* subtract 14 for place we left for next cave name, but there was no more cave */

    if (!g_file_set_contents(filename, (gchar *)out, pos-start, &error)) {
        /* could not save properly */
        g_warning("%s: %s", filename, error->message);
        g_error_free(error);
    }
}


static void
string_printf_markup(GString *string, const char *format, ...)
{
    char *text;
    va_list ap;

    va_start(ap, format);
    text=g_markup_vprintf_escaped(format, ap);
    va_end(ap);
    g_string_append(string, text);
    g_free(text);
}

/* save caveset as html gallery.
   htmlname: filename
   window: show progress bar in a small dialog, transient for this window, if not NULL
*/
void
gd_save_html(char *htmlname, GtkWidget *window)
{
    char *pngbasename;        /* used as a base for img src= tags */
    char *pngoutbasename;    /* used as a base name for png output files */
    int i;
    GtkWidget *dialog=NULL, *progress=NULL;
    GString *contents;
    GError *error=NULL;
    
    contents=g_string_sized_new(20000);

    if (window) {
        dialog=gtk_dialog_new ();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        progress=gtk_progress_bar_new ();
        gtk_window_set_title (GTK_WINDOW (dialog), _("Saving HTML gallery"));
        gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), progress);
        gtk_widget_show_all (dialog);
    }

    if (g_str_has_suffix (htmlname, ".html")) {
        /* has html extension */
        pngoutbasename=g_strdup (htmlname);
        *g_strrstr(pngoutbasename, ".html")=0;
    }
    else {
        /* has no html extension */
        pngoutbasename=g_strdup(htmlname);
        htmlname=g_strdup_printf("%s.html", pngoutbasename);
    }
    pngbasename=g_path_get_basename(pngoutbasename);

    g_string_append(contents, "<HTML>\n");
    g_string_append(contents, "<HEAD>\n");
    string_printf_markup(contents, "<TITLE>%s</TITLE>\n", gd_caveset_data->name);
    g_string_append(contents, "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n");
    if (gd_html_stylesheet_filename)
        g_string_append_printf(contents, "<link rel=\"stylesheet\" href=\"%s\">\n", gd_html_stylesheet_filename);
    if (gd_html_favicon_filename)
        g_string_append_printf(contents, "<link rel=\"shortcut icon\" href=\"%s\">\n", gd_html_favicon_filename);
    g_string_append(contents, "</HEAD>\n\n");

    g_string_append(contents, "<BODY>\n");

    string_printf_markup(contents, "<H1>%s</H1>\n", gd_caveset_data->name);
    /* if the game has its own title screen */
    if (gd_caveset_data->title_screen->len!=0) {
        GdkPixbuf *title_image;
        char *pngname;

        /* create the title image and save it */        
        title_image=gd_create_title_image();
        pngname=g_strdup_printf("%s_%03d.png", pngoutbasename, 0);    /* it is the "zeroth" image */
        gdk_pixbuf_save(title_image, pngname, "png", &error, "compression", "9", NULL);
        if (error) {
            g_warning("%s", error->message);
            g_error_free(error);
            error=NULL;
        }
        g_free (pngname);

        g_string_append_printf(contents, "<IMAGE SRC=\"%s_%03d.png\" WIDTH=\"%d\" HEIGHT=\"%d\">\n", pngbasename, 0, gdk_pixbuf_get_width(title_image), gdk_pixbuf_get_height (title_image));
        g_string_append_printf(contents, "<BR>\n");
        
        g_object_unref(title_image);
    }
    g_string_append(contents, "<TABLE>\n");
    string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Caves"), gd_caveset_count());
    if (!g_str_equal(gd_caveset_data->author, ""))
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Author"), gd_caveset_data->author);
    if (!g_str_equal(gd_caveset_data->description, ""))
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Description"), gd_caveset_data->description);
    if (!g_str_equal(gd_caveset_data->www, ""))
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("WWW"), gd_caveset_data->www);
    if (!g_str_equal(gd_caveset_data->remark->str, ""))
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Remark"), gd_caveset_data->remark->str);
    if (!g_str_equal(gd_caveset_data->story->str, ""))
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Story"), gd_caveset_data->story->str);
    g_string_append(contents, "</TABLE>\n");

    /* cave names, descriptions, hrefs */
    g_string_append(contents, "<DL>\n");
    for (i=0; i<gd_caveset_count(); i++) {
        GdCave *cave;
        
        cave=gd_return_nth_cave(i);
        string_printf_markup(contents, "<DT><A HREF=\"#cave%03d\">%s</A></DT>\n<DD>%s</DD>\n", i+1, cave->name, cave->description);
    }
    g_string_append(contents, "</DL>\n\n");

    for (i=0; i<gd_caveset_count(); i++) {
        GdkPixbuf *pixbuf;
        GdCave *cave;
        char *pngname;
        char *text;
        gboolean has_amoeba=FALSE, has_magic=FALSE;
        int x, y;
        GError *error=NULL;

        if (progress) {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress), (float) i/gd_caveset_count());
            text=g_strdup_printf("%d/%d", i, gd_caveset_count());
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), text);
            g_free(text);
            gdk_window_process_all_updates();
        }

        /* rendering cave for png: seed=0 */
        cave=gd_cave_new_from_caveset(i, 0, 0);
        pixbuf=gd_drawcave_to_pixbuf(cave, 0, 0, TRUE, FALSE);
        
        /* check cave to see if we have amoeba or magic wall. properties will be shown in html, if so. */
        for (y=0; y<cave->h; y++)
            for (x=0; x<cave->w; x++)
                if (gd_cave_get_rc(cave, x, y)==O_AMOEBA) {
                    has_amoeba=TRUE;
                    break;
                }
        for (y=0; y<cave->h; y++)
            for (x=0; x<cave->w; x++)
                if (gd_cave_get_rc(cave, x, y)==O_MAGIC_WALL) {
                    has_magic=TRUE;
                    break;
                }

        /* save image */        
        pngname=g_strdup_printf("%s_%03d.png", pngoutbasename, i + 1);
        gdk_pixbuf_save(pixbuf, pngname, "png", &error, "compression", "9", NULL);
        if (error) {
            g_warning("%s", error->message);
            g_error_free(error);
            error=NULL;
        }
        g_free (pngname);

        /* cave header */        
        string_printf_markup(contents, "<A NAME=\"cave%03d\"></A>\n<H2>%s</H2>\n", i+1, cave->name);
        g_string_append_printf(contents, "<IMAGE SRC=\"%s_%03d.png\" WIDTH=\"%d\" HEIGHT=\"%d\">\n", pngbasename, i+1, gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
        g_string_append(contents, "<BR>\n");
        g_string_append(contents, "<TABLE>\n");
        if (!g_str_equal(cave->author, ""))
            string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Author"), cave->author);
        if (!g_str_equal(cave->description, ""))
            string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Description"), cave->description);
        if (!g_str_equal(cave->remark->str, "")) {
            /* we must split the story into lines, and join them with html <br> */
            char **spl;
            char *join;
            char *escaped;
            
            escaped=g_markup_escape_text(cave->remark->str, -1);
            spl=g_strsplit_set(escaped, "\n", -1);
            g_free(escaped);
            /* maintain line breaks */
            join=g_strjoinv("<BR>\n", spl);
            g_strfreev(spl);
            g_string_append_printf(contents, "<TR><TH>%s<TD>%s\n", _("Remark"), join);    /* string already escaped! */
            g_free(join);
        }
        if (!g_str_equal(cave->story->str, "")) {
            /* we must split the story into lines, and join them with html <br> */
            char **spl;
            char *join;
            char *escaped;
            
            escaped=g_markup_escape_text(cave->story->str, -1);
            spl=g_strsplit_set(escaped, "\n", -1);
            g_free(escaped);
            /* maintain line breaks */
            join=g_strjoinv("<BR>\n", spl);
            g_strfreev(spl);
            g_string_append_printf(contents, "<TR><TH>%s<TD>%s\n", _("Story"), join);    /* string already escaped! */
            g_free(join);
        }
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Type"), cave->intermission ? _("Intermission") : _("Normal cave"));
        string_printf_markup(contents, "<TR><TH>%s<TD>%s\n", _("Selectable as start"), cave->selectable ? _("Yes") : _("No"));
        string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Diamonds needed"), cave->diamonds_needed);
        string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Diamond value"), cave->diamond_value);
        string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Extra diamond value"), cave->extra_diamond_value);
        string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Time (s)"), cave->time);
        if (has_amoeba)
            string_printf_markup(contents, "<TR><TH>%s<TD>%d, %d\n", _("Amoeba threshold and time (s)"), cave->amoeba_max_count, cave->amoeba_time);
        if (has_magic)
            string_printf_markup(contents, "<TR><TH>%s<TD>%d\n", _("Magic wall milling time (s)"), cave->magic_wall_time);
        g_string_append(contents, "</TABLE>\n");

        g_string_append(contents, "\n");

        gd_cave_free (cave);
        g_object_unref (pixbuf);
    }
    g_string_append(contents, "</BODY>\n");
    g_string_append(contents, "</HTML>\n");
    g_free (pngoutbasename);
    g_free (pngbasename);

    if (!g_file_set_contents (htmlname, contents->str, contents->len, &error)) {
        /* could not save properly */
        g_warning("%s", error->message);
        g_error_free(error);
    }
    g_string_free(contents, TRUE);
    
    if (dialog)
        gtk_widget_destroy(dialog);
}




/*
 * Copyright (c) 2007, 2008 Czirkos Zoltan <cirix@fw.hu>
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

const char *s_bd1="GDashBD1";
const char *s_bd2="GDashBD2";
const char *s_plc="GDashPLC";
const char *s_dlb="GDashDLB";
const char *s_atg="GDashATG";
const char *s_crl="GDashCRL";
const char *s_cd7="GDashCD7";
const char *s_1st="GDash1ST";

typedef enum _format {
	F_BD1,
	F_BD2,
	F_PLC,
	F_DLB,
	F_ATG,
	F_CRL,
	F_CRD,
	F_1ST,
} format;


int main(int argc, char *argv[])
{
	gsize length, skipbytes;
	GError *error=NULL;
	gchar *contents;
	gchar *out;
	gchar *outname;
	guint32 *outlength;
	format f;
	
	if (argc!=2 && argc!=3) {
		g_print("%s: converts old GDash imported binaries to new format.\n", argv[0]);
		g_print("Usage:\n");
		g_print("  %s <infile> [outfile]\n", argv[0]);
		
		return 1;
	}
	
	/* determine format */
	skipbytes=0;
	if (g_str_has_suffix(argv[1], ".bd1"))
		f=F_BD1;
	else
	if (g_str_has_suffix(argv[1], ".bd2"))
		f=F_BD2;
	else
	if (g_str_has_suffix(argv[1], ".plc"))
		f=F_PLC, skipbytes=2;
	else
	if (g_str_has_suffix(argv[1], ".dlb"))
		f=F_DLB, skipbytes=0x3e;
	else
	if (g_str_has_suffix(argv[1], ".atg"))
		f=F_ATG;
	else
	if (g_str_has_suffix(argv[1], ".crl"))
		f=F_CRL;
	else
	if (g_str_has_suffix(argv[1], ".crd"))
		f=F_CRD;
	else
	if (g_str_has_suffix(argv[1], ".1stb"))
		f=F_1ST, skipbytes=2;
	else {
		g_print("Cannot determine file format from extension: %s", argv[1]);
		return 2;
	}

	/* load file */	
	g_file_get_contents(argv[1], &contents, &length, &error);
	if (error) {
		g_print("%s", error->message);
		return 3;
	}
	
	/* bytes we don't need */
	contents+=skipbytes;
	length-=skipbytes;
	
	if (argv[2])
		outname=argv[2];
	else {
		/* this MUST find a '.' as one of the g_str_has_suffixes above was a match */
		*strrchr(argv[1], '.')=0;
		outname=g_strdup_printf("%s.gds", argv[1]);
	}
	
	/* create out data */
	out=g_malloc(length+12);	/* 8 bytes identifier, 4 bytes length */
	switch(f) {
		case F_BD1: strcpy(out, s_bd1); break;
		case F_BD2: strcpy(out, s_bd2); break;
		case F_PLC: strcpy(out, s_plc); break;
		case F_DLB: strcpy(out, s_dlb); break;
		case F_ATG: strcpy(out, s_atg); break;
		case F_CRL: strcpy(out, s_crl); break;
		case F_CRD: strcpy(out, s_cd7); break;
		case F_1ST: strcpy(out, s_1st); break;
	}
	outlength=(guint32 *)(out+8);
	*outlength=GUINT32_TO_LE(length);
	g_memmove(out+12, contents, length);

	/* write outfile */	
	g_file_set_contents(outname, out, length+12, &error);
	if (error) {
		g_print("%s", error->message);
		return 4;
	}
	
	/* success */
	return 0;
}


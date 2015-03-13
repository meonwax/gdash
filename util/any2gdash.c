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
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* identifiers in output file */
static const char *s_bd1="GDashBD1";
static const char *s_bd2="GDashBD2";
static const char *s_plc="GDashPLC";
static const char *s_pca="GDashPCA";
static const char *s_crl="GDashCRL";
static const char *s_cd7="GDashCD7";
static const char *s_1st="GDash1ST";
static const char *s_b1a="GDashB1A";	/* boulder dash 1, atari version */
static const char *s_b2a="GDashB2A";	/* boulder dash 2, atari version */

/* loaded memory map */
static unsigned char memory[65536];
static unsigned char out[65536];
static int outpos;

/* default effect table, taken from afl boulder dash. used to detect if
   game-wide diego effects are present. */
static unsigned char default_effect[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x08, 0x09, 0x0a, 0x0b,
	0x10, 0x10, 0x12, 0x12, 0x14, 0x14, 0x16, 0x16, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2a, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x30, 0x31, 0x32, 0x33, 0x38, 0x38, 0x3a, 0x3a, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x60, 0x46, 0x4e, 0x22, 0x2e, 0x62, 0x2e, 0x4a, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
	0x64, 0x44, 0x44, 0x44, 0x44, 0x48, 0x48, 0x48, 0x48, 0x00, 0x00, 0x00, 0x66, 0x68, 0x6a, 0x68,
	0x66, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x62, 0x66, 0x68, 0x6a, 0x00, 0x4e, 0x4e, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x4c, 0x4c, 0x40, 0x40, 0x42, 0x2e, 0x40,
};


/* load file, and fill memory map. returns true if success, prints error and returns false if fail */
static int loadfile(const char *filename)
{
	const unsigned char magic[]={0x56, 0x49, 0x43, 0x45, 0x20, 0x53, 0x6E, 0x61, 0x70, 0x73, 0x68, 0x6F, 0x74, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x1A, 0x01, 0x00, 0x43, 0x36, 0x34};
	unsigned char buf[24];
	int readbytes;
	FILE *fin;
	
	fin=fopen(filename, "rb");
	if (!fin) {
		printf("Cannot open file %s\n", filename);
		return 0;
	}

	assert(sizeof(magic)==sizeof(buf));
	readbytes=fread(buf, 1, sizeof(buf), fin);
	if (readbytes!=sizeof(buf)) {
		fclose(fin);
		printf("Could not read %d bytes.\n", (int) sizeof(buf));
		return 0;
	}
	
	if (memcmp(magic, buf, sizeof(buf))==0) {
		/* FOUND a vice snapshot file. */
		fseek(fin, 0x80, SEEK_SET);
		readbytes=fread(memory, 1, 65536, fin);
		if (readbytes!=65536) {
			fclose(fin);
			printf("Could not read 65536 bytes.\n");
			return 0;
		}
		printf("%s is a proper vice snapshot.\n", filename);
		fclose(fin);
		return 1;
	} else {
		int filesize;
		
		/* image map */
		/* check size */
		fseek(fin, 0, SEEK_END);
		filesize=ftell(fin);
		fseek(fin, 0, SEEK_SET);
		
		/* 65538 bytes: we hope that this is a full-memory map saved by vice. check it. */
		if (filesize==65538) {
			/* check start address */
			if (fgetc(fin)!=0 || fgetc(fin)!=0) {
				fclose(fin);
				printf("Memory map should begin from address 0000.\n");
				printf("Use save \"filename\" 0 0000 ffff in vice monitor.\n");
				return 0;
			}
			
			readbytes=fread(memory, 1, 65536, fin);
			if (readbytes!=65536) {
				fclose(fin);
				printf("Could not read 65536 bytes\n.\n");
				return 0;
			}
			
			printf("%s looks like a proper vice memory map.\n", filename);
			fclose(fin);
			return 1;
		} else
		/* or maybe a 64k map saved by atari800. read it. */
		if (filesize==65536) {
			readbytes=fread(memory, 1, 65536, fin);
			if (readbytes!=65536) {
				fclose(fin);
				printf("Could not read 65536 bytes\n.\n");
				return 0;
			}
			
			printf("%s lookslike a proper atari800 memory map.\n", filename);
			fclose(fin);
			return 1;
		} else {
			/* if not 65536, not 65538: report error. */
			fclose(fin);
			printf("Memory map file should be 65536+2 bytes long or 65536 bytes long.\n");
			printf("Use save \"filename\" 0 0000 ffff in vice monitor.\n");
			printf("Use write 0000 ffff filename in atari800 monitor.\n");
			return 0;
		}
		
		assert(0);
		return 0;
	}
}


/* save bd1 caves from memory map */
/* atari: a boolean value, true if try atari map. */
static int try_bd1(int atari)
{
	int i;
	/* there are cave pointers at 0x5806. two byte entries pointing to caves. */
	/* their value is relative to 0x582e. */
	/* atari values are 3500 and 3528. */
	int cavepointers=atari?0x3500:0x5806;
	int cavestart=atari?0x3528:0x582e;

	strcpy((char *)out, atari?s_b1a:s_bd1);
	outpos=12;
	printf("\n*** Trying to interpret BD1(e) caves...\n");

	/* try to autodetect */

	/* fixme what is this? */
	if (!atari) {
		if (memory[0x5f3a]!=0x44 || memory[0x5f3b]!=0x44 || memory[0x5f3c]!=0x48 || memory[0x5f3d]!=0x48) {
			printf("Assumptions failed for BD1(e).\n");
			return 0;
		}
	}
		
	/* 4*5 (4 groups * 5 caves (4cave+1intermission)) */
	for (i=0; i<4*5; i++) {
		int lo, hi, j, pos, start;
		
		lo=memory[cavepointers+i*2];
		hi=memory[cavepointers+i*2+1];
		start=pos=hi*256+lo+cavestart;
		if (pos>(0xffff-0x0400)) {
			printf("Cannot interpret memory contents as BD1(e) -- invalid cave pointer.\n");
			return 0;
		}
		printf("Cave %d, addr: %04x\n", i+1, pos);
		
		/* copy */
		/* first 32 bytes - cave options */
		for (j=0; j<32; j++)
			out[outpos++]=memory[pos++]; /* 5levels: 5random, 5diamond, 5time */

		/* now cave objects */
		j=memory[pos++];
		while (j!=0xFF) { /* végét jelenti; de kiírtuk ezt is */
			if (pos-start > 0x400) {
				/* bd1 caves cannot be this long */
				printf("Cannot interpret memory contents as BD1(e) -- cave data too long.\n");
				return 0;
			}
			out[outpos++]=j;
			if (j==0x0F) {
				/* crazy cream 3 extension: escape byte 0x0f means raster */
				out[outpos++]=memory[pos++];	/* param byte 1: object to draw */
				out[outpos++]=memory[pos++];	/* param byte 2: begin column */
				out[outpos++]=memory[pos++];	/* param byte 3: begin row */
				out[outpos++]=memory[pos++];	/* param byte 4: amount of rows */
				out[outpos++]=memory[pos++];	/* param byte 5: amount of columns */
				out[outpos++]=memory[pos++];	/* param byte 6: distance between rows */
				out[outpos++]=memory[pos++];	/* param byte 7: distance between columns */
			} else {
				switch (j>>6) {
					case 0: /* point */
						out[outpos++]=memory[pos++];	/* x */
						out[outpos++]=memory[pos++];	/* y */
						break;
					case 1: /* line */
						out[outpos++]=memory[pos++];	/* x */
						out[outpos++]=memory[pos++];	/* y */
						out[outpos++]=memory[pos++];	/* len */
						out[outpos++]=memory[pos++];	/* dir */
						break;
					case 2: /* fillrect */
						out[outpos++]=memory[pos++];	/* x */
						out[outpos++]=memory[pos++];	/* y */
						out[outpos++]=memory[pos++];	/* w */
						out[outpos++]=memory[pos++];	/* h */
						out[outpos++]=memory[pos++];	/* fill */
						break;
					case 3: /* outline */
						out[outpos++]=memory[pos++];	/* x */
						out[outpos++]=memory[pos++];	/* y */
						out[outpos++]=memory[pos++];	/* w */
						out[outpos++]=memory[pos++];	/* h */
						break;
				}
			}
			j=memory[pos++];
		}
		out[outpos++]=j; /* lezáró 0xff kiírása */
	}
	printf("Found %d BD1 caves!\n", 20);
	return 1;
}


/* save plck caves from c64 memory map */
static int try_plck()
{
	int x, i;
	int has_names;
	int has_diego=0;
	int valid;
	int ok;
	
	strcpy((char *)out, s_plc);
	outpos=12;
	printf("\n*** Trying to interpret PLCK caves...\n");
	/* try plck */
	valid=0;
	ok=0;

	/* try to detect plck cave selection table. assume there are at least 5 caves. */
	/* selection table without names. */
	for (x=0; x<5; x++)
		if (memory[0x5e8b+x]==0x0e || memory[0x5e8b+x]==0x19)
			ok++;
	if (ok==5) {
		valid=1;
		has_names=0;
	}
	
	/* selection table with names. */
	ok=0;
	for (x=0; x<5; x++)
		if (memory[0x5e8b+13*x+12]==0x0e || memory[0x5e8b+13*x+12]==0x19)
			ok++;
	if (ok==5) {
		valid=1;
		has_names=1;
	}
	if (!valid) {
		printf("Assumptions failed for PLCK - could not find cave selection table.\n");
		return 0;
	}

	printf(has_names?"PLCK caves have names.\n":"PLCK caves have no names.\n");
	
	i=0;
	/* while present and (selectable or nonselectable)   <- find any valid byte in cave selection table. */
	while ((!has_names && (memory[0x5e8b+i]!=0 && (memory[0x5e8b+i]==0x0e || memory[0x5e8b+i]==0x19)))
			|| (has_names && (memory[0x5e8b+i*13+12]!=0 && (memory[0x5e8b+i*13+12]==0x0e || memory[0x5e8b+i*13+12]==0x19)))) {
		int j, pos, zero;
		
		pos=0x7000+0x200*i;
		
		/* check if memory is not filled with zeroes. */
		zero=1;
		for (j=0; j<512 && zero; j++)
			if (memory[pos+j]!=0)
				zero=0;
		if (zero)
			break;

		/* check for error */
		if (i>71) {
			/* caves cannot be this long */
			printf("Data corrupt or detection failed for plck caves -- too many caves.\n");
			return 0;
		}
		
		printf("Cave %d, addr: %04x  ", i+1, pos);
		if (memory[pos+0x1e5]==0x20 && memory[pos+0x1e6]==0x90 && memory[pos+0x1e7]==0x46) {
			printf("- has diego effects.");
			has_diego=1;
		}
		printf("\n");
		
		/* copy 1f0 bytes for cave */
		for (j=0; j<0x1f0; ++j)
			out[outpos++]=memory[pos++];

		/* and fill the rest with our own data. this way we stay compatible, as cave was always 1f0 bytes */
		if (has_names) {	/* we detected that it has names */
			out[outpos++]=memory[0x5e8b+i*13+12];
			out[outpos++]=memory[0x5e8b+i*13+12]+1;	/* save twice, add +1 for second for detection in gdash */
			if (memory[0x5e8b+i*13+12]!=0x0e && memory[0x5e8b+i*13+12]!=0x19)
				printf("ERROR: cave selection table corrupt or autodetection failed?\n");
			for (j=0; j<12; j++)
				out[outpos++]=memory[0x5e8b+i*13+j];
			out[outpos++]=0;	/* fill the rest with zero */
			out[outpos++]=0;
		}
		else {	/* no names */
			out[outpos++]=memory[0x5e8b+i];
			out[outpos++]=memory[0x5e8b+i]+1;	/* save twice for detection, add 1 to second one */
			if (memory[0x5e8b+i]!=0x0e && memory[0x5e8b+i]!=0x19)
				printf("ERROR: cave selection table corrupt or autodetection failed?\n");
			for (j=2; j<16; j++)
				out[outpos++]=0;
		}

        i++;
    }
	printf("Found %d PLCK caves in %d bytes!\n", i, outpos);
	
	/* now try to do something about diego effects. */
	/* if we found at least one cave with diego effects, this check can be skipped. */
	if (!has_diego) {
		int j;
		const int numbers=sizeof(default_effect)/sizeof(default_effect[0]);
		char diffs[numbers];
		int n;

		/* check effect table from memory at 3b00. */
		n=0;
		for (j=0; j<numbers; j++)
			if (default_effect[j]!=memory[0x3b00+j]) {
				n++;
				diffs[j]=1;				
			} else
				diffs[j]=0;
		if (n!=0) {
			/* found an unstandard effect table */
			int b_stone_to=memory[0x3b00 + 0x11];
			int f_diamond_to=memory[0x3b00 + 0x17];
			int explosion_to=memory[0x3b00 + 0x1e];
			int dirt_pointer=memory[0x3b00 + 0x42];
			int growing_pointer=memory[0x3b00 + 0x6b];
			
			/* report effect table changes that we are able to convert to the per cave description. */
			printf ("Found global diego effects!\n");
			if (b_stone_to != 0x10)
				printf (" - Effect for bouncing stone.\n");
			if (f_diamond_to != 0x16)
				printf (" - Effect for falling diamond.\n");
			if (explosion_to != 0x1e)
				printf (" - Effect for explosion.\n");
			if (dirt_pointer != 0x46)
				printf (" - Dirt looks like effect.\n");
			if (growing_pointer != 0x4e)
				printf (" - Growing wall looks like effect.\n");

			/* go through all caves and add no1v5.3e compatible flags */
			for (j=0; j<i; j++) {
				/* flags used previously on c64; gdash also knows them */
				out[12+0x0200*j+0x1e5]=0x20;
				out[12+0x0200*j+0x1e6]=0x90;
				out[12+0x0200*j+0x1e7]=0x46;

				/* set detected stuff for cave */
				out[12+0x0200*j+0x1ea]=b_stone_to;
				out[12+0x0200*j+0x1eb]=f_diamond_to;
				out[12+0x0200*j+0x1ec]=explosion_to;
				out[12+0x0200*j+0x1ed]=dirt_pointer;
				out[12+0x0200*j+0x1ee]=growing_pointer;
				out[12+0x0200*j+0x1ef]=200;	/* FIXME AMOEBA THRESHOLD */
			}
		}
		/* null out effects we could handle */
		diffs[0x11]=0;
		diffs[0x17]=0;
		diffs[0x1e]=0;
		diffs[0x42]=0;
		diffs[0x6b]=0;
		/* check if growing wall (delayed)==growing wall in terms of graphics, if it is, then it is no diff to original */
		if (memory[0x3b00+0x6b]==memory[0x3b00+0x6c])
			diffs[0x6c]=0;
		
		/* and check if there are others we cannot fit into a "standard" cawe */
		for (j=0; j<numbers; j++)
			if (diffs[j]) {
				printf("*** Don't know how to handle effect for element number %x, default %x, this one %x\n", j, default_effect[j], memory[0x3b00+j]);
			}
	}
	return 1;
}



/* save plck caves from atari memory map */
static int try_atari_plck()
{
	int x, i;
	int ok;
	int has_diego=0;
	int has_selection;
	
	strcpy((char *)out, s_pca);
	outpos=12;
	printf("\n*** Trying to interpret Atari PLCK caves...\n");
	/* try plck */
	ok=0;

	/* try to detect the cave selection table. assume there are at least 5 caves. */
	/* example:
		6040: 66 60 4A 41 49 4C 20 20 20 20 2E 43 41 56 59 53  f.JAIL    .CAVYS
		6050: 41 46 45 20 20 20 20 2E 43 41 56 59 52 41 49 4E  AFE    .CAVYRAIN
		6060: 20 20 20 20 2E 43 41 56 59 45 53 43 41 50 41 44      .CAVYESCAPAD
		6070: 45 2E 43 41 56 59 43 48 41 53 45 20 20 20 2E 49  E.CAVYCHASE   .I
		6080: 4E 54 4E 52 45 53 43 55 45 20 20 2E 43 41 56 59  NTNRESCUE  .CAVY
		we detect the Y's and N's after CAV or INT
	*/
	
	/* caves are preceded with $ab $ab $ab $ab CAVENAME */
	/*
		6FF0: AB AB AB AB 4A 41 49 4C 2E 43 41 56 9B 20 20 20  ....JAIL.CAV.   
		7000: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
		7010: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
	*/
	/* try to detect this; assume at least 5 caves. */
	ok=0;
	for (x=0; x<5; x++) {
		int pos=0x6ff0 + x*0x200;
		if (memory[pos]==0xAB && memory[pos+1]==0xAB && memory[pos+2]==0xAB && memory[pos+3]==0xAB)
			ok++;
	}
	if (ok!=5) {
		printf("Assumptions failed for Atari PLCK - could not find $ab $ab $ab $ab before caves.\n");
		return 0;
	}
	
	/* check if it has a selection table */
	ok=0;
	for (x=0; x<5; x++) {
		if (memory[0x604e + x*13]=='Y' || memory[0x604e + x*13]=='N')
			ok++;
	}
	has_selection=(ok==5);
	if (has_selection)
		printf("Found selection table.\n");




	i=0;
	/* detect caves by $ab bytes before them */
	while (memory[0x6ff0+i*0x200]==0xAB && memory[0x6ff1+i*0x200]==0xAB && memory[0x6ff2+i*0x200]==0xAB && memory[0x6ff3+i*0x200]==0xAB) {
		int j, pos;
		int selectable;
		
		pos=0x7000+0x200*i;
		if (has_selection)
			selectable=memory[0x604e + i*13]=='Y';
		else
			/* has no selection table... we make intermissions unselectable. */
			selectable=memory[pos + 0x1da]==0;

		/* check for error */
		if (i>71) {
			/* caves cannot be this long */
			printf("Data corrupt or detection failed for plck caves -- too many caves.\n");
			return 0;
		}
		
		printf("Cave %d, addr: %04x, sel: %d", i+1, pos, selectable);
		if (memory[pos+0x1e5]==0x20 && memory[pos+0x1e6]==0x90 && memory[pos+0x1e7]==0x46) {
			printf("- has diego effects.");
			has_diego=1;
		}
		printf("\n");
		
		/* copy 1f0 bytes for cave */
		for (j=0; j<0x1f0; ++j)
			out[outpos++]=memory[pos++];

		/* and fill the rest with our own data. this way we stay compatible, as cave was always 1f0 bytes */
		/* we have to stay compatible with c64 plck import routine above. */
		/* so we use 0x19 for selectable caves, 0x0e for nonselectable. */
		out[outpos++]=selectable?0x19:0x0e;
		out[outpos++]=(selectable?0x19:0x0e)+1;	/* save twice, add +1 for second for detection in gdash */
		/* copy cave name, which is:
			6FE0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
			6FF0: AB AB AB AB 4A 41 49 4C 2E 43 41 56 9B 20 20 20  ....JAIL.CAV.   		... cave name
			7000: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD		... cave data
			7010: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
			7020: 44 44 44 44 44 44 44 44 4E 77 77 77 77 77 77 77  DDDDDDDDNwwwwwww
		*/

		pos=0x7000+0x200*i;
		for (j=0; j<12; j++) {
			unsigned char c=memory[pos-0x10+4+j];
		
			/* somehow this character set the end-of-string in atari version */
			if (c==0x9b)
				c=0x20;
			out[outpos++]=c;
		}
		out[outpos++]=0;	/* fill the rest with zero */
		out[outpos++]=0;

        i++;
    }
	printf("Found %d PLCK caves in %d bytes!\n", i, outpos);
	
	return 1;
}



/* save crazy light caves from memory map */
static int try_crli()
{
	int x, i, b, caves;

	strcpy((char *)out, s_crl);
	outpos=12;

	/* try autodetect */
	printf("\n*** Trying to interpret crazy light caves...\n");
	b=1;
	if (memory[0x6ffc]!='V' || memory[0x6ffd]!='3' || memory[0x6ffe]!='.' || memory[0x6fff]!='0')	/* version string */
		b=0;
	for (x=0x7060; x<0x7090; x++)	/* cave selection table */
		if (memory[x]!=0 && memory[x]!=1 && memory[x]!=255)
			b=0;	
	if (!b) {
		printf("Assumptions failed for crazy light.\n");
		return 0;
	}

	caves=0;
	for (i=0; i<48; i++)
		if (memory[0x7060+i]!=0xFF) {
			int j, n;
			int startpos, pos;
			int cavepos;
			
			caves++;

			startpos=memory[0x7000+i] + memory[0x7030+i]*256L;
			pos=startpos;
			printf("Cave %d, addr: %04x (%d), ", caves, pos, pos);
			cavepos=0;
			/* 'decompress' data, to see how many bytes there are */
			while (cavepos < 0x3b0) {	/* <- loop until the uncompressed reaches its size */
				if (memory[pos] == 0xbf) {	
					/* escaped byte */
					cavepos+=memory[pos+2];	/* number of bytes */
					pos += 3;
				}
				else {
					/* plain data */
					cavepos++;
					pos++;
				}
			}
			pos+=14;	/* add 14 bytes for name */
			n=pos-startpos;	/* bytes to copy */
			startpos-=14;	/* name is BEFORE the cave data */
			printf("length %d bytes\n", n);
			
			out[outpos++]=memory[0x7060+i];	/* is_selectable */
			for (j=0; j<n; j++)
				out[outpos++]=memory[startpos++];
	    }
	printf("Found %d crazy light caves in %d bytes!\n", caves, outpos);
	return 1;
}

static int
try_crdr()
{
	int i, b, caves;

	strcpy((char *)out, s_cd7);
	outpos=12;

	/* try autodetect */
	printf("\n*** Trying to interpret crazy dream caves...\n");
	b=1;
//	if (memory[0x6ffc]!='V' || memory[0x6ffd]!='3' || memory[0x6ffe]!='.' || memory[0x6fff]!='0')	/* version string */
//		b=0;

	caves=0;
	for (i=0; i<20; i++) {
		int j, n;
		int startpos, pos;
		
		caves++;

		startpos=memory[0x7500+i] + memory[0x7580+i]*256L;
		pos=startpos;
		printf("Cave %d, addr: %04x (%d), ", caves, pos, pos);

		/* name */
		for (j=0; j<14; j++)
			out[outpos++]=memory[0x8c00+i*16+j];

		/* selectable */
		/* XXX seems to be broken; rather set unselectable intermissions */
		out[outpos++]=/*memory[0x7410+caves]*/(caves%5)!=0;

		/* cave data */
		for (j=0; j<0x49; j++)
			out[outpos++]=memory[pos++];
		/* cave objects */
		while (memory[pos]!=0xff) {
			printf("%x ", memory[pos]);
			switch(memory[pos]) {
				case 1:	/* point */
					for (j=0; j<4; j++)
						out[outpos++]=memory[pos++];
					break;
				case 2: /* rectangle */
					for (j=0; j<6; j++)
						out[outpos++]=memory[pos++];
					break;
				case 3: /* fillrect */
					for (j=0; j<6; j++)
						out[outpos++]=memory[pos++];
					break;
				case 4: /* line */
					for (j=0; j<6; j++)
						out[outpos++]=memory[pos++];
					break;
				case 6: /* ??? */
					printf("[");
					for (j=0; j<5; j++) {
						printf("%02x ", memory[pos]);
						out[outpos++]=memory[pos++];
					}
					printf("]");
					break;
				case 7: /* ??? */
					printf("[");
					for (j=0; j<3; j++) {
						printf("%02x ", memory[pos]);
						out[outpos++]=memory[pos++];
					}
					printf("]");
					break;
				case 11: /* raster */
					for (j=0; j<8; j++)
						out[outpos++]=memory[pos++];
					break;
				default:
					printf("\n*** Unknown crdr object code %x...\n", memory[pos]);
					for (j=0; j<50; j++) {
						printf("%02x ", memory[pos]);
						out[outpos++]=memory[pos++];
					}
					return 0;
					break;
			}
		}
		out[outpos++]=memory[pos++];	/* copy $ff */
		n=pos-startpos;	/* bytes to copy */
		printf("length %d bytes\n", n);
    }
	printf("Found %d crazy dream caves in %d bytes!\n", caves, outpos);
	return 1;
}



/* save bd2 caves from memory map */
static int try_bd2()
{
	int i;
	int unsupported=0, uns[256];

	/* 256 counters for the 256 possible bytes (8bit). mark each unsupported extension found. */
	/* if more than 5 types found, bail out with error */
	for (i=0; i<256; ++i)
		uns[i]=0;
		
	strcpy((char *)out, s_bd2);
	outpos=12;
	printf("\n*** Trying to interpret BD2 caves...\n");
	
	/* 4*5 (4 groups * 5 caves (4cave+1intermission)) */
	for (i=0; i<4*5; i++) {
		int lo, hi, j, pos, start;
		int amount, n, mappos;
		
		lo=0x89b0+i*2;
		hi=0x89b0+i*2+1;
		start=pos=memory[hi]*256L+memory[lo];
		if (pos> (0xffff-0x0400)) {
			printf("Cannot interpret memory contents as BD2 -- invalid cave pointer.\n");
			return 0;
		}
		if (i<16)
			printf("Cave %c, addr: %04x\n", i+'A', pos);
		else
			printf("Intermission %d, addr: %04x\n", i-15, pos);
		
		/* copy */
		/* first bytes: cave options */
		for (j=0; j<=0x19; j++)
			out[outpos++]=memory[pos++];			/* cave options */

		j=memory[pos++];
		while (j!=0xFF) { /* végét jelenti; de kiírtuk ezt is */
			if (pos-start > 0x400) {
				/* bd1 caves cannot be this long */
				printf("Data corrupt or detection failed for bd2 caves -- cave data too long.\n");
				return 0;
			}
			out[outpos++]=j;
			switch (j) {
				case 0: /* line */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* dir */
					out[outpos++]=memory[pos++];	/* len */
					break;
				case 1: /* rectangle (outline) */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					break;
				case 2: /* fillrect */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					out[outpos++]=memory[pos++];	/* fillobj */
					break;
				case 3: /* point */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					break;
				case 4: /* raster */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					out[outpos++]=memory[pos++];	/* dy */
					out[outpos++]=memory[pos++];	/* dx */
					break;
				case 5: /* profi boulder extension: bitmap */
					out[outpos++]=memory[pos++];	/* obj */
					amount=memory[pos++];
					out[outpos++]=amount;	/* amount */
					out[outpos++]=memory[pos++];	/* target msb */
					out[outpos++]=memory[pos++];	/* target lsb */
					for (n=0; n<amount; ++n)
						out[outpos++]=memory[pos++];	/* data */
					break;
				case 6: /* join */
					out[outpos++]=memory[pos++];	/* add to this */
					out[outpos++]=memory[pos++];	/* add this */
					out[outpos++]=memory[pos++];	/* dy*40+dx */
					break;
				case 7: /* slime permeabilty */
					out[outpos++]=memory[pos++];	/* perm */
					break;
				case 9:	/* profi boulder extension: plck map */
					lo=memory[pos++];
					hi=memory[pos++];
					mappos=hi*256+lo;
					out[outpos++]=memory[pos++];	/* inbox y */
					out[outpos++]=memory[pos++];	/* inbox x */
					for (n=0; n<40*(22-2)/2; n++)	/* 40*20 caves, upper and lower row not contained, 1byte/2 elements */
						out[outpos++]=memory[mappos+n];
					break;
				default: 
					if (uns[j]==0) {
						/* not seen this extension previously */
						printf("  Found unsupported bd2 extension n.%d\n", j);	
						unsupported++;
						uns[j]=1;	/* mark the newly found unknown extension */
					}
					if (unsupported>5) {
						/* found to many unsupported extensions - this can't be bd2 */
						printf("Data corrupt or detection failed for bd2 caves --\n too many unknown extensions.\n");
						return 0;
					}
					break;
			}
			j=memory[pos++];	/* read next */
		}
		out[outpos++]=j; /* closing 0xff */
		out[outpos++]=memory[pos++];	/* animation */

		/* color table */
		lo=0x89d8+i*2;
		hi=0x89d8+i*2+1;
		pos=memory[hi]*256L+memory[lo];	/* pointer to the three colors */
		out[outpos++]=memory[pos++];
		out[outpos++]=memory[pos++];
		out[outpos++]=memory[pos++];
	}
	
	return 1;
}



/* save bd2 caves from memory map */
static int try_bd2_atari()
{
	const int cavepointers=0x86b0;
	const int cavecolors=0x86d8;
	int i;
	int unsupported=0, uns[256];

	/* 256 counters for the 256 possible bytes (8bit). mark each unsupported extension found. */
	/* if more than 5 types found, bail out with error */
	for (i=0; i<256; ++i)
		uns[i]=0;
		
	strcpy((char *)out, s_b2a);
	outpos=12;
	printf("\n*** Trying to interpret Atari BD2 caves...\n");
	
	/* 4*5 (4 groups * 5 caves (4cave+1intermission)) */
	for (i=0; i<4*5; i++) {
		int lo, hi, j, pos, start;
		int amount, n, mappos;
		
		lo=cavepointers+i*2;
		hi=cavepointers+i*2+1;
		start=pos=memory[hi]*256L+memory[lo];
		if (pos> (0xffff-0x0400)) {
			printf("Cannot interpret memory contents as BD2 -- invalid cave pointer.\n");
			return 0;
		}
		if (i<16)
			printf("Cave %c, addr: %04x\n", i+'A', pos);
		else
			printf("Intermission %d, addr: %04x\n", i-15, pos);
		
		/* copy */
		/* first bytes: cave options */
		for (j=0; j<=0x19; j++)
			out[outpos++]=memory[pos++];			/* cave options */

		j=memory[pos++];
		while (j!=0xFF) { /* végét jelenti; de kiírtuk ezt is */
			if (pos-start > 0x400) {
				/* bd1 caves cannot be this long */
				printf("Data corrupt or detection failed for bd2 caves -- cave data too long.\n");
				return 0;
			}
			out[outpos++]=j;
			switch (j) {
				case 0: /* line */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* dir */
					out[outpos++]=memory[pos++];	/* len */
					break;
				case 1: /* rectangle (outline) */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					break;
				case 2: /* fillrect */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					out[outpos++]=memory[pos++];	/* fillobj */
					break;
				case 3: /* point */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					break;
				case 4: /* raster */
					out[outpos++]=memory[pos++];	/* obj */
					out[outpos++]=memory[pos++];	/* y */
					out[outpos++]=memory[pos++];	/* x */
					out[outpos++]=memory[pos++];	/* h */
					out[outpos++]=memory[pos++];	/* w */
					out[outpos++]=memory[pos++];	/* dy */
					out[outpos++]=memory[pos++];	/* dx */
					break;
				case 5: /* profi boulder extension: bitmap */
					out[outpos++]=memory[pos++];	/* obj */
					amount=memory[pos++];
					out[outpos++]=amount;	/* amount */
					out[outpos++]=memory[pos++];	/* target msb */
					out[outpos++]=memory[pos++];	/* target lsb */
					for (n=0; n<amount; ++n)
						out[outpos++]=memory[pos++];	/* data */
					break;
				case 6: /* join */
					out[outpos++]=memory[pos++];	/* add to this */
					out[outpos++]=memory[pos++];	/* add this */
					out[outpos++]=memory[pos++];	/* dy*40+dx */
					break;
				case 7: /* slime permeabilty */
					out[outpos++]=memory[pos++];	/* perm */
					break;
				case 9:	/* profi boulder extension: plck map */
					lo=memory[pos++];
					hi=memory[pos++];
					mappos=hi*256+lo;
					out[outpos++]=memory[pos++];	/* inbox y */
					out[outpos++]=memory[pos++];	/* inbox x */
					for (n=0; n<40*(22-2)/2; n++)	/* 40*20 caves, upper and lower row not contained, 1byte/2 elements */
						out[outpos++]=memory[mappos+n];
					break;
				default: 
					if (uns[j]==0) {
						/* not seen this extension previously */
						printf("  Found unsupported bd2 extension n.%d\n", j);	
						unsupported++;
						uns[j]=1;	/* mark the newly found unknown extension */
					}
					if (unsupported>5) {
						/* found to many unsupported extensions - this can't be bd2 */
						printf("Data corrupt or detection failed for bd2 caves --\n too many unknown extensions.\n");
						return 0;
					}
					break;
			}
			j=memory[pos++];	/* read next */
		}
		out[outpos++]=j; /* closing 0xff */
		out[outpos++]=memory[pos++];	/* animation bits */

		/* color table */
		lo=cavecolors+i*2;
		hi=cavecolors+i*2+1;
		pos=memory[hi]*256L+memory[lo];	/* pointer to the three colors */
		out[outpos++]=memory[pos++];
		out[outpos++]=memory[pos++];
		out[outpos++]=memory[pos++];
		out[outpos++]=memory[pos++];	/* slime and amoeba color */
		out[outpos++]=memory[pos++];	/* background (and border) */
	}
	
	return 1;
}


/* save plck caves from memory map */
static int try_1stb()
{
	int i;

	strcpy((char *)out, s_1st);
	outpos=12;
	printf("\n*** Trying to interpret 1stb caves...\n");

	/* there must be 20 caves, according to bd inside faq. */
	for (i=0; i<20; i++) {
		int j;
		int pos=0x7010+0x400*i;

		/* 1stb caves have cave time, diamonds and the like encoded in bcd numbers,
		   one digit in each bytes. check that. */
		for (j=0x370; j<0x379+3; j++)
			if (memory[pos+j]>9) {
				printf("Data corrupt or detection failed for 1stb caves.\n");
				return 0;
			}
		printf("Cave %d, addr: %04x\n", i+1, pos);

		for (j=0; j<0x400; ++j)
			out[outpos++]=memory[pos++];
    }
	printf("Found %d 1stb caves!\n", i);
	return 1;
}


static int save(const char *outname)
{
	FILE *fout;
	int written;

	/* write data length in little endian */
	out[8]=((outpos-12))&0xff;
	out[9]=((outpos-12)>>8)&0xff;
	out[10]=((outpos-12)>>16)&0xff;
	out[11]=((outpos-12)>>24)&0xff;

	/* output */
	fout=fopen(outname, "wb");
	if (!fout) {
		printf("Cannot open outfile %s\n", outname);
		return 0;
	}
	written=fwrite(out, 1, outpos, fout);
	if (written!=outpos || fclose(fout)!=0) {
		printf("File write error to %s\n", outname);
		return 0;
	}

	printf("Saved to %s\n", outname);
	return 1;
}



int main(int argc, char* argv[])
{
	char outfilename[512];
	int result;

	if (argc<2 || argc>3) {
		printf("Converts memory dumps or vice snapshots of bd games\n");
		printf("to formats loadable by gdash.\n");
		printf("\n");
		printf("Usage: %s <inputfile> [outputfile]\n", argv[0]);
		printf("  where inputfile is a memory map or a vice snapshot.\n");
		printf("\n");
		printf("Use Alt-F11 or Alt-S in vice to create a snapshot file (*.vsf),\n");
		printf("or the monitor command s \"filename\" 0 0 ffff for memory contents.\n");
		printf("\n");
		printf("Beware that memory contents depend on actual memory pages set for cpu,\n");
		printf("so snapshot is better. Also load the game completely: wait until the\n");
		printf("usual scrolling screen appears, where you press f1 or fire.\n");
		printf("Proper extension is added to output file automatically.\n");
		printf("\n");
		printf("Engines supported: BD1, BD2, PLCK, 1stB, CrLi.\n");
		printf("Game-wide Diego effects are also supported, and converted to\n");
		printf("cave-specific effects. Unknown effects are reported, so check\n");
		printf("output!");
		
		return 0;
	}
	
	if (!loadfile(argv[1]))
		return -1;
	/* if second cmdline option, treat as output filename */
	if (argc==3)
		strcpy(outfilename, argv[2]);
	else {
		strcpy(outfilename, argv[1]);	/* use input file name, and change extension */

		/* output filename part */
		strcpy(outfilename, argv[1]);
		if (strchr(outfilename, '/') || strchr(outfilename, '\\')) {
			/* yes we have a pathname. */
			char *sep, *dot;
			sep=strrchr(outfilename, '/');
			if (!sep)	/* XXX should use #define for windows code */
				sep=strrchr(outfilename, '\\');
			dot=strrchr(outfilename, '.');
			if ((dot-sep) > 1)	/* dot later than last separator; >1 ensures that filename is not sg like .asd */
				*dot='\0';
		} else {
			/* we have no pathname */
			char *dot=strrchr(outfilename, '.');

			/* if we have extension, and filename is not only an extension :) */		
			if (dot || dot!=outfilename)
				*dot='\0';
		}
	}

	/* interpreting */
	strcat(outfilename, ".gds");
	if (try_plck()) {
		result=!save(outfilename);
	} else
	if (try_atari_plck()) {
		result=!save(outfilename);
	} else
	if (try_bd1(0)) {	/* try c64 bd1 */
		result=!save(outfilename);
	} else
	if (try_bd1(1)) {	/* try atari bd1 */
		result=!save(outfilename);
	} else
	if (try_bd2()) {	/* try c64 bd2 */
		result=!save(outfilename);
	} else
	if (try_bd2_atari()) {	/* try atari bd2 */
		result=!save(outfilename);
	} else
	if (try_crli()) {
		result=!save(outfilename);
	} else
	if (try_1stb()) {
		result=!save(outfilename);
	} else
	if (try_crdr()) {
		result=!save(outfilename);
	} else {
		printf("Could not export cave data -- unknown cave format!\n");
		result=-1;
	}
	
	return result;
}

/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
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
#ifndef _GD_C64IMPORT
#define _GD_C64IMPORT

#include "config.h"

#include <vector>
#include "cave/cavetypes.hpp"

// forward declaration
class CaveStored;


/// A pack of routines to import C64 data files created by any2gdash.
class C64Import {
public:
    // import tables for elements and characters in texts
    static const char bd_internal_character_encoding[];
    static GdElement const import_table_crli[];
    static GdElement const import_table_bd1[0x40];
    static GdElement const import_table_plck[0x10];
    static GdElement const import_table_1stb[0x80];
    static GdElement const import_table_crdr[0x100];

    /// File formats known to the c64 importer.
    enum GdCavefileFormat {
        GD_FORMAT_BD1,          ///<  boulder dash 1
        GD_FORMAT_BD1_ATARI,    ///<  boulder dash 1 atari version
        GD_FORMAT_BD2,          ///<  boulder dash 2 with rockford's extensions
        GD_FORMAT_BD2_ATARI,    ///<  boulder dash 2, atari version
        GD_FORMAT_PLC,          ///<  construction kit
        GD_FORMAT_PLC_ATARI,    ///<  construction kit, atari version
        GD_FORMAT_DLB,          ///<  no one's delight boulder dash
        GD_FORMAT_CRLI,         ///<  crazy light construction kit
        GD_FORMAT_CRDR_7,       ///<  crazy dream 7
        GD_FORMAT_FIRSTB,       ///<  first boulder
        
        GD_FORMAT_UNKNOWN,      ///<  unknown format
    };
    
    static char const *gd_format_strings[GD_FORMAT_UNKNOWN+1];

    enum ImportHack {
        None,
        Crazy_Dream_1,
        Crazy_Dream_7,
        Crazy_Dream_9,
        Deluxe_Caves_1,
        Deluxe_Caves_3,
    };

    // set some cave parameters for original c64 engines
    static void cave_set_bd1_defaults(CaveStored &cave);
    static void cave_set_bd2_defaults(CaveStored &cave);
    static void cave_set_plck_defaults(CaveStored &cave);
    static void cave_set_1stb_defaults(CaveStored &cave);
    static void cave_set_crdr_7_defaults(CaveStored &cave);
    static void cave_set_crli_defaults(CaveStored &cave);

    static void cave_set_engine_defaults(CaveStored &cave, GdEngineEnum engine);

    // import helper routines
    typedef GdElementEnum (*ImportFuncArray) (unsigned char const data[], unsigned i);
    typedef GdElementEnum (*ImportFuncByte) (unsigned char const c, unsigned i);

    static GdElementEnum bd1_import(unsigned char const data[], unsigned i);
    static GdElementEnum bd1_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum deluxecaves_1_import(unsigned char const data[], unsigned i);
    static GdElementEnum deluxecaves_1_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum deluxecaves_3_import(unsigned char const data[], unsigned i);
    static GdElementEnum deluxecaves_3_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum firstboulder_import(unsigned char const data[], unsigned i);
    static GdElementEnum firstboulder_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum crazylight_import(unsigned char const data[], unsigned i);
    static GdElementEnum crazylight_import_byte(unsigned char const c, unsigned i);

    static int slime_plck(unsigned c64_data);
    static GdString name_from_c64_bin(const unsigned char *data);
    
    // import routines
    static int cave_copy_from_bd1(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format, ImportHack hack);
    static int cave_copy_from_bd2(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format);
    static int cave_copy_from_plck(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format);
    static int cave_copy_from_dlb(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_1stb(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_crdr_7(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_crli(CaveStored &cave, const guint8 *data, int remaining_bytes);

    static GdCavefileFormat imported_get_format(const guint8 *buf);
    static std::vector<CaveStored *> caves_import_from_buffer(const guint8 *buf, int length);
};
#endif


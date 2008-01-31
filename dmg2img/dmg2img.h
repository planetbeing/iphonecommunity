/*
 *	DMG2IMG
 *	dmg2img.h
 *
 *	Copyright (c) 2004 vu1tur <to@vu1tur.eu.org>
 *	This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License 
 *	as published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <zlib.h>

#define PLIST_ADDRESS   0x1DC
#define PLIST_ADDRESS_C 0x124
#define PLIST_SIZE      0x11C

#define BT_ZLIB 0x80000005
#define BT_COPY 0x00000001
#define BT_ZERO 0x00000002
#define BT_UNKNOWN 0x7ffffffe
#define BT_END 0xffffffff

#ifndef __int32
#define __int32 int
#endif

z_stream z;

const char plist_begin[]="<plist version=\"1.0\">";
const char plist_end[]="</plist>";
const char list_begin[]="<array>";
const char list_end[]="</array>";
const char chunk_begin[]="<data>";
const char chunk_end[]="</data>";

unsigned __int32 convert_char4(unsigned char *c) {
    return (((__int32)c[0])<<24) | (((__int32)c[1])<<16) | 
      (((__int32)c[2])<<8) | ((__int32)c[3]);
}

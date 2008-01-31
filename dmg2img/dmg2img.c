/*
 *	DMG2IMG
 *	dmg2img.c
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

#define VERSION "dmg2img v0.3a is derived from dmg2iso by vu1tur (to@vu1tur.eu.org)"
#define USAGE "\
usage: dmg2img [-s] [-v] [-V] <input.dmg> [<output.img>]\n\
or     dmg2img -i <input.dmg> -o <output.img>"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dmg2img.h"
#include "base64.h"

void
mem_overflow()
{
    printf("ERROR: not enough memory\n");
    exit(-1);
}
  
int
main(int argc, char* argv[])
{
    int i, err, verbose=1, partnum, scb;
    Bytef *tmp, *otmp;
    unsigned long addr, addrc;
    unsigned int pl_size;
    unsigned char c[4];
    char *input_file=NULL, *output_file=NULL;
    char *zeroblock;
    char *plist;
    char **parts = NULL;
    char *data_begin, *data_end;
    unsigned int *partlen = NULL;
    unsigned int data_size;
    unsigned int offset;
    unsigned __int32 out_offs,out_size,in_offs,in_size;
    unsigned __int32 block_type;
    FILE *FIN, *FOUT;
    double percent;
    int skip;
   
    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-s"))
            verbose = 0;
        else
        if (!strcmp(argv[i], "-v"))
            verbose = 2;
        else
        if (!strcmp(argv[i], "-V"))
            verbose = 3;
        else
        if (!strcmp(argv[i], "-i") && i<argc-1)
	    input_file = argv[++i];
        else
        if (!strcmp(argv[i], "-o") && i<argc-1)
	    output_file = argv[++i];
        else
        if (!input_file)
	    input_file = argv[i];
        else
        if (!output_file)
	    output_file = argv[i];
    }
   
    if (!input_file) {
        printf("\n%s\n\n%s\n\n", VERSION, USAGE);
        return 0;
    }
    if (!output_file) {
        i = strlen(input_file);
        output_file = (char *)malloc(i+6);
        if (output_file) {
            strcpy(output_file, input_file);
            if (strcasecmp(&output_file[i-4], ".dmg"))
                strcat(output_file, ".img");
	    else
	        strcpy(&output_file[i-4], ".img");
	}
    }

    if (verbose) printf("\n%s\n\n", VERSION);

    FIN = fopen(input_file, "rb");
    if (FIN == NULL) {
        printf("ERROR: Can't open input file  %s\n", input_file);
        return 0;
    }
    if (output_file) 
        FOUT = fopen(output_file, "wb");
    else
        FOUT = NULL;
    if (FOUT == NULL) {
        printf("ERROR: Can't create output file  %s\n", output_file);
    }
   
    fseek(FIN, -PLIST_ADDRESS,SEEK_END);
    fread(c,4,1,FIN);
    addr = convert_char4(c);
    fseek(FIN, -PLIST_SIZE,SEEK_END);
    fread(c,4,1,FIN);
    pl_size = convert_char4(c);
    fseek(FIN, -PLIST_ADDRESS_C,SEEK_END);
    fread(c,4,1,FIN);
    /* address control - should be equal to addr */
    addrc = convert_char4(c);
    if (addr!=addrc) {
        printf("ERROR: dmg file is corrupted.\n");
        exit(-1);
    }

    if (verbose) {
        printf("%s --> %s\n\n", input_file, output_file);
	printf("reading property list, %u bytes from address %u ...\n", pl_size, addr);
    }

    plist = (char*)malloc(pl_size+1);
    tmp = (Bytef*)malloc(0x40000);
    otmp = (Bytef*)malloc(0x40000);

    if (!plist || !tmp || !otmp) mem_overflow();

    fseek(FIN, addr, SEEK_SET);
    fread(plist, pl_size, 1, FIN);
    plist[pl_size]='\0';

    if (!strstr(plist, plist_begin) ||
        !strstr(&plist[pl_size-20], plist_end)) {
    	printf("ERROR: Property list is corrupted.\n");
        exit(-1);
    }

    data_begin = plist;
    partnum = 0;
    scb = strlen(chunk_begin);
    while(1) {
        data_begin = strstr(data_begin, chunk_begin);
        if (!data_begin) break;
        data_begin += scb;
        data_end  = strstr(data_begin, chunk_end);
        if (!data_end) break;
        data_size = data_end-data_begin;
        i = partnum;
        ++partnum;
        parts = (char**)realloc(parts, partnum*sizeof(char*));
        partlen = (int *)realloc(partlen, partnum*sizeof(int));
        if (!parts || !partlen) mem_overflow();       
	parts[i] = (char*)malloc(data_size+1);
	if (!parts[i]) mem_overflow();
	parts[i][data_size] = '\0';       
        memcpy(parts[i], data_begin, data_size);
        if (verbose>=3) printf("%s\n", parts[i]);
        cleanup_base64(parts[i], data_size);
	decode_base64(parts[i],strlen(parts[i]), parts[i], &partlen[i]);
        if (verbose>=2)
	    printf("partition %d: begin=%d, size=%d, decoded=%d\n",i,
                   data_begin-plist, data_size, partlen[i]);
    }
    if (verbose) printf("\n");
   
    z.zalloc = (alloc_func)0;
    z.zfree = (free_func)0;
    z.opaque = (voidpf)0;
    if (verbose)
        printf("decompressing:\n");

    percent = 0.0;
    for (i = 0; i < partnum; i++) {
        skip = 0;

        if(i != 3)
            skip = 1;

        if (verbose)
	    printf("opening partition %d ...",i);
        if (verbose>=3)
	    printf("\n");
        else
        if (verbose)
	    printf("         ");
        offset = 0xcc;
        block_type = 0;
	while (block_type != 0xffffffff && offset<partlen[i]-40) {
	    if (verbose>=3)
	        printf("offset = %u  block_type = 0x%08x\n",offset,block_type);
            block_type = convert_char4(parts[i]+offset);
            out_offs = convert_char4(parts[i]+offset+12)*0x200;
            out_size = convert_char4(parts[i]+offset+20)*0x200;
            in_offs = convert_char4(parts[i]+offset+28);
            in_size = convert_char4(parts[i]+offset+36);
	    if (out_size!=0) {
               if (block_type == BT_ZLIB) {
 	           if (verbose>=3)
 	               printf("uncompress (in_addr=%u in_size=%u out_addr=%u out_size=%u)\n", in_offs, in_size, out_offs, out_size);
               	 if (inflateInit(&z) != Z_OK) {
                    	printf("ERROR: Can't initialize inflate stream\n");
                         return 0;
                    }
                    fseek(FIN,in_offs,SEEK_SET);
                    fread(tmp,in_size,1,FIN);
                    z.next_in = (Bytef*)tmp;
                    z.next_out = (Bytef*)otmp;
                    while (1) {
                    	z.avail_in = z.avail_out = 32768;
                        err = inflate(&z, Z_NO_FLUSH);
                        if (err != Z_OK && err != Z_STREAM_END) {
                            printf("ERROR: Inflation failed\n");
                            return 0;
                        }
                        if (err == Z_STREAM_END) break;
                    }
                    if (inflateEnd(&z) != Z_OK) { 
                    	printf("ERROR:\n");
                         return 0;
                    }
		    if(skip==0)
			    fwrite(otmp,out_size,1,FOUT);
                    percent = 100.0*(double)in_offs/(double)addr;
                    if (verbose>=3)
                        printf("%6.2f %%\n", percent);
                    else
		    if (verbose)
                        printf("\b\b\b\b\b\b\b\b%6.2f %%", percent);
               }
               else if (block_type == BT_COPY) {
               	    fseek(FIN,in_offs,SEEK_SET);
                    fread(tmp,in_size,1,FIN);
 	            if (verbose>=3)
 	               printf("copy data  (in_addr=%u in_size=%u out_size=%u)\n", in_offs, in_size, out_size);
		    if(skip==0)
                    	fwrite(tmp,out_size,1,FOUT);//copy
               }
               else if (block_type == BT_ZERO) {
               	    zeroblock = (char*)malloc(out_size);
		    if (!zeroblock) mem_overflow();
                    memset(zeroblock,0,out_size);
 	            if (verbose>=3)
 	               printf("null bytes (out_size=%u)\n", out_size);
		    if(skip==0)
                    	fwrite(zeroblock,out_size,1,FOUT);
                    free(zeroblock);
               }
               else if (block_type == BT_UNKNOWN) {
 	            if (verbose>=3)
 	               printf("0x%08x (in_addr=%u in_size=%u out_addr=%u out_size=%u)\n", block_type, in_offs, in_size, out_offs, out_size);
	       }
               else if (block_type == BT_END) {
 	            if (verbose>=3)
 	               printf("end block found\n");
               } 
	     } else {
 	            if (verbose>=3)
 	               printf("0x%08x (in_addr=%u in_size=%u out_addr=%u out_size=%u) ???\n", block_type, in_offs, in_size, out_offs, out_size);
	     }
             offset+=0x28;
	}
	if (verbose)
            printf("  ok\n");

	if (skip==0) {
            break;
        }
    }

    fclose(FOUT);

    if (verbose)
	printf("Archive successfully decompressed as  %s\n", output_file);
    free(tmp);
    free(otmp);
    for (i = 0; i < partnum; i++) free(parts[i]);
    free(parts);
    free(partlen);
    free(plist);
    fclose(FIN);

#if defined(__linux__)
    if (verbose) {
        printf("\n\
Linux users should be able to mount the archive [as root] by :\n\n\
modprobe hfsplus\n\
mount -t hfsplus -o loop %s /mnt\n\n", output_file);
    }
#endif

    return 0;
}

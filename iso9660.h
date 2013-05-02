/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

/**
 * Default ISO file contents, as obtained by mkisofs(8) utility
 */

// At offset 0x8000
const unsigned char ISO9660_PRIMARY_DESCRIPTOR[] = {
0x01,0x43,0x44,0x30,0x30,0x31,0x01,0x00,0x4c,0x49,0x4e,0x55,0x58,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x43,0x44,0x52,0x4f,0x4d,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xaf,0x00,0x00,0x00,0x00,0x00,0x00,0xaf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x01,0x00,0x00,0x01,
0x00,0x08,0x08,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x13,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x22,0x00,0x17,0x00,
0x00,0x00,0x00,0x00,0x00,0x17,0x00,0x08,0x00,0x00,0x00,0x00,0x08,0x00,0x70,0x0b,
0x06,0x0a,0x32,0x2c,0xec,0x02,0x00,0x00,0x01,0x00,0x00,0x01,0x01,0x00
};

// Some empty sector headers
const unsigned char ISO9660_AT_8800[] = { 0xff,0x43,0x44,0x30,0x30,0x31,0x01 };
const unsigned char ISO9660_AT_9800[] = { 0x01,0x00,0x17,0x00,0x00,0x00,0x01 };
const unsigned char ISO9660_AT_A800[] = { 0x01,0x00,0x00,0x00,0x00,0x17,0x00,0x01 };

// At offset 0xB800
// This record contains only one entry: CONTEXT.SH;1
// Actual file record starts at offset 0x44 (68)
const unsigned char ISO9660_CONTEXT_SH_STRUCT[] = {
0x22,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x00,0x17,0x00,0x08,0x00,0x00,0x00,0x00,
0x08,0x00,0x70,0x0b,0x06,0x0a,0x32,0x2c,0xec,0x02,0x00,0x00,0x01,0x00,0x00,0x01,
0x01,0x00,0x22,0x00,0x17,0x00,0x00,0x00,0x00,0x00,0x00,0x17,0x00,0x08,0x00,0x00,
0x00,0x00,0x08,0x00,0x70,0x0b,0x06,0x0a,0x35,0x32,0xec,0x02,0x00,0x00,0x01,0x00,
0x00,0x01,0x01,0x01,0x2e,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0xfe,0x01,
0x00,0x00,0x00,0x00,0x01,0xfe,0x70,0x0b,0x06,0x0a,0x32,0x2c,0xec,0x00,0x00,0x00,
0x01,0x00,0x00,0x01,0x0c,0x43,0x4f,0x4e,0x54,0x45,0x58,0x54,0x2e,0x53,0x48,0x3b,
0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/**
* -------------------------------------------------------------------------------------
 * The following parts are borrowed from the isofs/cd9660/iso.h from FreeBSD project
 * -------------------------------------------------------------------------------------
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)iso.h	8.6 (Berkeley) 5/10/95
 */

#define ISODCL(from, to) (to - from + 1)

struct iso_primary_descriptor {
	char type		        	[ISODCL (  1,	1)]; /* 711 */
	char id			        	[ISODCL (  2,	6)];
	char version		    	[ISODCL (  7,	7)]; /* 711 */
	char unused1		    	[ISODCL (  8,	8)];
	char system_id		    	[ISODCL (  9,  40)]; /* achars */
	char volume_id		    	[ISODCL ( 41,  72)]; /* dchars */
	char unused2		    	[ISODCL ( 73,  80)];
	char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
	char unused3			    [ISODCL ( 89, 120)];
	char volume_set_size		[ISODCL (121, 124)]; /* 723 */
	char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	char logical_block_size		[ISODCL (129, 132)]; /* 723 */
	char path_table_size		[ISODCL (133, 140)]; /* 733 */
	char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
	char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
	char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
	char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
	char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
	char volume_set_id		    [ISODCL (191, 318)]; /* dchars */
	char publisher_id		    [ISODCL (319, 446)]; /* achars */
	char preparer_id		    [ISODCL (447, 574)]; /* achars */
	char application_id		    [ISODCL (575, 702)]; /* achars */
	char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
	char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
	char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
	char creation_date		    [ISODCL (814, 830)]; /* 8.4.26.1 */
	char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
	char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
	char effective_date		    [ISODCL (865, 881)]; /* 8.4.26.1 */
	char file_structure_version	[ISODCL (882, 882)]; /* 711 */
	char unused4			    [ISODCL (883, 883)];
	char application_data		[ISODCL (884, 1395)];
	char unused5			    [ISODCL (1396, 2048)];
};

struct iso_directory_record {
	char length			        [ISODCL (1, 1)]; /* 711 */
	char ext_attr_length	    [ISODCL (2, 2)]; /* 711 */
	unsigned char extent		[ISODCL (3, 10)]; /* 733 */
	unsigned char size			[ISODCL (11, 18)]; /* 733 */
	char date			        [ISODCL (19, 25)]; /* 7 by 711 */
	char flags			        [ISODCL (26, 26)];
	char file_unit_size		    [ISODCL (27, 27)]; /* 711 */
	char interleave			    [ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	char name_len			    [ISODCL (33, 33)]; /* 711 */
    char name                   [12]; /* XXX */
};
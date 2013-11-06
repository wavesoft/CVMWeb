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

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include "iso9660.h"
#include "CrashReport.h"

// CD-ROM size for 2KiB of data
static const int CONTEXTISO_CDROM_SIZE = 358400;
const char * LIBCONTEXTISO_APP = "LIBCONTEXTISO - A TINY ISO 9660-COMPATIBLE FILESYSTEM CREATOR LIBRARY (C) 2012  I.CHARALAMPIDIS                                 ";

/**
 * Hard-coded offsets
 */
static const int PRIMARY_DESCRIPTOR_OFFSET          = 0x8000;
static const int SECONDARY_DIRECTORY_RECORD_OFFSET  = 0xB800;
static const int CONTENTS_OFFSET                    = 0xC000;

/**
 * Some locally-used constants
 */
const char dateZero[] = { 0x30,0x30,0x30,0x30, 0x30,0x30, 0x30,0x30, 0x30,0x30, 0x30,0x30, 0x30,0x30, 0x30,0x30, 0x30 };

/**
 * Update a long in ISO9660 pivot-endian representation
 */
void isosetl( int x, unsigned char buffer[]) {
    buffer[0] =  x        & 0xFF;
    buffer[1] = (x >>  8) & 0xFF;
    buffer[2] = (x >> 16) & 0xFF;
    buffer[3] = (x >> 24) & 0xFF;
    buffer[4] = (x >> 24) & 0xFF;
    buffer[5] = (x >> 16) & 0xFF;
    buffer[6] = (x >>  8) & 0xFF;
    buffer[7] =  x        & 0xFF;
}

/**
 * Generate a CD-ROM ISO buffer compatible with the ISO9660 (CDFS) filesystem
 * (Using reference from: http://users.telenet.be/it3.consultants.bvba/handouts/ISO9960.html)
 */
char * build_simple_cdrom( const char * volume_id, const char * filename, const char * buffer, int size ) {
    CRASH_REPORT_BEGIN;

    // Local variables
    iso_primary_descriptor  descPrimary;
    iso_directory_record    descFile;
    time_t rawTimeNow;
    struct tm * tmNow;
    
    // Prepare primary record
    memset(&descPrimary, 0, sizeof(iso_primary_descriptor));
#pragma warning(suppress: 6386)
    memset(&descPrimary.volume_set_id[0], 0x20, 1205); // Reaches till .unused5
    memset(&descPrimary.file_structure_version[0], 1, 1);
    memset(&descPrimary.unused4[0], 0, 1);
    
    // Copy defaults to the primary sector descriptor
    memcpy(&descPrimary, ISO9660_PRIMARY_DESCRIPTOR, ISO9660_PRIMARY_DESCRIPTOR_SIZE);
    
    // Build the current date
    static char dateNow[18];
    time(&rawTimeNow);
    tmNow = gmtime(&rawTimeNow);
    sprintf(&dateNow[0], "%04u%02u%02u%02u%02u%02u000", // <-- Last 0x30 ('0') is GMT Timezone
            tmNow->tm_year + 1900,
            tmNow->tm_mon,
            tmNow->tm_mday,
            tmNow->tm_hour,
            tmNow->tm_min,
            tmNow->tm_sec
        );
        
    // Set date fields
    memcpy(&descPrimary.creation_date[0], dateNow, 17);
    memcpy(&descPrimary.modification_date[0], dateNow, 17);
    memcpy(&descPrimary.effective_date[0], dateNow, 17);
    memcpy(&descPrimary.expiration_date[0], dateZero, 17);
    memcpy(&descPrimary.application_id, LIBCONTEXTISO_APP, 127);
    
    // Update volume_id on the primary sector
    int lVol = strlen(volume_id);
    if (lVol > 31) lVol=31;
    memset(&descPrimary.volume_id, 0x20, 31);
    memcpy(&descPrimary.volume_id, volume_id, lVol);

    // Limit the data size to the size of the 350Kb CD-ROM buffer (that's 302Kb of data)
    int lDataSize = size;
    if (lDataSize > (CONTEXTISO_CDROM_SIZE - CONTENTS_OFFSET))
        lDataSize = CONTEXTISO_CDROM_SIZE - CONTENTS_OFFSET;
    
    // Calculate the volume size
    int lVolSize = 1 + ((lDataSize - 1) / 2048);
    isosetl(lVolSize, (unsigned char *)descPrimary.volume_space_size);
    
    // Copy defaults to CONTEXT.SH record
    memcpy(&descFile, &ISO9660_CONTEXT_SH_STRUCT[0x44], sizeof(iso_directory_record));
    
    // Update CONTEXT.SH record
    isosetl(lDataSize, descFile.size);      // <-- File size
    size_t i=0; for (;i<strlen(filename);i++) {    // <-- File name
        if (i>=10) break;
        char c = toupper(filename[i]);
        if (c==' ') c='_';
        descFile.name[i]=c;
    }
    descFile.name[i]=';';  // Add file revision (;1)
    descFile.name[i+1]='1';
    
    // Compose the CD-ROM Disk buffer
    static char bytes[CONTEXTISO_CDROM_SIZE];
    memset(&bytes,0,CONTEXTISO_CDROM_SIZE);
#pragma warning(suppress: 6385)
    memcpy(&bytes[PRIMARY_DESCRIPTOR_OFFSET],           &descPrimary,               sizeof(iso_primary_descriptor));
    memcpy(&bytes[0x8800],                              &ISO9660_AT_8800,           ISO9660_AT_8800_SIZE);
    memcpy(&bytes[0x9800],                              &ISO9660_AT_9800,           ISO9660_AT_9800_SIZE);
    memcpy(&bytes[0xA800],                              &ISO9660_AT_A800,           ISO9660_AT_A800_SIZE);
    memcpy(&bytes[SECONDARY_DIRECTORY_RECORD_OFFSET],   &ISO9660_CONTEXT_SH_STRUCT, ISO9660_CONTEXT_SH_STRUCT_SIZE);
    memcpy(&bytes[SECONDARY_DIRECTORY_RECORD_OFFSET+68],&descFile,                  sizeof(iso_directory_record));
    memcpy(&bytes[CONTENTS_OFFSET],                     buffer,                     lDataSize);
    
    // Return the built buffer
    return bytes;

    CRASH_REPORT_END;
}

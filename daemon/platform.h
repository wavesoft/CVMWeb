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

// VS: MMVJ9-FKY74-W449Y-RB79G-8GJGJ 

#ifndef PLATFORM_H_X4S3J8HJ
#define PLATFORM_H_X4S3J8HJ

#define USAGE_IDLE      0   // The resource is completely free for our use
#define USAGE_LOW       1   // The resource is used only in a fraction, we can still use it
#define USAGE_REGULAR   2   // The resource is in regular use, we might be able to use a fraction of it
#define USAGE_HIGH      3   // The resource is completely in use, we are forbidden to touch it

/* Structure that holds the platform resource status */
typedef struct {
    
    unsigned char       cpu;
    unsigned char       ram;
    unsigned char       network;
    
} PLAF_USAGE;

/* Resource fetching */
int                     platformMeasureResources( PLAF_USAGE * usage );

#endif /* end of include guard: PLATFORM_H_X4S3J8HJ */

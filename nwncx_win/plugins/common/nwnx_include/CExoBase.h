/*
 *  NWNeXalt - Empty File
 *  (c) 2007 Doug Swarin (zac@intertex.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 *  $HeadURL$
 *
 */

#ifndef _NX_NWN_STRUCT_CEXOBASE_
#define _NX_NWN_STRUCT_CEXOBASE_

struct CExoBase {
    void            *exo_ini;                /* 0000 */
	void         *exo_timers;             /* 0004 */
	void          *exo_debug;              /* 0008 */
	void      *exo_aliases;            /* 000C */
	void           *exo_rand;               /* 0010 */
    uint32_t           exp_pack;				/* 0x14 */
	void   *exo_internal;           /* 0018 */
};

#endif /* _NX_NWN_STRUCT_CEXOBASE_ */

/* vim: set sw=4: */

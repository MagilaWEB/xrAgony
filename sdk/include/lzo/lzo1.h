/* lzo1.h -- public interface of the LZO1 compression algorithm

	This file is part of the LZO real-time data compression library.

	Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
	All Rights Reserved.

	The LZO library is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	The LZO library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the LZO library; see the file COPYING.
	If not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

	Markus F.X.J. Oberhumer
	<markus@oberhumer.com>
	http://www.oberhumer.com/opensource/lzo/
 */


#ifndef __LZO1_H_INCLUDED
#define __LZO1_H_INCLUDED 1

#ifndef __LZOCONF_H_INCLUDED
#include <lzo/lzoconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

/* Memory required for the wrkmem parameter.
 * When the required size is 0, you can also pass a NULL pointer.
 */

#define LZO1_MEM_COMPRESS		((lzo_uint32_t) (8192L * lzo_sizeof_dict_t))
#define LZO1_MEM_DECOMPRESS	 (0)


LZO_EXTERN(int)
lzo1_compress			( const lzo_bytep src, lzo_uint  src_len,
								lzo_bytep dst, lzo_uintp dst_len,
								lzo_voidp wrkmem );

LZO_EXTERN(int)
lzo1_decompress		 ( const lzo_bytep src, lzo_uint  src_len,
								lzo_bytep dst, lzo_uintp dst_len,
								lzo_voidp wrkmem /* NOT USED */ );


/***********************************************************************
// better compression ratio at the cost of more memory and time
************************************************************************/

#define LZO1_99_MEM_COMPRESS	((lzo_uint32_t) (65536L * lzo_sizeof_dict_t))

LZO_EXTERN(int)
lzo1_99_compress		( const lzo_bytep src, lzo_uint  src_len,
								lzo_bytep dst, lzo_uintp dst_len,
								lzo_voidp wrkmem );



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 sw=4 et: */

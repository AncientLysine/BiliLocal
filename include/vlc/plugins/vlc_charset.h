/*****************************************************************************
 * charset.h: Unicode UTF-8 wrappers function
 *****************************************************************************
 * Copyright (C) 2003-2005 VLC authors and VideoLAN
 * Copyright © 2005-2010 Rémi Denis-Courmont
 * $Id: cd4d40725344eda07f51dd16f11aeadc351141e8 $
 *
 * Author: Rémi Denis-Courmont <rem # videolan,org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_CHARSET_H
#define VLC_CHARSET_H 1

/**
 * \file
 * This files handles locale conversions in vlc
 */

/* iconv wrappers (defined in src/extras/libc.c) */
typedef void *vlc_iconv_t;
VLC_API vlc_iconv_t vlc_iconv_open( const char *, const char * ) VLC_USED;
VLC_API size_t vlc_iconv( vlc_iconv_t, const char **, size_t *, char **, size_t * ) VLC_USED;
VLC_API int vlc_iconv_close( vlc_iconv_t );

#include <stdarg.h>

VLC_API void LocaleFree( const char * );
VLC_API char * FromLocale( const char * ) VLC_USED;
VLC_API char * FromLocaleDup( const char * ) VLC_USED;
VLC_API char * ToLocale( const char * ) VLC_USED;
VLC_API char * ToLocaleDup( const char * ) VLC_USED;

VLC_API int utf8_vfprintf( FILE *stream, const char *fmt, va_list ap );
VLC_API int utf8_fprintf( FILE *, const char *, ... ) VLC_FORMAT( 2, 3 );
VLC_API char * vlc_strcasestr(const char *, const char *) VLC_USED;

VLC_API char * EnsureUTF8( char * );
VLC_API const char * IsUTF8( const char * ) VLC_USED;

#ifdef WIN32
VLC_USED
static inline char *FromWide (const wchar_t *wide)
{
    size_t len = WideCharToMultiByte (CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (len == 0)
        return NULL;

    char *out = (char *)malloc (len);

    if (likely(out))
        WideCharToMultiByte (CP_UTF8, 0, wide, -1, out, len, NULL, NULL);
    return out;
}

VLC_USED
static inline wchar_t *ToWide (const char *utf8)
{
    int len = MultiByteToWideChar (CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len == 0)
        return NULL;

    wchar_t *out = (wchar_t *)malloc (len * sizeof (wchar_t));

    if (likely(out))
        MultiByteToWideChar (CP_UTF8, 0, utf8, -1, out, len);
    return out;
}
#endif

/**
 * Converts a nul-terminated string from ISO-8859-1 to UTF-8.
 */
static inline char *FromLatin1 (const char *latin)
{
    char *str = (char *)malloc (2 * strlen (latin) + 1), *utf8 = str;
    unsigned char c;

    if (str == NULL)
        return NULL;

    while ((c = *(latin++)) != '\0')
    {
         if (c >= 0x80)
         {
             *(utf8++) = 0xC0 | (c >> 6);
             *(utf8++) = 0x80 | (c & 0x3F);
         }
         else
             *(utf8++) = c;
    }
    *(utf8++) = '\0';

    utf8 = (char *)realloc (str, utf8 - str);
    return utf8 ? utf8 : str;
}

VLC_API char * FromCharset( const char *charset, const void *data, size_t data_size ) VLC_USED;
VLC_API void * ToCharset( const char *charset, const char *in, size_t *outsize ) VLC_USED;

VLC_API double us_strtod( const char *, char ** ) VLC_USED;
VLC_API float us_strtof( const char *, char ** ) VLC_USED;
VLC_API double us_atof( const char * ) VLC_USED;
VLC_API int us_vasprintf( char **, const char *, va_list );
VLC_API int us_asprintf( char **, const char *, ... ) VLC_USED;

#endif

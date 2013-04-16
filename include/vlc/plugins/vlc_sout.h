/*****************************************************************************
 * stream_output.h : stream output module
 *****************************************************************************
 * Copyright (C) 2002-2008 VLC authors and VideoLAN
 * $Id: 8c2e3428113a4900fc9ec197027fa808ff5b267f $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Eric Petit <titer@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x.nl>
 *          Rémi Denis-Courmont
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

#ifndef VLC_SOUT_H_
#define VLC_SOUT_H_

/**
 * \file
 * This file defines structures and functions for stream output in vlc
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <vlc_es.h>

/** Stream output instance */
struct sout_instance_t
{
    VLC_COMMON_MEMBERS

    char *psz_sout;

    /* meta data (Read only) XXX it won't be set before the first packet received */
    vlc_meta_t          *p_meta;

    /** count of output that can't control the space */
    int                 i_out_pace_nocontrol;

    vlc_mutex_t         lock;
    sout_stream_t       *p_stream;

    /** Private */
    sout_instance_sys_t *p_sys;
};

/****************************************************************************
 * sout_stream_id_t: opaque (private for all sout_stream_t)
 ****************************************************************************/
typedef struct sout_stream_id_t  sout_stream_id_t;

/** Stream output access_output */
struct sout_access_out_t
{
    VLC_COMMON_MEMBERS

    module_t                *p_module;
    char                    *psz_access;

    int                      i_writes;
    /** Local counter reset each time it is transferred to stats */
    int64_t                  i_sent_bytes;

    char                    *psz_path;
    sout_access_out_sys_t   *p_sys;
    int                     (*pf_seek)( sout_access_out_t *, off_t );
    ssize_t                 (*pf_read)( sout_access_out_t *, block_t * );
    ssize_t                 (*pf_write)( sout_access_out_t *, block_t * );
    int                     (*pf_control)( sout_access_out_t *, int, va_list );

    config_chain_t          *p_cfg;
};

enum access_out_query_e
{
    ACCESS_OUT_CONTROLS_PACE, /* arg1=bool *, can fail (assume true) */
};

VLC_API sout_access_out_t * sout_AccessOutNew( vlc_object_t *, const char *psz_access, const char *psz_name ) VLC_USED;
#define sout_AccessOutNew( obj, access, name ) \
        sout_AccessOutNew( VLC_OBJECT(obj), access, name )
VLC_API void sout_AccessOutDelete( sout_access_out_t * );
VLC_API int sout_AccessOutSeek( sout_access_out_t *, off_t );
VLC_API ssize_t sout_AccessOutRead( sout_access_out_t *, block_t * );
VLC_API ssize_t sout_AccessOutWrite( sout_access_out_t *, block_t * );
VLC_API int sout_AccessOutControl( sout_access_out_t *, int, ... );

static inline bool sout_AccessOutCanControlPace( sout_access_out_t *p_ao )
{
    bool b;
    if( sout_AccessOutControl( p_ao, ACCESS_OUT_CONTROLS_PACE, &b ) )
        return true;
    return b;
}

/** Muxer structure */
struct  sout_mux_t
{
    VLC_COMMON_MEMBERS
    module_t            *p_module;

    sout_instance_t     *p_sout;

    char                *psz_mux;
    config_chain_t          *p_cfg;

    sout_access_out_t   *p_access;

    int                 (*pf_addstream)( sout_mux_t *, sout_input_t * );
    int                 (*pf_delstream)( sout_mux_t *, sout_input_t * );
    int                 (*pf_mux)      ( sout_mux_t * );
    int                 (*pf_control)  ( sout_mux_t *, int, va_list );

    /* here are all inputs accepted by muxer */
    int                 i_nb_inputs;
    sout_input_t        **pp_inputs;

    /* mux private */
    sout_mux_sys_t      *p_sys;

    /* XXX private to stream_output.c */
    /* if muxer doesn't support adding stream at any time then we first wait
     *  for stream then we refuse all stream and start muxing */
    bool  b_add_stream_any_time;
    bool  b_waiting_stream;
    /* we wait one second after first stream added */
    mtime_t     i_add_stream_start;
};

enum sout_mux_query_e
{
    /* capabilities */
    MUX_CAN_ADD_STREAM_WHILE_MUXING,    /* arg1= bool *,      res=cannot fail */
    /* properties */
    MUX_GET_ADD_STREAM_WAIT,            /* arg1= bool *,      res=cannot fail */
    MUX_GET_MIME,                       /* arg1= char **            res=can fail    */
};

struct sout_input_t
{
    sout_instance_t *p_sout;

    es_format_t     *p_fmt;
    block_fifo_t    *p_fifo;

    void            *p_sys;
};


VLC_API sout_mux_t * sout_MuxNew( sout_instance_t*, const char *, sout_access_out_t * ) VLC_USED;
VLC_API sout_input_t * sout_MuxAddStream( sout_mux_t *, es_format_t * ) VLC_USED;
VLC_API void sout_MuxDeleteStream( sout_mux_t *, sout_input_t * );
VLC_API void sout_MuxDelete( sout_mux_t * );
VLC_API void sout_MuxSendBuffer( sout_mux_t *, sout_input_t  *, block_t * );
VLC_API int sout_MuxGetStream(sout_mux_t *, int , mtime_t *);

static inline int sout_MuxControl( sout_mux_t *p_mux, int i_query, ... )
{
    va_list args;
    int     i_result;

    va_start( args, i_query );
    i_result = p_mux->pf_control( p_mux, i_query, args );
    va_end( args );
    return i_result;
}

/****************************************************************************
 * sout_stream:
 ****************************************************************************/
struct sout_stream_t
{
    VLC_COMMON_MEMBERS

    module_t          *p_module;
    sout_instance_t   *p_sout;

    char              *psz_name;
    config_chain_t        *p_cfg;
    sout_stream_t     *p_next;

    /* Subpicture unit */
    spu_t             *p_spu;

    /* add, remove a stream */
    sout_stream_id_t *(*pf_add)( sout_stream_t *, es_format_t * );
    int               (*pf_del)( sout_stream_t *, sout_stream_id_t * );
    /* manage a packet */
    int               (*pf_send)( sout_stream_t *, sout_stream_id_t *, block_t* );

    /* private */
    sout_stream_sys_t *p_sys;
};

VLC_API void sout_StreamChainDelete(sout_stream_t *p_first, sout_stream_t *p_last );
VLC_API sout_stream_t *sout_StreamChainNew(sout_instance_t *p_sout,
        char *psz_chain, sout_stream_t *p_next, sout_stream_t **p_last) VLC_USED;

static inline sout_stream_id_t *sout_StreamIdAdd( sout_stream_t *s, es_format_t *fmt )
{
    return s->pf_add( s, fmt );
}
static inline int sout_StreamIdDel( sout_stream_t *s, sout_stream_id_t *id )
{
    return s->pf_del( s, id );
}
static inline int sout_StreamIdSend( sout_stream_t *s, sout_stream_id_t *id, block_t *b )
{
    return s->pf_send( s, id, b );
}

/****************************************************************************
 * Encoder
 ****************************************************************************/

VLC_API encoder_t * sout_EncoderCreate( vlc_object_t *obj );
#define sout_EncoderCreate(o) sout_EncoderCreate(VLC_OBJECT(o))

/****************************************************************************
 * Announce handler
 ****************************************************************************/
VLC_API session_descriptor_t* sout_AnnounceRegisterSDP( vlc_object_t *, const char *, const char * ) VLC_USED;
VLC_API int sout_AnnounceUnRegister(vlc_object_t *,session_descriptor_t* );
#define sout_AnnounceRegisterSDP(o, sdp, addr) \
        sout_AnnounceRegisterSDP(VLC_OBJECT (o), sdp, addr)
#define sout_AnnounceUnRegister(o, a) \
        sout_AnnounceUnRegister(VLC_OBJECT (o), a)

/** SDP */

struct sockaddr;

VLC_API char * vlc_sdp_Start( vlc_object_t *obj, const char *cfgpref, const struct sockaddr *src, size_t srclen, const struct sockaddr *addr, size_t addrlen ) VLC_USED;
VLC_API char * sdp_AddMedia(char **sdp, const char *type, const char *protocol, int dport, unsigned pt, bool bw_indep, unsigned bw, const char *ptname, unsigned clockrate, unsigned channels, const char *fmtp);
VLC_API char * sdp_AddAttribute(char **sdp, const char *name, const char *fmt, ...) VLC_FORMAT( 3, 4 );

/** Description module */
typedef struct sout_description_data_t
{
    int i_es;
    es_format_t **es;
    vlc_sem_t *sem;
} sout_description_data_t;

#ifdef __cplusplus
}
#endif

#endif

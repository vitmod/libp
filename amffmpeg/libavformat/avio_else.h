#ifndef AVIO_ELSE_HEADER_
#define AVIO_ELSE_HEADER_

#define AVIOContext ByteIOContext

#define avio_write put_buffer



#define avio_w8 	put_byte
#define avio_wb16 	put_be16
#define avio_wb32 	put_be32
#define avio_read   get_buffer

#define avio_r8		get_byte

#define avio_rl6	get_le16
#define avio_rb16	get_be16

#define avio_rl24	get_le24
#define avio_rb24	get_be24

#define avio_rl32   get_le32
#define avio_rb32	get_be32




#define avio_skip   url_fskip
#define avio_seek	url_fseek

#define avio_open_dyn_buf 	url_open_dyn_buf 
#define avio_flush 			flush_buffer 
#define avio_close_dyn_buf 	url_close_dyn_buf
#define avio_open_dyn_buf 	url_open_dyn_buf 

#ifdef DEBUG
#define av_dlog(ic,fmt...)	av_log(ic, AV_LOG_DEBUG,##fmt);
#else
#define av_dlog(ic,fmt...)

#endif
#define ffurl_write 		url_write
#define ffurl_read			url_read



#define avio_close			url_fclose

#define ffurl_open			url_open
#define ffurl_close			url_close
#define ffurl_get_file_handle	url_get_file_handle			


#define avio_tell(s)			url_ftell(s)


#define av_url_split 		url_split

#define ffurl_read_complete url_read_complete

#define av_dict_set(pm,key,value,flags) av_metadata_set(pm,key,value) 
#define av_dict_copy(m1,m2,flags) 					av_metadata_copy(m1,m2,flags) 




#define ff_mpegts_parse_packet(ts,pkt,buf,len) mpegts_parse_packet(ts,pkt,buf,len)
#define ff_mpegts_parse_open(s)				   mpegts_parse_open(s)
#define ff_mpegts_parse_close(s)			   mpegts_parse_close(s)


#define ffio_init_context(s,buffer,buffer_size,write_flag,opaque,read_packet,write_packet,seek)\
				init_put_byte(s,buffer,buffer_size,write_flag,opaque,read_packet,write_packet,seek)

#define avio_alloc_context(buffer,buffer_size,write_flag,opaque,read_packet,write_packet,seek)\
	av_alloc_put_byte(buffer,buffer_size,write_flag,opaque,read_packet,write_packet,seek)

#define  ff_udp_get_local_port(h) 			udp_get_local_port(h)
#define  ff_udp_set_remote_url(h,url)       udp_set_remote_url(h,url)

int ff_hex_to_data(uint8_t *data, const char *p);




#define MAX_URL_SIZE 4096

#endif


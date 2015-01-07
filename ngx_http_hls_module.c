#include<ngx_core.h>
#include<ngx_config.h>
#include<ngx_http.h>
#include <stdio.h>
typedef struct{
	ngx_str_t media_type;
	ngx_str_t media_path;
} ngx_http_hls_loc_conf_t;

//static u_char filename[1024];

static ngx_int_t ngx_hls_handler(ngx_http_request_t *r);
static char* ngx_http_hls(ngx_conf_t *cf,ngx_command_t *cmd,void *conf);
static void* ngx_http_hls_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_hls_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static char* ngx_http_hls_conf(ngx_conf_t *cf,ngx_command_t *cmd, void *conf);



static char* ngx_http_hls_conf(ngx_conf_t *cf,ngx_command_t *cmd, void *conf){
	ngx_http_core_loc_conf_t *clcf;
	clcf= ngx_http_conf_get_module_loc_conf(cf,ngx_http_core_module);
	ngx_conf_set_str_slot(cf,cmd,conf);
	return NGX_CONF_OK;
}


//static char* ngx_http_nameserver_conf_3(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char* ngx_http_hls_convert_string(ngx_str_t str)
{
	char *string = malloc(str.len + 1);
	memcpy(string,str.data,str.len);
	string[str.len] = '\0' ;
	return string;
}

static ngx_command_t ngx_http_hls_commands[] ={ 
	{ngx_string("hls"),
	 NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
	 ngx_http_hls,
	 0,
	 0,
	 NULL		
	},
	{
		ngx_string("media_type"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_hls_conf,	
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_hls_loc_conf_t,media_type),
		NULL
	},
	{
		ngx_string("media_path"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_hls_conf,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_hls_loc_conf_t,media_path),
		NULL
	},

	ngx_null_command
};
ngx_http_module_t ngx_http_hls_module_ctx={

	NULL,
	NULL,

	NULL,
	NULL,

	NULL,
	NULL,

	ngx_http_hls_create_loc_conf,
	ngx_http_hls_merge_loc_conf,
};

ngx_module_t ngx_http_hls_module={

	NGX_MODULE_V1,
	&ngx_http_hls_module_ctx,
	ngx_http_hls_commands,
	NGX_HTTP_MODULE,

	NULL,
	NULL,

	NULL,
	NULL,

	NULL,
	NULL,

	NULL,
	NGX_MODULE_V1_PADDING
};
static char* ngx_http_hls(ngx_conf_t *cf,ngx_command_t *cmd,void *conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = ngx_http_conf_get_module_loc_conf(cf,ngx_http_core_module);
	clcf->handler = ngx_hls_handler;
	return NGX_CONF_OK;
}

static ngx_int_t ngx_hls_handler(ngx_http_request_t *r)
{
    //ngx_log_t *log = r->connection->log;
	
	if(!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}
	ngx_http_hls_loc_conf_t *elcf = ngx_http_get_module_loc_conf(r,ngx_http_hls_module);

	char* media_type = ngx_http_hls_convert_string(elcf->media_type);
	char* media_path = ngx_http_hls_convert_string(elcf->media_path);
	//char *media_type = (char*)elcf->media_type.data;
	//char *media_path = (char*)elcf->media_path.data;
	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}

	if(strcmp(media_type,"live") == 0){
		//TODO:Live media.
	}
	else{
		ngx_str_t media_file;
		ngx_str_t media_start;
		ngx_str_t media_end;
		ngx_http_arg(r, (u_char *)"media", 5, &media_file);

		rc = ngx_http_arg(r,(u_char *)"start",5,&media_start);

		ngx_http_arg(r, (u_char *)"end", 3 , &media_end);

		if(rc == NGX_DECLINED || media_start.len == 0){
			char filename[512];
			bzero(filename,512);
			ngx_str_t type = ngx_string("application/x-mpegURL");//m3u8文件的content type
			ngx_snprintf((u_char*)filename,1024,"%s%V.m3u8",media_path,&media_file);
			
			//M3U8文件不存在，目前采用直接返回错误的方式，后期采用根据TS文件创建新的M3U8
			if(access((char*)filename,F_OK) == -1){
				ngx_log_error(NGX_LOG_CRIT,r->connection->log,0,"File[%s] is not exist!media_path = %s,media_file=%V",filename,media_path,&media_file);
				//r->headers_out.status = NGX_HTTP_NOT_FOUND;
				//rc = ngx_http_send_header(r);
				return NGX_HTTP_NOT_FOUND;	
			}
			else{
				ngx_chain_t *out = ngx_pcalloc(r->pool,sizeof(ngx_chain_t));
				ngx_buf_t *b;
				//b = ngx_create_temp_buf(r->pool,sizeof(ngx_buf_t));
				b = ngx_pcalloc(r->pool,sizeof(ngx_buf_t));
				if(b == NULL){
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				b->in_file = 1;
				b->file = ngx_pcalloc(r->pool,sizeof(ngx_file_t));
				b->file->fd = ngx_open_file(filename,NGX_FILE_RDONLY|NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
				if(b->file->fd == -1){
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				b->file->name.data = (u_char*)filename;
				b->file->name.len = sizeof(filename) -1;
				if(ngx_file_info(filename,&b->file->info) == NGX_FILE_ERROR){
					return NGX_HTTP_INTERNAL_SERVER_ERROR;
				}
				
				r->headers_out.status = NGX_HTTP_OK;
				r->headers_out.content_length_n = b->file->info.st_size;
				r->headers_out.content_type = type;
				b->file_pos = 0;
				b->file_last = b->file->info.st_size;
				b->last_in_chain = 1;
				b->file->log = r->connection->log;
				b->last_buf = 1;
				

				//清理文件句柄
				/*
				ngx_pool_cleanup_t *cln = ngx_pool_cleanup_add(r->pool,sizeof(ngx_pool_cleanup_file_t));
				if(cln == NULL){
					return NGX_ERROR;
				}
				cln->handler = ngx_pool_cleanup_file;
				ngx_pool_cleanup_file_t *clnf = cln->data;
				clnf->fd = b->file->fd;
				clnf->name = b->file->name.data;
				clnf->log = r->pool->log;
				*/
				
				out->buf = b;
				out->next = NULL;
				rc = ngx_http_send_header(r);
				if(rc == NGX_ERROR || rc > NGX_OK || r->header_only)
				{
					return rc;
				}
				
				return ngx_http_output_filter(r,out);
			
			}
		}
		else{
			ngx_str_t type = ngx_string("video/mp2t");	//ts文件的content type
			//ngx_str_t type = ngx_string("application/octet-stream");
			char filename[512];
			bzero(filename,512);
			ngx_snprintf((u_char*)filename,512,"%s%V.ts",media_path,&media_file);
			ngx_log_error(NGX_LOG_CRIT,r->connection->log,0,"File[%s] is not exist!media_path = %s,media_file=%V",filename,media_path,&media_file);
			r->allow_ranges = 1; //开启range选项
			char *start = ngx_http_hls_convert_string(media_start);
			char *end = ngx_http_hls_convert_string(media_end);
			ngx_log_error(NGX_LOG_CRIT,r->connection->log,0,"start=%s;end=%s",start,end);
			ngx_chain_t *out = ngx_pcalloc(r->pool,sizeof(ngx_chain_t));
			ngx_buf_t *b = ngx_pcalloc(r->pool,sizeof(ngx_buf_t));
			if(b == NULL){
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			b->in_file = 1;
			b->file = ngx_pcalloc(r->pool,sizeof(ngx_file_t));
			b->file->fd = ngx_open_file(filename,NGX_FILE_RDONLY|NGX_FILE_NONBLOCK, NGX_FILE_OPEN,0);
			if(b->file->fd == -1){
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			b->file->name.data = (u_char*)filename;
			b->file->name.len = sizeof(filename)-1;
		
			r->headers_out.status = NGX_HTTP_OK;
			r->headers_out.content_length_n = atol(end)-atol(start);
			 ngx_log_error(NGX_LOG_CRIT,r->connection->log,0,"%d",r->headers_out.content_length_n);
			r->headers_out.content_type = type;
			b->file_pos = atol(start);
			b->file_last= atol(end);
			b->last_in_chain = 1;
			b->file->log = r->connection->log;
			b->last_buf = 1;
			out->buf = b;
			out->next = NULL;

			rc = ngx_http_send_header(r);
			if(rc == NGX_ERROR || rc > NGX_OK || r->header_only)
			{
				return rc;
			}
			
			return ngx_http_output_filter(r,out);
			
		}	
	
	}
	return NGX_OK;
}

static void *ngx_http_hls_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_hls_loc_conf_t *clcf;
	clcf = ngx_pcalloc(cf->pool,sizeof(ngx_http_hls_loc_conf_t));
	if( NULL == clcf)
	{
		return NGX_CONF_ERROR;
	}

	//clcf->mysql_host = ngx_string("");
	//clcf->mysql_host=ngx_string(" ");
	//clcf->mysql_port = 0;

	return clcf;

}

static char* ngx_http_hls_merge_loc_conf(ngx_conf_t *cf ,void *parent,void *child)
{
	ngx_http_hls_loc_conf_t *prev = parent;
	ngx_http_hls_loc_conf_t *next = child;

	ngx_conf_merge_str_value(next->media_type,prev->media_type,"");
	ngx_conf_merge_str_value(next->media_path,prev->media_path,"");

	return NGX_OK;
	
}

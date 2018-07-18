/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include "config.h"
#include "upnphttpdigest.h"
#ifdef ENABLE_HTTP_DATE
#include <time.h>
#endif
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "minitr064dpath.h"
#include "upnpsoap.h"
#include "upnputils.h"

#include <openssl/err.h>
#include <openssl/ssl.h>
#include "upnpglobalvars.h"
static SSL_CTX *ssl_ctx = NULL;

static void
syslogsslerr(void)
{
	unsigned long err;
	char buffer[256];
	while((err = ERR_get_error()) != 0) {
		syslog(LOG_ERR, "%s", ERR_error_string(err, buffer));
	}
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	syslog(LOG_DEBUG, "verify_callback(%d, %p)", preverify_ok, ctx);
	return preverify_ok;
}

int init_ssl(void)
{
	const SSL_METHOD *method;
	SSL_library_init();
	method = TLSv1_server_method();
	if(method == NULL) {
		syslog(LOG_ERR, "TLSv1_server_method() failed");
		syslogsslerr();
		return -1;
	}
	ssl_ctx = SSL_CTX_new(method);
	if(ssl_ctx == NULL) {
		syslog(LOG_ERR, "SSL_CTX_new() failed");
		syslogsslerr();
		return -1;
	}
	/* set the local certificate */
	if(!SSL_CTX_use_certificate_file(ssl_ctx, sslcertfile, SSL_FILETYPE_PEM)) {
		syslog(LOG_ERR, "SSL_CTX_use_certificate_file(%s) failed", sslcertfile);
		syslogsslerr();
		return -1;
	}
	/* set the private key */
	if(!SSL_CTX_use_PrivateKey_file(ssl_ctx, sslkeyfile, SSL_FILETYPE_PEM)) {
		syslog(LOG_ERR, "SSL_CTX_use_PrivateKey_file(%s) failed", sslkeyfile);
		syslogsslerr();
		return -1;
	}
	/* verify private key */
	if(!SSL_CTX_check_private_key(ssl_ctx)) {
		syslog(LOG_ERR, "SSL_CTX_check_private_key() failed");
		syslogsslerr();
		return -1;
	}
	/*SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE, verify_callback);*/
	SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, verify_callback);
	/*SSL_CTX_set_verify_depth(depth);*/
	syslog(LOG_INFO, "using %s", SSLeay_version(SSLEAY_VERSION));
	return 0;
}

void free_ssl(void)
{
	/* free context */
	if(ssl_ctx != NULL) {
		SSL_CTX_free(ssl_ctx);
		ssl_ctx = NULL;
	}
	ERR_remove_thread_state(NULL);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
}

struct upnphttp *
New_upnphttp(int s)
{
	struct upnphttp * ret;
	if(s<0)
		return NULL;
	ret = (struct upnphttp *)malloc(sizeof(struct upnphttp));
	if(ret == NULL)
		return NULL;
	memset(ret, 0, sizeof(struct upnphttp));
	ret->socket = s;
	if(!set_non_blocking(s))
		syslog(LOG_WARNING, "New_upnphttp::set_non_blocking(): %m");
	return ret;
}

void
InitSSL_upnphttp(struct upnphttp * h)
{
	int r;
	h->ssl = SSL_new(ssl_ctx);
	if(h->ssl == NULL) {
		syslog(LOG_ERR, "SSL_new() failed");
		syslogsslerr();
		abort();
	}
	if(!SSL_set_fd(h->ssl, h->socket)) {
		syslog(LOG_ERR, "SSL_set_fd() failed");
		syslogsslerr();
		abort();
	}
	r = SSL_accept(h->ssl); /* start the handshaking */
	if(r < 0) {
		int err;
		err = SSL_get_error(h->ssl, r);
		syslog(LOG_DEBUG, "SSL_accept() returned %d, SSL_get_error() %d", r, err);
		if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
			syslog(LOG_ERR, "SSL_accept() failed");
			syslogsslerr();
			abort();
		}
	}
}

void
CloseSocket_upnphttp(struct upnphttp * h)
{
	/* SSL_shutdown() ? */
	if(close(h->socket) < 0)
	{
		syslog(LOG_ERR, "CloseSocket_upnphttp: close(%d): %m", h->socket);
	}
	h->socket = -1;
	h->state = EToDelete;
}

void
Delete_upnphttp(struct upnphttp * h)
{
	if(h)
	{
		if(h->ssl)
			SSL_free(h->ssl);
		if(h->socket >= 0)
			CloseSocket_upnphttp(h);
		if(h->req_buf)
			free(h->req_buf);
		if(h->res_buf)
			free(h->res_buf);
		free(h);
	}
}

/* parse HttpHeaders of the REQUEST
 * This function is called after the \r\n\r\n character
 * sequence has been found in h->req_buf */
static void
ParseHttpHeaders(struct upnphttp * h)
{
	char * line;
	char * colon;
	char * p;
	int n;
	if((h->req_buf == NULL) || (h->req_contentoff <= 0))
		return;
	line = h->req_buf;
	while(line < (h->req_buf + h->req_contentoff))
	{
		colon = line;
		while(*colon != ':')
		{
			if(*colon == '\r' || *colon == '\n')
			{
				colon = NULL;	/* no ':' character found on the line */
				break;
			}
			colon++;
		}
		if(colon)
		{
			if(strncasecmp(line, "Content-Length", 14)==0)
			{
				p = colon;
				while(*p < '0' || *p > '9')
					p++;
				h->req_contentlen = atoi(p);
				if(h->req_contentlen < 0) {
					syslog(LOG_WARNING, "ParseHttpHeaders() invalid Content-Length %d", h->req_contentlen);
					h->req_contentlen = 0;
				}
				/*printf("*** Content-Lenght = %d ***\n", h->req_contentlen);
				printf("    readbufflen=%d contentoff = %d\n",
					h->req_buflen, h->req_contentoff);*/
			}
			else if(strncasecmp(line, "SOAPAction", 10)==0)
			{
				p = colon;
				n = 0;
				while(*p == ':' || *p == ' ' || *p == '\t')
					p++;
				while(p[n]>=' ')
					n++;
				if((p[0] == '"' && p[n-1] == '"')
				  || (p[0] == '\'' && p[n-1] == '\''))
				{
					p++; n -= 2;
				}
				h->req_soapActionOff = p - h->req_buf;
				h->req_soapActionLen = n;
			}
			else if(strncasecmp(line, "accept-language", 15) == 0)
			{
				p = colon;
				n = 0;
				while(*p == ':' || *p == ' ' || *p == '\t')
					p++;
				while(p[n]>=' ')
					n++;
				syslog(LOG_DEBUG, "accept-language HTTP header : '%.*s'", n, p);
				/* keep only the 1st accepted language */
				n = 0;
				while(p[n]>' ' && p[n] != ',')
					n++;
				if(n >= (int)sizeof(h->accept_language))
					n = (int)sizeof(h->accept_language) - 1;
				memcpy(h->accept_language, p, n);
				h->accept_language[n] = '\0';
			}
			else if(strncasecmp(line, "expect", 6) == 0)
			{
				p = colon;
				n = 0;
				while(*p == ':' || *p == ' ' || *p == '\t')
					p++;
				while(p[n]>=' ')
					n++;
				if(strncasecmp(p, "100-continue", 12) == 0) {
					h->respflags |= FLAG_CONTINUE;
					syslog(LOG_DEBUG, "\"Expect: 100-Continue\" header detected");
				}
			}
            else if(strncasecmp(line, "Authorization", 13) == 0)
            {
                processAuthorizationHeader(h, colon+1);
            }
		}
		/* the loop below won't run off the end of the buffer
		 * because the buffer is guaranteed to contain the \r\n\r\n
		 * character sequence */
		while(!(line[0] == '\r' && line[1] == '\n'))
			line++;
		line += 2;
	}
}

/* very minimalistic 404 error message */
void
Send401(struct upnphttp * h)
{
    static const char body401[] =
            "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>"
                    "<BODY><h1>401 Unauthorized.</h1></BODY></HTML>\r\n";

    h->respflags = FLAG_HTML;
    BuildResp2_upnphttp(h, 401, "Unauthorized",
            body401, sizeof(body401) - 1);
    SendRespAndClose_upnphttp(h);
}

/* very minimalistic 404 error message */
static void
Send404(struct upnphttp * h)
{
	static const char body404[] =
		"<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>"
		"<BODY><H1>Not Found</H1>The requested URL was not found"
		" on this server.</BODY></HTML>\r\n";

	h->respflags = FLAG_HTML;
	BuildResp2_upnphttp(h, 404, "Not Found",
	                    body404, sizeof(body404) - 1);
	SendRespAndClose_upnphttp(h);
}

static void
Send405(struct upnphttp * h)
{
	static const char body405[] =
		"<HTML><HEAD><TITLE>405 Method Not Allowed</TITLE></HEAD>"
		"<BODY><H1>Method Not Allowed</H1>The HTTP Method "
		"is not allowed on this resource.</BODY></HTML>\r\n";

	h->respflags |= FLAG_HTML;
	BuildResp2_upnphttp(h, 405, "Method Not Allowed",
	                    body405, sizeof(body405) - 1);
	SendRespAndClose_upnphttp(h);
}

/* very minimalistic 501 error message */
static void
Send501(struct upnphttp * h)
{
	static const char body501[] =
		"<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"
		"<BODY><H1>Not Implemented</H1>The HTTP Method "
		"is not implemented by this server.</BODY></HTML>\r\n";

	h->respflags = FLAG_HTML;
	BuildResp2_upnphttp(h, 501, "Not Implemented",
	                    body501, sizeof(body501) - 1);
	SendRespAndClose_upnphttp(h);
}

/* findendheaders() find the \r\n\r\n character sequence and
 * return a pointer to it.
 * It returns NULL if not found */
static const char *
findendheaders(const char * s, int len)
{
	while(len-->3)
	{
		if(s[0]=='\r' && s[1]=='\n' && s[2]=='\r' && s[3]=='\n')
			return s;
		s++;
	}
	return NULL;
}

/* Sends the description generated by the parameter */
static void
sendXMLdesc(struct upnphttp * h, char * (f)(int *, struct upnphttp *))
{
	char * desc;
	int len;
	desc = f(&len, h);
	if(!desc)
	{
		static const char error500[] = "<HTML><HEAD><TITLE>Error 500</TITLE>"
		   "</HEAD><BODY>Internal Server Error</BODY></HTML>\r\n";
		syslog(LOG_ERR, "Failed to generate XML description");
		h->respflags = FLAG_HTML;
		BuildResp2_upnphttp(h, 500, "Internal Server Error",
		                    error500, sizeof(error500)-1);
	}
	else
	{
		BuildResp_upnphttp(h, desc, len);
	}
	SendRespAndClose_upnphttp(h);
	free(desc);
}

/* ProcessHTTPPOST_upnphttp()
 * executes the SOAP query if it is possible */
static void
ProcessHTTPPOST_upnphttp(struct upnphttp * h)
{
	if((h->req_buflen - h->req_contentoff) >= h->req_contentlen)
	{
		/* the request body is received */
		if(h->req_soapActionOff > 0)
		{
			/* we can process the request */
			syslog(LOG_INFO, "SOAPAction: %.*s",
			       h->req_soapActionLen, h->req_buf + h->req_soapActionOff);
			ExecuteSoapAction(h,
				h->req_buf + h->req_soapActionOff,
				h->req_soapActionLen);
		}
		else
		{
			static const char err400str[] =
				"<html><body>Bad request</body></html>";
			syslog(LOG_INFO, "No SOAPAction in HTTP headers");
			h->respflags = FLAG_HTML;
			BuildResp2_upnphttp(h, 400, "Bad Request",
			                    err400str, sizeof(err400str) - 1);
			SendRespAndClose_upnphttp(h);
		}
	}
	else if(h->respflags & FLAG_CONTINUE)
	{
		/* Sending the 100 Continue response */
		if(!h->res_buf) {
			h->res_buf = malloc(256);
			h->res_buf_alloclen = 256;
		}
		h->res_buflen = snprintf(h->res_buf, h->res_buf_alloclen,
		                         "%s 100 Continue\r\n\r\n", h->HttpVer);
		h->res_sent = 0;
		h->state = ESendingContinue;
		if(SendResp_upnphttp(h))
			h->state = EWaitingForHttpContent;
	}
	else
	{
		/* waiting for remaining data */
		h->state = EWaitingForHttpContent;
	}
}

/* Parse and process Http Query
 * called once all the HTTP headers have been received,
 * so it is guaranteed that h->req_buf contains the \r\n\r\n
 * character sequence */
static void
ProcessHttpQuery_upnphttp(struct upnphttp * h)
{
	static const struct {
		const char * path;
		char * (* f)(int *, struct upnphttp *);
	} path_desc[] = {
		{ ROOTDESC_PATH, genRootDesc},
        { DI_PATH, genDI},
        { DC_PATH, genDC},
        { LCS_PATH, genLCS},
        { LHC_PATH, genLHC},
        { LEIC_PATH, genLEIC},
        { WLANC_PATH, genWLANC},
        { WCIC_PATH, genWCIC},
        { WDIC_PATH, genWDIC},
        { WEIC_PATH, genWEIC},
        { WDLC_PATH, genWDLC},
        { WELC_PATH, genWELC},
        { WANPPP_PATH, genWANPPP},
        { WANIP_PATH, genWANIP},
		{ NULL, NULL}
	};
	char HttpCommand[16];
	char HttpUrl[128];
	char * HttpVer;
	char * p;
	int i;
	p = h->req_buf;
	if(!p)
		return;
	/* note : checking (*p != '\r') is enough to avoid runing off the
	 * end of the buffer, because h->req_buf is guaranteed to contain
	 * the \r\n\r\n character sequence */
	for(i = 0; i<15 && *p != ' ' && *p != '\r'; i++)
		HttpCommand[i] = *(p++);
	HttpCommand[i] = '\0';
	while(*p==' ')
		p++;
	for(i = 0; i<127 && *p != ' ' && *p != '\r'; i++)
		HttpUrl[i] = *(p++);
	HttpUrl[i] = '\0';
	while(*p==' ')
		p++;
	HttpVer = h->HttpVer;
	for(i = 0; i<15 && *p != '\r'; i++)
		HttpVer[i] = *(p++);
	HttpVer[i] = '\0';
	syslog(LOG_INFO, "HTTP REQUEST : %s %s (%s)",
	       HttpCommand, HttpUrl, HttpVer);
	ParseHttpHeaders(h);
	if(strcmp("POST", HttpCommand) == 0)
	{
		if(memcmp(HttpUrl, BASE_CTL_PATH, 8+5)) {
			syslog(LOG_NOTICE, "%s not found, responding ERROR 404", HttpUrl);
			Send404(h);
			return;
		}
		h->req_command = EPost;
		ProcessHTTPPOST_upnphttp(h);
	}
	else if(strcmp("GET", HttpCommand) == 0)
	{
		h->req_command = EGet;
		for(i=0; path_desc[i].path; i++) {
			if(strcasecmp(path_desc[i].path, HttpUrl) == 0) {
				if(path_desc[i].f)
					sendXMLdesc(h, path_desc[i].f);
				else
					continue;
				return;
			}
		}
		if(0 == memcmp(HttpUrl, BASE_CTL_PATH, 8+5)) {
			/* 405 Method Not Allowed
			 * Allow: POST */
			h->respflags = FLAG_ALLOW_POST;
			Send405(h);
			return;
		}
		syslog(LOG_NOTICE, "%s not found, responding ERROR 404", HttpUrl);
		Send404(h);
	}
	else if(strcmp("SUBSCRIBE", HttpCommand) == 0)
	{
		syslog(LOG_NOTICE, "SUBSCRIBE not implemented. Events not supported");
		Send501(h);
	}
	else
	{
		syslog(LOG_NOTICE, "Unsupported HTTP Command %s", HttpCommand);
		Send501(h);
	}
}


void
Process_upnphttp(struct upnphttp * h)
{
	char * h_tmp;
	char buf[2048];
	int n;

	if(!h)
		return;
	switch(h->state)
	{
	case EWaitingForHttpRequest:
		if(h->ssl) {
			n = SSL_read(h->ssl, buf, sizeof(buf));
		} else {
			n = recv(h->socket, buf, sizeof(buf), 0);
		}
		if(n<0)
		{
			if(h->ssl) {
				int err;
				err = SSL_get_error(h->ssl, n);
				if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
				{
					syslog(LOG_ERR, "SSL_read() failed");
					syslogsslerr();
					h->state = EToDelete;
				}
			} else {
			if(errno != EAGAIN &&
			   errno != EWOULDBLOCK &&
			   errno != EINTR)
			{
				syslog(LOG_ERR, "recv (state0): %m");
				h->state = EToDelete;
			}
			/* if errno is EAGAIN, EWOULDBLOCK or EINTR, try again later */
			}
		}
		else if(n==0)
		{
			syslog(LOG_WARNING, "HTTP Connection closed unexpectedly");
			h->state = EToDelete;
		}
		else
		{
			const char * endheaders;
			/* if 1st arg of realloc() is null,
			 * realloc behaves the same as malloc() */
			h_tmp = (char *)realloc(h->req_buf, n + h->req_buflen + 1);
			if (h_tmp == NULL)
			{
				syslog(LOG_WARNING, "Unable to allocate new memory for h->req_buf)");
				h->state = EToDelete;
			}
			else
			{
				h->req_buf = h_tmp;
				memcpy(h->req_buf + h->req_buflen, buf, n);
				h->req_buflen += n;
				h->req_buf[h->req_buflen] = '\0';
			}
			/* search for the string "\r\n\r\n" */
			endheaders = findendheaders(h->req_buf, h->req_buflen);
			if(endheaders)
			{
				/* at this point, the request buffer (h->req_buf)
				 * is guaranteed to contain the \r\n\r\n character sequence */
				h->req_contentoff = endheaders - h->req_buf + 4;
				ProcessHttpQuery_upnphttp(h);
			}
		}
		break;
	case EWaitingForHttpContent:
		if(h->ssl) {
			n = SSL_read(h->ssl, buf, sizeof(buf));
		} else {
			n = recv(h->socket, buf, sizeof(buf), 0);
		}
		if(n<0)
		{
			if(h->ssl) {
				int err;
				err = SSL_get_error(h->ssl, n);
				if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
				{
					syslog(LOG_ERR, "SSL_read() failed");
					syslogsslerr();
					h->state = EToDelete;
				}
			} else {
			if(errno != EAGAIN &&
			   errno != EWOULDBLOCK &&
			   errno != EINTR)
			{
				syslog(LOG_ERR, "recv (state1): %m");
				h->state = EToDelete;
			}
			/* if errno is EAGAIN, EWOULDBLOCK or EINTR, try again later */
			}
		}
		else if(n==0)
		{
			syslog(LOG_WARNING, "HTTP Connection closed inexpectedly");
			h->state = EToDelete;
		}
		else
		{
			void * tmp = realloc(h->req_buf, n + h->req_buflen);
			if(!tmp)
			{
				syslog(LOG_ERR, "memory allocation error %m");
				h->state = EToDelete;
			}
			else
			{
				h->req_buf = tmp;
				memcpy(h->req_buf + h->req_buflen, buf, n);
				h->req_buflen += n;
				if((h->req_buflen - h->req_contentoff) >= h->req_contentlen)
				{
					ProcessHTTPPOST_upnphttp(h);
				}
			}
		}
		break;
	case ESendingContinue:
		if(SendResp_upnphttp(h))
			h->state = EWaitingForHttpContent;
		break;
	case ESendingAndClosing:
		SendRespAndClose_upnphttp(h);
		break;
	default:
		syslog(LOG_WARNING, "Unexpected state: %d", h->state);
	}
}

static const char httpresphead[] =
	"%s %d %s\r\n"
	"Content-Type: %s\r\n"
	"Connection: close\r\n"
	"Content-Length: %d\r\n"
	"Server: " MINITR064D_SERVER_STRING "\r\n"
	"Ext:\r\n"
	;	/*"\r\n";*/
/*
		"<?xml version=\"1.0\"?>\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"

		"</s:Body>"
		"</s:Envelope>";
*/
/* with response code and response message
 * also allocate enough memory */

void
BuildHeader_upnphttp(struct upnphttp * h, int respcode,
                     const char * respmsg,
                     int bodylen)
{
	int templen;
	if(!h->res_buf ||
	   h->res_buf_alloclen < ((int)sizeof(httpresphead) + 256 + bodylen)) {
		if(h->res_buf)
			free(h->res_buf);
		templen = sizeof(httpresphead) + 256 + bodylen;
		h->res_buf = (char *)malloc(templen);
		if(!h->res_buf) {
			syslog(LOG_ERR, "malloc error in BuildHeader_upnphttp()");
			return;
		}
		h->res_buf_alloclen = templen;
	}
	h->res_sent = 0;
	h->res_buflen = snprintf(h->res_buf, h->res_buf_alloclen,
	                         httpresphead, h->HttpVer,
	                         respcode, respmsg,
	                         (h->respflags&FLAG_HTML)?"text/html":"text/xml; charset=\"utf-8\"",
							 bodylen);
	/* Content-Type MUST be 'text/xml; charset="utf-8"' according to UDA v1.1 */
	/* Content-Type MUST be 'text/xml' according to UDA v1.0 */
	/* Additional headers */
#ifdef ENABLE_HTTP_DATE
	{
		char http_date[64];
		time_t t;
		struct tm tm;
		time(&t);
		gmtime_r(&t, &tm);
		/* %a and %b depend on locale */
		strftime(http_date, sizeof(http_date),
		         "%a, %d %b %Y %H:%M:%S GMT", &tm);
		h->res_buflen += snprintf(h->res_buf + h->res_buflen,
		                          h->res_buf_alloclen - h->res_buflen,
		                          "Date: %s\r\n", http_date);
	}
#endif
	if(h->respflags & FLAG_ALLOW_POST) {
		h->res_buflen += snprintf(h->res_buf + h->res_buflen,
		                          h->res_buf_alloclen - h->res_buflen,
		                          "Allow: %s\r\n", "POST");
	} else if(h->respflags & FLAG_ALLOW_SUB_UNSUB) {
		h->res_buflen += snprintf(h->res_buf + h->res_buflen,
		                          h->res_buf_alloclen - h->res_buflen,
		                          "Allow: %s\r\n", "SUBSCRIBE, UNSUBSCRIBE");
	}
	if(h->accept_language[0] != '\0') {
		/* defaulting to "en" */
		h->res_buflen += snprintf(h->res_buf + h->res_buflen,
		                          h->res_buf_alloclen - h->res_buflen,
		                          "Content-Language: %s\r\n",
		                          h->accept_language[0] == '*' ? "en" : h->accept_language);
	}
    if(respcode == 401) {
        h->res_buflen += buildWWWAuthenticateHeader(h);
    }
	h->res_buf[h->res_buflen++] = '\r';
	h->res_buf[h->res_buflen++] = '\n';
	if(h->res_buf_alloclen < (h->res_buflen + bodylen))
	{
		char * tmp;
		tmp = (char *)realloc(h->res_buf, (h->res_buflen + bodylen));
		if(tmp)
		{
			h->res_buf = tmp;
			h->res_buf_alloclen = h->res_buflen + bodylen;
		}
		else
		{
			syslog(LOG_ERR, "realloc error in BuildHeader_upnphttp()");
		}
	}
}

void
BuildResp2_upnphttp(struct upnphttp * h, int respcode,
                    const char * respmsg,
                    const char * body, int bodylen)
{
	BuildHeader_upnphttp(h, respcode, respmsg, bodylen);
	if(body)
		memcpy(h->res_buf + h->res_buflen, body, bodylen);
	h->res_buflen += bodylen;
}

/* responding 200 OK ! */
void
BuildResp_upnphttp(struct upnphttp * h,
                        const char * body, int bodylen)
{
	BuildResp2_upnphttp(h, 200, "OK", body, bodylen);
}

int
SendResp_upnphttp(struct upnphttp * h)
{
	ssize_t n;

	while (h->res_sent < h->res_buflen)
	{
		if(h->ssl) {
			n = SSL_write(h->ssl, h->res_buf + h->res_sent,
			         h->res_buflen - h->res_sent);
		} else {
			n = send(h->socket, h->res_buf + h->res_sent,
			         h->res_buflen - h->res_sent, 0);
		}
		if(n<0)
		{
			if(h->ssl) {
				int err;
				err = SSL_get_error(h->ssl, n);
				if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
					/* try again later */
					return 0;
				}
				syslog(LOG_ERR, "SSL_write() failed");
				syslogsslerr();
				break;
			} else {
			if(errno == EINTR)
				continue;	/* try again immediatly */
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				/* try again later */
				return 0;
			}
			syslog(LOG_ERR, "send(res_buf): %m");
			break; /* avoid infinite loop */
			}
		}
		else if(n == 0)
		{
			syslog(LOG_ERR, "send(res_buf): %d bytes sent (out of %d)",
							h->res_sent, h->res_buflen);
			break;
		}
		else
		{
			h->res_sent += n;
		}
	}
	return 1;	/* finished */
}

void
SendRespAndClose_upnphttp(struct upnphttp * h)
{
	ssize_t n;

	while (h->res_sent < h->res_buflen)
	{
		if(h->ssl) {
			n = SSL_write(h->ssl, h->res_buf + h->res_sent,
			         h->res_buflen - h->res_sent);
		} else {
			n = send(h->socket, h->res_buf + h->res_sent,
			         h->res_buflen - h->res_sent, 0);
		}
		if(n<0)
		{
			if(h->ssl) {
				int err;
				err = SSL_get_error(h->ssl, n);
				if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
					/* try again later */
					h->state = ESendingAndClosing;
					return;
				}
				syslog(LOG_ERR, "SSL_write() failed");
				syslogsslerr();
				break; /* avoid infinite loop */
			} else {
			if(errno == EINTR)
				continue;	/* try again immediatly */
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				/* try again later */
				h->state = ESendingAndClosing;
				return;
			}
			syslog(LOG_ERR, "send(res_buf): %m");
			break; /* avoid infinite loop */
			}
		}
		else if(n == 0)
		{
			syslog(LOG_ERR, "send(res_buf): %d bytes sent (out of %d)",
							h->res_sent, h->res_buflen);
			break;
		}
		else
		{
			h->res_sent += n;
		}
	}
	CloseSocket_upnphttp(h);
}


#include <uv.h>
#include <malloc.h>
#include <string.h>
#include <ImageMagick-6/wand/MagickWand.h>

#include "curl_service.h"
#include "helper.h"
#include "image_helper.h"

#define default_http_port 8080
#define default_tcp_backlog 0xFF

#define http_header_format 		\
	"HTTP/1.1 %d\r\n"			\
	"Content-Type: %s\r\n"		\
	"Content-Length: %zu\r\n"	\
	"\r\n"

static error http_handle_get(uv_stream_t *client, char **content_type, char **body, size_t *body_size);

typedef
struct _data_with_len {
	char *data;
	size_t len;
} data_with_len;

typedef
struct _luv_write_data {
	uv_write_t wt;
	uv_buf_t *bufs;
	size_t bufcount;
} luv_write_data;

uv_tcp_t* g_tcp_server;

typedef 
enum _http_request_type {
	http_unsupported = -2,
	http_eagain = -1,
    http_get,
	http_last
} http_request_type;

typedef error (*http_function)(uv_stream_t *client, char **content_type, char **body, size_t *body_size);
const char *g_http_requests [http_last + 1] = {
	"GET",
	NULL
};
http_function g_http_functions[] = {
	http_handle_get,
	NULL
};

static http_request_type http_get_request_type(const char *header)
{
	int i;
	http_request_type ret = http_unsupported;

	// check header 
	bail_assert(strstr(header, "\r\n\r\n") != NULL, http_eagain);
	bail_assert(strstr(header, " ") != NULL, http_unsupported);

	for (i = 0; g_http_requests[i]; ++i) {
		if (strncmp(g_http_requests[i], header, strlen(g_http_requests[i])) == 0) {
			ret = i;
			break;
		}
	}

exit:
	return ret;
}

static error http_do_image_operation(
	const char *version,
	const char *operation,
	const char *parameter,
	const char *url,
	char **content_type,
	char **image,
	size_t *image_size)
{
	int ret;
	curl_service *cs = NULL;
	
	char *image_org = NULL;
	size_t image_org_size = 0;

	image_function image_func;

	char *format = NULL;

	bail_assert(content_type != NULL, err_parameter);
	bail_assert(image != NULL, err_parameter);
	bail_assert(image_size != NULL, err_parameter);

	bail_assert(strcmp(version, "v1") == 0, err_incorrectdata);

	// get related image function (crop, grayscale etc)
	image_func = image_get_op_func(operation);
	bail_assert(image_func != NULL, err_incorrectdata);

	// we know that our functions verify "parameter" first
	// so here we are using them to verify its params before downloading image
	bail_assert(image_func(parameter, NULL, 0, NULL, NULL, NULL) != err_incorrectdata, err_incorrectdata);

	ret = cs_create(&cs);
	bail_assert(ret == cs_success, err_alloc);

	// download image
	ret = cs_get(cs, url, &image_org, &image_org_size);
	bail_assert(ret == cs_success, err_alloc);

	// do image operation (crop, grayscale etc)
	ret = image_func(parameter, image_org, image_org_size, image, image_size, &format);
	bail_assert(ret == cs_success, err_alloc);

	// the space for null terminator comes from array_size
	*content_type = calloc(strlen(format) + array_size("image/"), sizeof(char));
	bail_assert(*content_type != NULL, err_alloc);

	sprintf(*content_type, "image/%s", format);
	ret = err_success;
	
exit:
	if (format) {
		free(format);
	}
	if (cs) {
		cs_free(cs);
	}
	return ret;
}

// http_handle_get verifies and parses the query string
static error http_handle_get(uv_stream_t *client,
	char **content_type,
	char **body,
	size_t *body_size)
{
	error ret;
	char *querystring = NULL;
	char *version = NULL;
	char *operation = NULL;
	char *parameter = NULL;
	char *url = NULL;
    char *ptr = NULL;

	data_with_len *lr = (data_with_len *)uv_handle_get_data((uv_handle_t *)client);
	
	bail_assert(strstr(lr->data + strlen(g_http_requests[http_get]), " ") != NULL, err_incorrectdata);
	bail_assert(sscanf(lr->data + strlen(g_http_requests[http_get]), "%ms", &querystring) == 1, err_incorrectdata);
	bail_assert(strstr(querystring, "url:") != NULL, err_incorrectdata);

	url = strstr(querystring, "url:") + array_size("url:") - 1;
	strstr(querystring, "url:")[0] = '\0';
	
    ptr = querystring;
	version = strtok_r(ptr, "/:", &ptr);
	operation = strtok_r(ptr, "/:", &ptr);
	parameter = strtok_r(ptr, "/:", &ptr);

	ret = http_do_image_operation(version, operation, parameter, url, content_type, body, body_size);
	bail_assert(ret == err_success, ret);

exit:

	if (querystring) {
		free(querystring);
	}
	return ret;
}

static void luv_cb_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) 
{
	buf->base = (char *)malloc(suggested_size);
	buf->len = suggested_size;
}

static void luv_cb_on_write(uv_write_t *wdata, int status)
{
	size_t i;
	luv_write_data *uwd = (luv_write_data *)wdata;

	for (i = 0; i < uwd->bufcount; ++i) {
		if (uwd->bufs[i].base != NULL) {
			free(uwd->bufs[i].base);
		}
	}
	
	uv_close((uv_handle_t *)wdata->handle, NULL);
	free(uwd);

	// TODO error handling


//exit:
	return;
}

// write http packet into client stream
static int luv_http_write(uv_stream_t *client, int code, const char *content_type, char *body, size_t body_size)
{
	int ret = err_success;

	char *header = NULL;
	luv_write_data *uwd = NULL;

	bail_assert(content_type != NULL, err_parameter);

	header = calloc(array_size(http_header_format) + 32, sizeof(char));
	bail_assert(ret == cs_success, err_alloc);

	sprintf(header, http_header_format,
		code,
		content_type,
		body_size
		);

	uwd = (luv_write_data *)malloc(sizeof(luv_write_data));
	bail_assert(uwd != NULL, err_alloc);

	uwd->bufcount = (body == NULL ? 1 : 2);
	uwd->bufs = (uv_buf_t *)malloc(sizeof(uv_buf_t) * uwd->bufcount);
	bail_assert(uwd->bufs, err_alloc);
	
	uwd->bufs[0] = uv_buf_init(header, strlen(header));
	if (body != NULL) {
		uwd->bufs[1] = uv_buf_init(body, body_size);
	}
    
    uv_write((uv_write_t *)uwd, client, uwd->bufs, uwd->bufcount, luv_cb_on_write);

exit:
	if (ret) {
		if (header) {
			free(header);
		}
		if (uwd) {
			if (uwd->bufs) {
				free(uwd->bufs);
			}
			free(uwd);
		}
	}
	return ret;
}

static void luv_cb_on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) 
{
	error ret = err_success;
	http_request_type req;

	char *content_type = NULL;
	char *body = NULL;
	size_t body_size;

	bail_assert(nread > 0 || nread == UV_EOF, err_tcp_read);

	data_with_len *lr = (data_with_len *)uv_handle_get_data((uv_handle_t *)client);
	if (lr->len < lr->len + nread + sizeof(char)) {
		char *p = realloc(lr->data, lr->len + nread + sizeof(char));
		bail_assert(p != NULL, err_alloc);

		memset(p + lr->len, '\0', nread + sizeof(char));
		lr->data = p;
	}
	strncat(lr->data, buf->base, nread);

	req = http_get_request_type(lr->data);
	// if we need more data
	if (req == http_eagain) {
		ret = err_success;
		goto exit;
	}
	bail_assert(req > -1, err_incorrectdata);

	// we found request type, call related function
	ret = g_http_functions[req](client, &content_type, &body, &body_size);
	bail_assert(ret == err_success, ret);

	ret = luv_http_write(client, 200, content_type, body, body_size);
	bail_assert(ret == err_success, ret);

exit:
	free(buf->base);
	// we dont need our data alive if we are not waiting for more
	if (req != http_eagain) {
		if (lr->data) {
			free(lr->data);
		}
	}
	if (ret) {
		// if error occurred while reading
		if (body) {
			free(body);
		}
		if (content_type) {
			free(content_type);
		}
		if (nread < 0 && nread != UV_EOF) {
        	uv_close((uv_handle_t *)client, NULL);
		}
		else {
			// ret != 0 means uv_write cannot be called, so we are reporting error to the client
			body = calloc(array_size("Error No: ") + 10, sizeof(char));
			if (body) {
				sprintf(body, "Error No: %d", ret);
				ret = luv_http_write(client, 500, "text/html", body, strlen(body));
				// we cant even write the error, freeing the mem
				if (ret) {
					free(body);
					uv_close((uv_handle_t *)client, NULL);
				}
			}
		}
	}
}

// when new connection comes, allocate buffer and start reading from the client
static void luv_cb_on_connection(uv_stream_t* server, int status)
{
	error ret;
	uv_tcp_t* client;
	data_with_len *lread;

	bail_assert(status == 0, err_parameter);

	client = malloc(sizeof(uv_tcp_t));
	bail_assert(client != NULL, err_alloc);

	ret = uv_tcp_init(uv_default_loop(), client);
	bail_assert(ret == 0, err_tcp_init);

	lread = calloc(1, sizeof(data_with_len));
	bail_assert(lread != NULL, err_alloc);

	uv_handle_set_data((uv_handle_t *)client, lread);

	ret = uv_accept(server, (uv_stream_t *)client);
	bail_assert(ret == 0, err_tcp_accept);

	ret = uv_read_start((uv_stream_t *)client, luv_cb_on_alloc, luv_cb_on_read);
	bail_assert(ret == 0, err_tcp_read);

exit:
	if (ret) {
		if (client) {
			free(client);
		}
		if (lread) {
			free(lread);
		}
	}
}

// init tcp server on all interfaces
error luv_init(void)
{
    error ret;
    struct sockaddr_in addr;

    ret = uv_ip4_addr("0.0.0.0", default_http_port, &addr);
    bail_assert(ret == 0, err_addr);

	g_tcp_server = (uv_tcp_t *)malloc(sizeof(*g_tcp_server));
	bail_assert(g_tcp_server != NULL, err_alloc);

	ret = uv_tcp_init(uv_default_loop(), g_tcp_server);
	bail_assert(ret == 0, err_tcp_init);

	ret = uv_tcp_bind(g_tcp_server, (const struct sockaddr *)&addr, 0);
	bail_assert(ret == 0, err_tcp_bind);

	ret = uv_listen((uv_stream_t *)g_tcp_server, default_tcp_backlog, luv_cb_on_connection);
	bail_assert(ret == 0, err_tcp_listen);

exit:
	// TODO handle tcp deinit
	if (ret) {
		if (g_tcp_server) {
			free(g_tcp_server);
		}
	}
	return ret;
}

error luv_run(void)
{
	error ret;
	
	ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	bail_assert(ret == 0, err_tcp_run);

exit:
	if (ret) {
	}
	return ret;
}

int main(int argc, char *argv[])
{
	error ret;

	ret = luv_init();
	bail_assert(ret == err_success, ret);

	ret = luv_run();
	bail_assert(ret == err_success, ret);

exit:
	if (ret) {
	}
	return ret;
}
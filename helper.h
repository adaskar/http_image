#ifndef _HELPER
#define _HELPER

// TODO this should not be here
typedef 
enum _error {
    err_success = 0,
    err_parameter,
    err_alloc,
    err_addr,
	err_incorrectdata,
	err_tcp_init,
	err_tcp_bind,
	err_tcp_listen,
	err_tcp_accept,
	err_tcp_read,
	err_tcp_write,
	err_tcp_run,
	err_magick_new,
	err_magick_read,
	err_magick_operation,
	err_magick_write,
} error;

#define bail_assert(s, error_code)					\
	do {											\
		if ( !(s) ) {								\
            ret = error_code;                       \
    		fprintf(								\
				stderr,								\
				"Error in %s at %d with code %d.\r\n",\
				__FILE__,							\
				__LINE__,							\
				error_code);						\
			goto exit;								\
		}											\
	} while (0)

#define array_size(arr)								\
	sizeof((arr)) / sizeof(*(arr))

#endif
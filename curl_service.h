#ifndef _CURL_SERVICE
#define _CURL_SERVICE

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <curl/curl.h>
#include "helper.h"

typedef 
enum _cs_error {
    cs_success = 0,
    cs_parameter,
    cs_alloc,
    cs_curl_init,
    cs_curl_perform
} cs_error;

typedef 
struct _curl_service {
    CURL *curl;
    struct {
        char *data;
        size_t size;
    } response;
} curl_service;

cs_error cs_create(curl_service **c_service);
cs_error cs_reset_response(curl_service *cs);
cs_error cs_get(curl_service *cs, const char *url, char **data, size_t *size);
cs_error cs_free(curl_service *cs);

#endif
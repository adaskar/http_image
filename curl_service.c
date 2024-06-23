#include "curl_service.h"

static size_t cs_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    int ret = 0;
    curl_service *cs = NULL;

    char *p = NULL;
    
    cs = (curl_service *)userdata;

    // this function should return 0 when error occurred
    bail_assert(cs != NULL, 0);

    p = realloc(cs->response.data, cs->response.size + nmemb * size);
    
    // this function should return 0 when error occurred
    bail_assert(p != NULL, 0);

    ret = nmemb * size;
    cs->response.data = p;
    memcpy(&(cs->response.data[cs->response.size]), ptr, ret);
    cs->response.size += ret;
    
exit:
    return ret;
}

// creates and init curl lib and needed structures
cs_error cs_create(curl_service **c_service)
{
    cs_error ret;

    curl_service *cs = NULL;
    struct curl_slist *clist = NULL;

    bail_assert(c_service != NULL, cs_parameter);

    cs = calloc(1, sizeof(curl_service));
    bail_assert(cs != NULL, cs_alloc);

    cs->curl = curl_easy_init();
    bail_assert(cs->curl != NULL, cs_curl_init);

    clist = curl_slist_append(clist, "Accept: */*");
    clist = curl_slist_append(clist, "User-Agent: http_image");

    curl_easy_setopt(cs->curl, CURLOPT_HTTPHEADER, clist);
    curl_easy_setopt(cs->curl, CURLOPT_WRITEDATA, (void *)cs);
    curl_easy_setopt(cs->curl, CURLOPT_WRITEFUNCTION, cs_write_callback);
    curl_easy_setopt(cs->curl, CURLOPT_FOLLOWLOCATION, 1L);

    *c_service = cs;
    ret = cs_success;

exit:
    if (ret != cs_success) {
        if (cs) {
            free (cs);
        }
    }
    return ret;
}

// clear internally allocated memory
cs_error cs_reset_response(curl_service *cs)
{
    cs_error ret;

    bail_assert(cs != NULL, cs_parameter);

    if (cs->response.data) {
        free(cs->response.data);
    }

    cs->response.size = 0;
    cs->response.data = NULL;

    ret = cs_success;

exit:
    return ret;
}

// make get request to url
// data should not be freed
// internal data can be freed by calling cs_reset_response
cs_error cs_get(curl_service *cs, const char *url, char **data, size_t *size)
{
    cs_error ret;
    CURLcode res;

    bail_assert(cs != NULL, cs_parameter);
    bail_assert(url != NULL, cs_parameter);
    bail_assert(data != NULL, cs_parameter);
    bail_assert(size != NULL, cs_parameter);

    cs_reset_response(cs);

    curl_easy_setopt(cs->curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(cs->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(cs->curl, CURLOPT_URL, url);

    res = curl_easy_perform(cs->curl);
    bail_assert(res == CURLE_OK, cs_curl_perform);

    *data = cs->response.data;
    *size = cs->response.size;

    ret = cs_success;

exit:
    return ret;
}

// free curl lib and used structures
cs_error cs_free(curl_service *cs)
{
    cs_error ret;

    bail_assert(cs != NULL, cs_parameter);

    if (cs->curl) {
        curl_easy_cleanup(cs->curl);
        cs->curl = NULL;
    }

    ret = cs_reset_response(cs);
    bail_assert(ret == cs_success, ret);

    if (cs) {
        free(cs);
    }

exit:
    return ret;
}

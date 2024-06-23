#ifndef _IMAGE_HELPER
#define _IMAGE_HELPER

#include <stdio.h>
#include "helper.h"

typedef error (*image_function)(const char *parameter, char *image, size_t size, char **out, size_t *outsize, char **format);

image_function image_get_op_func(const char *operation);

#endif
#include <string.h>
#include <ImageMagick-6/wand/MagickWand.h>
#include "image_helper.h"

error image_resize(const char *parameter, char *image, size_t size, char **out, size_t *outsize, char **format)
{
	error ret;
	size_t width;
	size_t height;
	MagickBooleanType mbt;
	MagickWand *mw = NULL;
	unsigned char *outimg = NULL;

	bail_assert(parameter != NULL, err_incorrectdata);
	bail_assert(strstr(parameter, "x") != NULL, err_incorrectdata);
	bail_assert(sscanf(parameter, "%zu", &width) == 1, err_incorrectdata);
	bail_assert(sscanf(strstr(parameter, "x") + 1, "%zu", &height) == 1, err_incorrectdata);
	
	bail_assert(image != NULL, err_parameter);
	bail_assert(out != NULL, err_parameter);
	bail_assert(outsize != NULL, err_parameter);
	bail_assert(format != NULL, err_parameter);

	mw = NewMagickWand();
	bail_assert(mw != NULL, err_magick_new);

	mbt = MagickReadImageBlob(mw, (const void *)image, size);
	bail_assert(mbt == MagickTrue, err_magick_read);

	*format = MagickGetImageFormat(mw);

	mbt = MagickResizeImage(mw, width, height, BoxFilter, 0);
	bail_assert(mbt == MagickTrue, err_magick_operation);

	outimg = MagickGetImageBlob(mw, outsize);
	bail_assert(outimg != NULL, err_magick_write);

	*out = malloc(*outsize);
	bail_assert(*out != NULL, err_alloc);

	memcpy(*out, outimg, *outsize);
	ret = 0;

exit:
    if (outimg) {
		MagickRelinquishMemory(outimg);
    }
	if (mw) {
		DestroyMagickWand(mw);
	}
	return ret;
}

error image_rotate(const char *parameter, char *image, size_t size, char **out, size_t *outsize, char **format)
{
	error ret;
	size_t degree;
	MagickBooleanType mbt;

	MagickWand *mw = NULL;
	PixelWand *pw;

	unsigned char *outimg = NULL;

	bail_assert(parameter != NULL, err_incorrectdata);
	bail_assert(sscanf(parameter, "%zu", &degree) == 1, err_incorrectdata);
	
	bail_assert(image != NULL, err_parameter);
	bail_assert(out != NULL, err_parameter);
	bail_assert(outsize != NULL, err_parameter);
	bail_assert(format != NULL, err_parameter);
	
	mw = NewMagickWand();
	bail_assert(mw != NULL, err_magick_new);

	mbt = MagickReadImageBlob(mw, (const void *)image, size);
	bail_assert(mbt == MagickTrue, err_magick_read);

	*format = MagickGetImageFormat(mw);

	pw = NewPixelWand();
	bail_assert(pw != NULL, err_magick_new);

	PixelSetColor(pw, "#000000");

	mbt = MagickRotateImage(mw, pw, (double)degree);
	bail_assert(mbt == MagickTrue, err_magick_operation);

	outimg = MagickGetImageBlob(mw, outsize);
	bail_assert(outimg != NULL, err_magick_write);

	*out = malloc(*outsize);
	bail_assert(*out != NULL, err_alloc);

	memcpy(*out, outimg, *outsize);
	ret = 0;

exit:
	if (outimg) {
		MagickRelinquishMemory(outimg);
	}
	if (pw) {
		// TODO DestroyPixelWand(pw);
	}
	if (mw) {
		DestroyMagickWand(mw);
	}
	return ret;
}

error image_grayscale(const char *parameter, char *image, size_t size, char **out, size_t *outsize, char **format)
{
	error ret;
	MagickBooleanType mbt;

	MagickWand *mw = NULL;

	unsigned char *outimg = NULL;
	
	bail_assert(image != NULL, err_parameter);
	bail_assert(out != NULL, err_parameter);
	bail_assert(outsize != NULL, err_parameter);
	bail_assert(format != NULL, err_parameter);
	
	mw = NewMagickWand();
	bail_assert(mw != NULL, err_magick_new);

	mbt = MagickReadImageBlob(mw, (const void *)image, size);
	bail_assert(mbt == MagickTrue, err_magick_read);

	*format = MagickGetImageFormat(mw);

	mbt = MagickSetImageType(mw, GrayscaleType);
	bail_assert(mbt == MagickTrue, err_magick_operation);

	outimg = MagickGetImageBlob(mw, outsize);
	bail_assert(outimg != NULL, err_magick_write);

	*out = malloc(*outsize);
	bail_assert(*out != NULL, err_alloc);

	memcpy(*out, outimg, *outsize);
	ret = 0;

exit:
	if (outimg) {
		MagickRelinquishMemory(outimg);
	}
	if (mw) {
		DestroyMagickWand(mw);
	}
	return ret;
}

// crop needs parameter as XxY_WxH
error image_crop(const char *parameter, char *image, size_t size, char **out, size_t *outsize, char **format)
{
	error ret;
	size_t x;
	size_t y;
	size_t width;
	size_t height;
	MagickBooleanType mbt;

	MagickWand *mw = NULL;

	unsigned char *outimg = NULL;
	
	bail_assert(parameter != NULL, err_incorrectdata);
	bail_assert(strstr(parameter, "x") != NULL, err_incorrectdata);
	bail_assert(strstr(parameter, "_") != NULL, err_incorrectdata);
	bail_assert(strstr(parameter, "x") < strstr(parameter, "_"), err_incorrectdata);
	bail_assert(strstr(strstr(parameter, "_") + 1, "x") != NULL, err_incorrectdata);

	bail_assert(sscanf(parameter, "%zu", &x) == 1, err_incorrectdata);
	bail_assert(sscanf(strstr(parameter, "x") + 1, "%zu", &y) == 1, err_incorrectdata);

	bail_assert(sscanf(strstr(parameter, "_") + 1, "%zu", &width) == 1, err_incorrectdata);
	bail_assert(sscanf(strstr(strstr(parameter, "_") + 1, "x") + 1, "%zu", &height) == 1, err_incorrectdata);

	bail_assert(image != NULL, err_parameter);
	bail_assert(out != NULL, err_parameter);
	bail_assert(outsize != NULL, err_parameter);
	bail_assert(format != NULL, err_parameter);
	
	mw = NewMagickWand();
	bail_assert(mw != NULL, err_magick_new);

	mbt = MagickReadImageBlob(mw, (const void *)image, size);
	bail_assert(mbt == MagickTrue, err_magick_read);

	*format = MagickGetImageFormat(mw);

	mbt = MagickCropImage(mw, width, height, x, y);
	bail_assert(mbt == MagickTrue, err_magick_operation);

	outimg = MagickGetImageBlob(mw, outsize);
	bail_assert(outimg != NULL, err_magick_write);

	*out = malloc(*outsize);
	bail_assert(*out != NULL, err_alloc);

	memcpy(*out, outimg, *outsize);
	ret = 0;

exit:
	if (outimg) {
		MagickRelinquishMemory(outimg);
	}
	if (mw) {
		DestroyMagickWand(mw);
	}
	return ret;
}

const char *g_http_image_op_names[] = {
	"resize",
	"rotate",
	"grayscale",
	"crop",
	NULL
};
image_function g_http_image_op_funcs[] = {
	image_resize,
	image_rotate,
	image_grayscale,
	image_crop,
	NULL
};

image_function image_get_op_func(const char *operation)
{
	int i;

	for (i = 0; g_http_image_op_names[i]; ++i) {
		if (strcmp(g_http_image_op_names[i], operation) == 0) {
			return g_http_image_op_funcs[i];
		}
	}
	return NULL;
}
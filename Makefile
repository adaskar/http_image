PREFIX = /usr
CC = gcc
RM = rm -f
CP = cp

CFLAGS = -Wall -ggdb -D_GNU_SOURCE `pkg-config --cflags MagickWand`
PKGS = 
LIBS = -lc -luv `pkg-config --libs MagickWand` -lcurl

TARGET = http_image

.PHONY: clean all


all: $(TARGET)

$(TARGET): http_image.o curl_service.o image_helper.o
	$(CC) -o $@ $^ $(LIBS)

http_image.o: http_image.c
	$(CC) $(CFLAGS) -c $^ $(PKGS)

image_helper.o: image_helper.c
	$(CC) $(CFLAGS) -c $^ $(PKGS)

curl_service.o: curl_service.c
	$(CC) $(CFLAGS) -c $^ $(PKGS)

clean:
	$(RM) *.o

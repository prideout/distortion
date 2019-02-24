# adapt to your case
VERSION = -7
CC = gcc$(VERSION)
CFLAGS = -std=c99 -Wall -c -Wc++-compat -O3
LIBS = -lX11 -lGL -lpng

OS_NAME= $(shell uname -s)

ifeq ($(OS_NAME), Linux)
        CFLAGS += -DLinux -D_GNU_SOURCE
        LIBS += -lGLEW -lm
endif

DEMOS=\
	OriginalScene \
	TextureWarping-UniformGrid \
	TextureWarping-Gridless \
	TextureWarping-PincushionGrid \
	TextureWarping-NonuniformGrid \
	TiledRendering \
	Checkerboard \
	VertexWarping \
	TessWarping \

SHARED=pez.o bstrlib.o pez.linux.o

run: TextureWarping-Gridless
	./TextureWarping-Gridless

all: $(DEMOS)

define DEMO_RULE
$(1): $(1).o $(1).glsl $(SHARED)
	$(CC) $(1).o $(SHARED) -o $(1) $(LIBS)
endef

$(foreach demo,$(DEMOS),$(eval $(call DEMO_RULE,$(demo))))

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(DEMOS)

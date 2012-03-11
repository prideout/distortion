CC=gcc
CFLAGS=-std=c99 -Wall -c -Wc++-compat -O3
LIBS=-lX11 -lGL -lpng
DEMOS=\
	OriginalScene \
	TextureWarping-UniformGrid \
	TextureWarping-PincushionGrid \
	TextureWarping-NonuniformGrid \
	TiledRendering \
	Checkerboard \
	VertexWarping \
	TessWarping \

SHARED=pez.o bstrlib.o pez.linux.o

run: TessWarping
	./TessWarping

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

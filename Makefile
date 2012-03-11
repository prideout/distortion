CC=gcc
CFLAGS=-std=c99 -Wall -c -Wc++-compat -O3
LIBS=-lX11 -lGL -lpng
DEMOS=\
	Cylinders \
	TextureWarping \
	BetterGrid \
	VertexWarping \
	TessWarping \
	TiledRendering \

SHARED=pez.o bstrlib.o pez.linux.o

run: BetterGrid
	./BetterGrid

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

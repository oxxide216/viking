.RECIPEPREFIX = >

CFLAGS = -Ilibs -Ilibs/winx/include -ggdb
LDFLAGS = -lvulkan -lX11 -lXext -lXrandr -lGL

SRC = $(wildcard src/*.c)

viking: $(SRC) libs/winx/libwinx.a
> cc $(CFLAGS) -o viking $(SRC) libs/winx/libwinx.a $(LDFLAGS)

libs/winx/libwinx.a:
> $(MAKE) -C libs/winx

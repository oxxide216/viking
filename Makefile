.RECIPEPREFIX = >

PLATFORM = LINUX

ifeq ($(PLATFORM), LINUX)
  CC = cc
else ifeq ($(PLATFORM), WINDOWS)
  CC = x86_64-w64-mingw32-gcc
endif
override CFLAGS += -Wall -Wextra -Iinclude -Ilibs -Ilibs/winx/include
ifeq ($(PLATFORM), LINUX)
  override LDFLAGS += -lm -lX11 -lXext -lXrandr -lGL -lvulkan
else ifeq ($(PLATFORM), WINDOWS)
  override LDFLAGS += -lm -lopengl32 -lgdi32 -lwinmm -static -L$(VULKAN_SDK)/Lib -lvulkan-1
endif
BUILD_DIR = build

SRC = $(wildcard src/*.c)
ifeq ($(PLATFORM), LINUX)
  PLATFORM_SRC = $(wildcard src/platform/x11/*.c)
else ifeq ($(PLATFORM), WINDOWS)
  PLATFORM_SRC = $(wildcard src/platform/win32/*.c)
endif
TESTS_SRC = $(wildcard tests/*.c)

ifeq ($(PLATFORM), LINUX)
  OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))
  PLATFORM_OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(PLATFORM_SRC))

  TESTS_EXECS = $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TESTS_SRC))
else ifeq ($(PLATFORM), WINDOWS)
  OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.obj,$(SRC))
  PLATFORM_OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.obj,$(PLATFORM_SRC))

  TESTS_EXECS = $(patsubst tests/%.c,$(BUILD_DIR)/tests/%.exe,$(TESTS_SRC))
endif

ifeq ($(PLATFORM), LINUX)
libviking.a: $(OBJ) $(PLATFORM_OBJ)
> ar rcs libviking.a $(OBJ) $(PLATFORM_OBJ)
else ifeq ($(PLATFORM), WINDOWS)
libviking.lib: $(OBJ) $(PLATFORM_OBJ)
> x86_64-w64-mingw32-ar rcs libviking.lib $(OBJ) $(PLATFORM_OBJ)
endif

tests: $(TESTS_EXECS)

$(BUILD_DIR)/%.o: src/%.c
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/tests/%: tests/%.c libviking.a libs/winx/libwinx.a
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -o $@ $< libviking.a libs/winx/libwinx.a $(LDFLAGS)

$(BUILD_DIR)/%.obj: src/%.c
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/tests/%.exe: tests/%.c libviking.lib libs/winx/libwinx.lib
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -o $@ $< libviking.lib libs/winx/libwinx.lib $(LDFLAGS)

libs/winx/libwinx.a:
> $(MAKE) -C libs/winx -B

libs/winx/libwinx.lib:
> $(MAKE) -C libs/winx -B

clean:
> rm -rf $(BUILD_DIR) libviking.a libviking.lib

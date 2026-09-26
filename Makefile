CC       := i686-w64-mingw32-gcc
CXX      := i686-w64-mingw32-g++
WARN     := -Wall -Wextra -Werror
CFLAGS   := -std=c11 -O2 $(WARN)
# The Mac build is clang with SSE maths; x87 would round the renderer's floats differently.
CXXFLAGS := -std=c++17 -O2 $(WARN) -fno-exceptions -fno-rtti -msse2 -mfpmath=sse
LDFLAGS  := -shared -static-libgcc -Wl,--enable-stdcall-fixup
GL_LIBS  := -lopengl32 -lgdi32 -luser32

BUILD    := build

# libd2's d2-util C ABI (MMPX, frame keys, HD pack lookup), built from the pinned submodule as a
# static library for i686 mingw. LIBD2 may point at another checkout for local work. Zig's cache
# and output go under build/, so the submodule's tree stays clean.
LIBD2       ?= third_party/libd2
ZIG         ?= zig
D2UTIL_DIR  := $(BUILD)/libd2
D2UTIL_LIB  := $(D2UTIL_DIR)/lib/d2util.lib

COMMON_SRC   := src/common/log.c src/common/build.c
RENDERER_C   := src/D2OpenGL/dllmain.c src/D2OpenGL/crashlog.c src/D2OpenGL/launch.c src/D2OpenGL/d2loader.c \
                src/D2OpenGL/install/patch.c src/D2OpenGL/install/install_114d.c \
                src/D2OpenGL/install/install_dllera.c
RENDERER_CXX := $(wildcard src/D2OpenGL/*.cpp src/D2OpenGL/*/*.cpp)

RENDERER_OBJ := $(patsubst %.c,$(BUILD)/%.o,$(COMMON_SRC) $(RENDERER_C)) \
                $(patsubst %.cpp,$(BUILD)/%.o,$(RENDERER_CXX))

all: $(BUILD)/D2OpenGL.dll

$(BUILD)/D2OpenGL.dll: $(RENDERER_OBJ) src/D2OpenGL/D2OpenGL.def $(D2UTIL_LIB)
	$(CXX) -o $@ $^ $(LDFLAGS) -static-libstdc++ $(GL_LIBS)

# Zig decides what is stale; the install leaves the archive untouched when nothing changed.
$(D2UTIL_LIB): FORCE
	@test -f $(LIBD2)/packages/util/build.zig || { echo "libd2 not found at $(LIBD2): git submodule update --init"; exit 1; }
	cd $(LIBD2)/packages/util && $(ZIG) build -Dtarget=x86-windows-gnu -Doptimize=ReleaseFast \
		--cache-dir $(abspath $(BUILD))/zig-cache --prefix $(abspath $(D2UTIL_DIR))

# The objects include d2util.h, which the library build installs.
$(RENDERER_OBJ): | $(D2UTIL_LIB)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(D2UTIL_DIR)/include -MMD -MP -c -o $@ $<

clean:
	rm -rf build

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all clean FORCE

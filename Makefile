CC       := i686-w64-mingw32-gcc
CXX      := i686-w64-mingw32-g++
WARN     := -Wall -Wextra -Werror
CFLAGS   := -std=c11 -O2 $(WARN)
# The Mac build is clang with SSE maths; x87 would round the renderer's floats differently.
CXXFLAGS := -std=c++17 -O2 $(WARN) -fno-exceptions -fno-rtti -msse2 -mfpmath=sse
LDFLAGS  := -shared -static-libgcc -Wl,--enable-stdcall-fixup
GL_LIBS  := -lopengl32 -lgdi32 -luser32

BUILD    := build

COMMON_SRC   := src/common/log.c src/common/build.c
RENDERER_C   := src/D2OpenGL/dllmain.c src/D2OpenGL/launch.c src/D2OpenGL/d2loader.c \
                src/D2OpenGL/install/patch.c src/D2OpenGL/install/install_114d.c \
                src/D2OpenGL/install/install_dllera.c
RENDERER_CXX := $(wildcard src/D2OpenGL/*.cpp src/D2OpenGL/*/*.cpp)

RENDERER_OBJ := $(patsubst %.c,$(BUILD)/%.o,$(COMMON_SRC) $(RENDERER_C)) \
                $(patsubst %.cpp,$(BUILD)/%.o,$(RENDERER_CXX))

all: $(BUILD)/D2OpenGL.dll

$(BUILD)/D2OpenGL.dll: $(RENDERER_OBJ) src/D2OpenGL/D2OpenGL.def
	$(CXX) -o $@ $^ $(LDFLAGS) -static-libstdc++ $(GL_LIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	rm -rf build

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all clean

CC = clang++
SLC = glslc
CFLAGS = -std=c++17 -march=native -Ofast -Wall -Wextra
LDLIBS = -lglfw -lvulkan
SOURCES = space.cpp
VSHADES = shaders/shader.vert
FSHADES = shaders/shader.frag
OBJECTS = space.o
VMODS = shaders/vert.spv
FMODS = shaders/frag.spv

all: $(OBJECTS) $(VMODS) $(FMODS)

$(OBJECTS): $(SOURCES)
	$(CC) $< -o $@ $(CFLAGS) $(LDLIBS)

$(VMODS): $(VSHADES)
	$(SLC) $< -o $@ -O

$(FMODS): $(FSHADES)
	$(SLC) $< -o $@ -O

clean:
	rm $(OBJECTS) $(VMODS) $(FMODS)

CC = clang++
SLC = glslc
CFLAGS = -std=c++17 -march=native -Ofast -Wall -Wextra
LDLIBS = -lglfw -lvulkan
SOURCES = Space.cpp
VSHADES = shaders/shader.vert
FSHADES = shaders/shader.frag
OBJECTS = Space.o
VMODS = data/vert.spv
FMODS = data/frag.spv

all: $(OBJECTS) $(VMODS) $(FMODS)

$(OBJECTS): $(SOURCES)
	$(CC) $< -o $@ $(CFLAGS) $(LDLIBS)

$(VMODS): $(VSHADES)
	$(SLC) $< -o $@ -O

$(FMODS): $(FSHADES)
	$(SLC) $< -o $@ -O

clean:
	rm $(OBJECTS) $(VMODS) $(FMODS)

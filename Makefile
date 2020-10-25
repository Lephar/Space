CC = clang++
SLC = glslc
CFLAGS = -std=c++17 -O2 -Wall -Wextra
LDLIBS = -lglfw -lvulkan
SOURCES = triangle.cpp
VSHADES = shaders/shader.vert
FSHADES = shaders/shader.frag
OBJECTS = triangle
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

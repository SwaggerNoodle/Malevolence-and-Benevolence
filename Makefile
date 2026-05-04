PROGRAM=Malevolence_and_Benevolence.exe
OBJECTS=main.o
SHADERS=shader.vert.spv shader.frag.spv


CC=gcc
CFLAGS=-Wall -I/mingw64/include
LDFLAGS=-L/mingw64/lib -lSDL3 -lvulkan-1 -lm -mwindows

GLSLC=glslc

all: $(PROGRAM) $(SHADERS)

$(PROGRAM): $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

shader.vert.spv: shader.vert
	$(GLSLC) shader.vert -o shader.vert.spv

shader.frag.spv: shader.frag
	$(GLSLC) shader.frag -o shader.frag.spv

clean:
	rm -f $(OBJECTS) $(PROGRAM) $(SHADERS)



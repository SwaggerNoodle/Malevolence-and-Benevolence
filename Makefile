PROGRAM=Malevolence_and_Benevolence.exe
OBJECTS=main.o rendering.o helpers.o physics.o
SHADERS=shader.vert.spv shader.frag.spv


CC=gcc
CFLAGS=-Wall -I/mingw64/include
LDFLAGS=-L/mingw64/lib -lSDL3 -lvulkan-1 -lm -mwindows

GLSLC=glslc



all: $(PROGRAM) $(SHADERS)

$(PROGRAM): $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LDFLAGS)

main.o: main.c game.h common.h rendering.h helpers.h physics.h
	$(CC) $(CFLAGS) -c main.c

rendering.o: rendering.c rendering.h helpers.h game.h common.h
	$(CC) $(CFLAGS) -c rendering.c

helpers.o: helpers.c helpers.h game.h common.h rendering.h
	$(CC) $(CFLAGS) -c helpers.c

physics.o: physics.c physics.h game.h common.h
		$(CC) $(CFLAGS) -c physics.c

shader.vert.spv: shader.vert
	$(GLSLC) shader.vert -o shader.vert.spv

shader.frag.spv: shader.frag
	$(GLSLC) shader.frag -o shader.frag.spv

clean:
	rm -f $(OBJECTS) $(PROGRAM) $(SHADERS)



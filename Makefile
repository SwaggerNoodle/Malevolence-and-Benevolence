PROGRAM=Malevolence_and_Benevolence.exe
OBJECTS=main.o


CC=gcc
CFLAGS=-Wall -I/mingw64/include
LDFLAGS=-L/mingw64/lib -lSDL3 -lopengl32 -mwindows


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LDFLAGS)

main.o: main.c
	$(CC) $(CLFLAGS) -c main.c

clean:
	rm -f $(OBJECTS) $(PROGRAM)



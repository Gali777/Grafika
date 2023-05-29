all:
	gcc src/main.c -o beadando.exe -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lopengl32 -lglu32 -lglut32 -Wall -g -std=c99 -Iinclude/ src/draw.c src/info.c src/load.c src/model.c src/transform.c
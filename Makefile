all:
	gcc -I src/include -L src/lib -o main main.c -lmingw32 -lSDL3 -lSDL3_ttf
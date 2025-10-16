all:
	gcc -I src/include -L src/lib -o main main.c audio_manager.c -lmingw32 -lSDL3 -lSDL3_ttf
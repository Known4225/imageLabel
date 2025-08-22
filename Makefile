CC = gcc

win:
	$(CC) imageLabel.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -DDEBUGGING_FLAG -Wall -o imageLabel.exe
winrel:
	$(CC) imageLabel.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -O3 -o imageLabel.exe
winlib: singlefile
	cp turtle.h turtlelib.c
	$(CC) turtlelib.c -c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DTURTLE_IMPLEMENTATION -DTURTLE_ENABLE_TEXTURES -DOS_WINDOWS -O3 -o Windows/turtletextures.lib
	rm turtlelib.c
singlefile:
	$(CC) deploy.c -o deploy.exe
	./deploy.exe
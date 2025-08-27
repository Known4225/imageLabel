CC = gcc

win:
	$(CC) imageLabel.c -L./Windows -lstb_image -lstb_image_resize2 -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -DDEBUGGING_FLAG -Wall -o imageLabel.exe
winrel:
	$(CC) imageLabel.c -L./Windows -lstb_image -lstb_image_resize2 -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DOS_WINDOWS -O3 -o imageLabel.exe
winlib: singlefile
	cp turtle.h turtlelib.c
	$(CC) turtlelib.c -c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -lwsock32 -lWs2_32 -DTURTLE_IMPLEMENTATION -DTURTLE_ENABLE_TEXTURES -DOS_WINDOWS -O3 -o Windows/turtletextures.lib
	rm turtlelib.c
singlefile:
	$(CC) deploy.c -o deploy.exe
	./deploy.exe
stbi:
	cp include/stb_image.h include/stb_image.c
	$(CC) include/stb_image.c -c -DSTB_IMAGE_IMPLEMENTATION -O3 -o Windows/stb_image.lib
	rm include/stb_image.c
	cp include/stb_image_resize2.h include/stb_image_resize2.c
	$(CC) include/stb_image_resize2.c -c -DSTB_IMAGE_RESIZE_IMPLEMENTATION -O3 -o Windows/stb_image_resize2.lib
	rm include/stb_image_resize2.c
compilar:
	gcc Wizard_of_Wor.c `pkg-config --cflags --libs sdl2` -o WOW
	./WOW

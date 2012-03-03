ALL=ungui

all: ${ALL}

ungui: ungui.c arrows.c callbacks.c
	gcc -g -o ungui ungui.c arrows.c callbacks.c `pkg-config --cflags --libs gtk+-3.0` -Wl,--export-dynamic -lm -lX11 -lXrender

clean:
	rm -f *.o ${ALL}

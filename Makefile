ALL=ungui
SRC=arrows.c data.c programs.c ungui.c objects.c

all: ${ALL}

ungui: ${SRC}
	gcc -g -O0 -o ungui ${SRC} `pkg-config --cflags --libs gtk+-2.0` -Wl,--export-dynamic -lm -lX11 -lXrender

clean:
	rm -f *.o ${ALL}

ALL=ungui

all: ${ALL}

ungui: ungui.c arrows.c
	gcc -g -o ungui ungui.c arrows.c `pkg-config --cflags --libs gtk+-3.0` -Wl,--export-dynamic

clean:
	rm -f *.o ${ALL}

ALL=ungui

all: ${ALL}

ungui: ungui.c
	gcc -g -o ungui ungui.c `pkg-config --cflags --libs gtk+-2.0` -Wl,--export-dynamic

clean:
	rm -f *.o ${ALL}

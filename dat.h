typedef struct _Object Object;
struct _Object {
	GtkWidget *window;
	GList *nexts;
	Object *prev;
	GtkWidget *data;
	GtkWidget *undo;
	GtkWidget *redo;
	char *cmd;
	char *output;
	int outlen;
	int stdin;
	int stdout;
	GList *undohist;
	GList *undoptr;
};

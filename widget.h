extern int widget_active;

typedef void (*widget_keyhandler_fn)( int key );

extern widget_keyhandler_fn widget_keyhandler;

int widget_init( void );
int widget_end( void );

const char* widget_selectfile(void);

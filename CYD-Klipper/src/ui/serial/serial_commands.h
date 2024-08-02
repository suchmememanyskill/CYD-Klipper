#include <WString.h>

namespace serial_console {

typedef struct _HANDLER {
    const char* name;
    void (* function)(String argv[]);
    int argc;
} HANDLER;

extern HANDLER commandHandlers[];

void help(String argv[]);
void reset(String argv[]);
void settings(String argv[]);
void sets(String argv[]);
void erase(String argv[]);
void key(String argv[]);
void touch(String argv[]);
void ssid(String argv[]);
void ip(String argv[]);
void rotation(String argv[]);
void brightness(String argv[]);
void printer(String argv[]);
void debug(String argv[]);
void echo(String argv[]);

int find_command(String cmd);
}
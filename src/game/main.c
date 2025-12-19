#include "common.h"
#include <stdio.h>
#include <string.h>

char g_nick[32] = "PLAYER";
int game_main(void);

int main(int argc, char **argv) {
    if (argc > 1 && argv[1] && argv[1][0]) {
        snprintf(g_nick, sizeof(g_nick), "%.31s", argv[1]);
    }
    return game_main();
}




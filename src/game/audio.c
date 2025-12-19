#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char* pick_alsa_device(void) {
    const char *env = getenv("ALSA_DEVICE");
    if (env && *env) return env;
    return "sysdefault:Headphones";
}

static pid_t g_child = -1;

bool audio_init(void) {
    return true;
}

static void stop_child_if_any(void) {
    if (g_child > 0) {
        kill(g_child, SIGTERM);
        int status = 0;
        (void)waitpid(g_child, &status, 0);
        g_child = -1;
    }
}

void audio_close(void) {
    stop_child_if_any();
}

static bool audio_play_music(const char *wav_path) {
    stop_child_if_any();
    if (!wav_path || !*wav_path) return false;

    char abswav[PATH_MAX];
    if (wav_path[0] == '/') {
        size_t n = strlen(wav_path);
        if (n >= sizeof(abswav)) return false;
        memcpy(abswav, wav_path, n + 1);
    } else {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) {
            strncpy(cwd, ".", sizeof(cwd));
            cwd[sizeof(cwd)-1] = '\0';
        }
        size_t need = strlen(cwd) + 1 + strlen(wav_path) + 1;
        if (need > sizeof(abswav)) return false;
        snprintf(abswav, sizeof(abswav), "%s/%s", cwd, wav_path);
    }

    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        const char *dev = pick_alsa_device();
        execlp("aplay", "aplay", "-q", "-D", dev, abswav, (char*)NULL);
        _exit(127);
    }
    g_child = pid;
    return true;
}

bool audio_play_bgm(const char *wav_path) {
    return audio_play_music(wav_path);
}

void audio_stop(void) {
    stop_child_if_any();
}




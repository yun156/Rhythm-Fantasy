#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool audio_init(void);
bool audio_play_bgm(const char *wav_path);
void audio_stop(void);
void audio_close(void);

#ifdef __cplusplus
}
#endif




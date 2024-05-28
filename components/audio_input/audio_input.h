#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t init_microphone(void);
    esp_err_t record_wav(uint32_t rec_time);

#ifdef __cplusplus
}
#endif

#endif

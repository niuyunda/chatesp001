#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2s_std.h"

    extern i2s_chan_handle_t tx_handle;
    esp_err_t init_speaker(void);

#ifdef __cplusplus
}
#endif

#endif

#include "capture.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct data {
    struct pw_thread_loop *loop;
    struct pw_stream *stream;
    AudioBuffer_t latest_buffer;
    int channels;
};

static void on_process(void *userdata) {
    struct data *d = (struct data *)userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    float *samples;
    uint32_t n_samples;

    if ((b = pw_stream_dequeue_buffer(d->stream)) == NULL) {
        pw_log_warn("out of buffers");
        return;
    }

    buf = b->buffer;
    if ((samples = (float *)buf->datas[0].data) == NULL)
        return;

    n_samples = buf->datas[0].chunk->size / sizeof(float);

    
    uint32_t channels = d->channels;
    if (d->latest_buffer.buffer) {
        free(d->latest_buffer.buffer);
    }
    d->latest_buffer.buffer = malloc(buf->datas[0].chunk->size);
    memcpy(d->latest_buffer.buffer, samples, buf->datas[0].chunk->size);
    d->latest_buffer.num_samples = n_samples / channels;
    d->latest_buffer.channels = channels;
    d->latest_buffer.sample_rate = 48000;

    pw_stream_queue_buffer(d->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

static struct data global_data;

int OD_Capture_Init(int channels) {
    pw_init(NULL, NULL);
    global_data.loop = pw_thread_loop_new("OD_Capture_Loop", NULL);
    
    
    
    
    struct pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "DSP",
        PW_KEY_NODE_NAME, "oneDirectionCore-Capture",
        "stream.capture.sink", "true",
        NULL);
    
    global_data.stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(global_data.loop),
        "oneDirectionCore-Capture",
        props,
        &stream_events,
        &global_data);

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *params[1];

    
    global_data.channels = channels;
    printf("[Capture Linux] Mapping PipeWire stream for %d channels, 48000Hz\n", channels);
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_F32,
            .rate = 48000,
            .channels = channels));
    fflush(stdout);

    pw_stream_connect(global_data.stream,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      PW_STREAM_FLAG_AUTOCONNECT |
                      PW_STREAM_FLAG_MAP_BUFFERS |
                      PW_STREAM_FLAG_RT_PROCESS,
                      params, 1);

    return 1;
}

int OD_Capture_Start(void) {
    printf("[OD Core Linux] Starting PipeWire capture thread...\n");
    pw_thread_loop_start(global_data.loop);
    return 1;
}

void OD_Capture_Stop(void) {
    printf("[OD Core Linux] Stopping PipeWire capture thread...\n");
    pw_thread_loop_stop(global_data.loop);
    if (global_data.loop) {
        pw_thread_loop_destroy(global_data.loop);
        global_data.loop = NULL;
    }
}

AudioBuffer_t* OD_Capture_GetLatestBuffer(void) {
    if (global_data.latest_buffer.buffer == NULL) return NULL;
    return &global_data.latest_buffer;
}

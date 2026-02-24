#ifndef OD_DSP_WINDOWS_H
#define OD_DSP_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../driver/capture_windows.h"

typedef struct {
    float azimuth_angle;
    float distance;
    int signature_match_id;
    float confidence;
    int sound_type;      
} SoundEntity_t;

typedef struct {
    SoundEntity_t entities[10];
    int entity_count;
} SpatialData_t;


__declspec(dllexport) SpatialData_t OD_DSP_ProcessBuffer(const AudioBuffer_t* buffer, float sensitivity, float separation);
__declspec(dllexport) int OD_DSP_LoadSignature(int id, const char* file_path);


#ifdef __cplusplus
}
#endif

#endif 

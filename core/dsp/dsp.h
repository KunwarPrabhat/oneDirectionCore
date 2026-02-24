#ifndef OD_DSP_H
#define OD_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../driver/capture.h"


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




SpatialData_t OD_DSP_ProcessBuffer(const AudioBuffer_t* buffer, float sensitivity, float separation);


int OD_DSP_LoadSignature(int id, const char* file_path);

#ifdef __cplusplus
}
#endif

#endif 

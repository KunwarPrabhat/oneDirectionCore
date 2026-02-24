#ifndef OD_CAPTURE_H
#define OD_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>


typedef struct {
    float* buffer;
    uint32_t num_samples; 
    uint32_t channels;    
    uint32_t sample_rate;
} AudioBuffer_t;




int OD_Capture_Init(int channels);


int OD_Capture_Start(void);


void OD_Capture_Stop(void);



AudioBuffer_t* OD_Capture_GetLatestBuffer(void);

#ifdef __cplusplus
}
#endif

#endif 

#ifndef OD_CAPTURE_WINDOWS_H
#define OD_CAPTURE_WINDOWS_H

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


__declspec(dllexport) int OD_Capture_Init(int channels);
__declspec(dllexport) int OD_Capture_Start(void);
__declspec(dllexport) void OD_Capture_Stop(void);
__declspec(dllexport) AudioBuffer_t* OD_Capture_GetLatestBuffer(void);


#ifdef __cplusplus
}
#endif

#endif 

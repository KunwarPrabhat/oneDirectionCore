#ifndef OD_CLASSIFIER_H
#define OD_CLASSIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    SOUND_UNKNOWN = 0,
    SOUND_FOOTSTEP,
    SOUND_AR,           // Assault Rifle (M416, AKM, SCAR-L)
    SOUND_SMG,          // SMG (UMP, Vector, UZI)
    SOUND_SR,           // Sniper Rifle (AWM, Kar98k, M24)
    SOUND_DMR,          // DMR (Mini14, SKS, SLR)
    SOUND_GRENADE,      // Frag grenade explosion
    SOUND_SMOKE,        // Smoke grenade pop/hiss
    SOUND_VEHICLE,      // Cars, bikes, boats
    SOUND_TYPE_COUNT
} SoundType_t;


typedef struct {
    SoundType_t type;
    float confidence;    
} ClassResult_t;


typedef struct {
    float energy;           
    float spectral_centroid; 
    float spectral_spread;   
    float high_freq_ratio;   
    float low_freq_ratio;    
    float mid_freq_ratio;    
    float transient;         
    float zero_crossing_rate; 
} SpectralFeatures_t;


typedef struct {
    const char* name;       
    int enabled;            
    
} Preset_t;




void OD_Classifier_Init(void);


void OD_Classifier_SetPreset(const char* preset_name);


SpectralFeatures_t OD_Classifier_ExtractFeatures(const float* left, const float* right,
                                                   uint32_t num_samples, uint32_t sample_rate);


ClassResult_t OD_Classifier_Classify(const SpectralFeatures_t* features);


const char* OD_Classifier_TypeName(SoundType_t type);




#ifdef __cplusplus
}
#endif

#endif 

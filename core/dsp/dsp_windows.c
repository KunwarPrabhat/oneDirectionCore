#include "dsp_windows.h"
#include "classifier_windows.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PI 3.14159265358979323846f

typedef struct {
    float x;
    float y;
} Vector2D;

static float dft_bin_energy(const float* samples, uint32_t n, uint32_t bin, uint32_t total_bins) {
    float real = 0.0f, imag = 0.0f;
    float freq = (float)bin / (float)total_bins;
    for (uint32_t i = 0; i < n; i++) {
        float angle = 2.0f * PI * freq * (float)i;
        real += samples[i] * cosf(angle);
        imag += samples[i] * sinf(angle);
    }
    return (real * real + imag * imag) / (float)n;
}

#define NUM_BANDS 4
static const uint32_t band_start[] = { 1, 6, 21, 85 };
static const uint32_t band_end[]   = { 6, 21, 85, 256 };
#define FFT_SIZE 512

SpatialData_t OD_DSP_ProcessBuffer(const AudioBuffer_t* buffer, float sensitivity, float separation) {
    SpatialData_t result;
    memset(&result, 0, sizeof(SpatialData_t));
    
    if (buffer == NULL || buffer->buffer == NULL || buffer->num_samples == 0 || buffer->channels < 1) {
        return result; 
    }

    uint32_t n = buffer->num_samples;
    if (n > FFT_SIZE) n = FFT_SIZE;

    float left[FFT_SIZE];
    float right[FFT_SIZE];
    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));

    
    for (uint32_t i = 0; i < n; i++) {
        if (buffer->channels == 1) {
            left[i] = buffer->buffer[i];
            right[i] = buffer->buffer[i];
        } else if (buffer->channels == 2) {
            left[i] = buffer->buffer[i * 2 + 0];
            right[i] = buffer->buffer[i * 2 + 1];
        } else if (buffer->channels >= 6) {
            
            float l = buffer->buffer[i * buffer->channels + 0];
            float r = buffer->buffer[i * buffer->channels + 1];
            float c = buffer->buffer[i * buffer->channels + 2];
            float sl = buffer->buffer[i * buffer->channels + 4];
            float sr = buffer->buffer[i * buffer->channels + 5];
            
            left[i] = l + (c * 0.707f) + sl;
            right[i] = r + (c * 0.707f) + sr;
            
            if (buffer->channels >= 8) {
                float bl = buffer->buffer[i * buffer->channels + 6];
                float br = buffer->buffer[i * buffer->channels + 7];
                left[i] += bl;
                right[i] += br;
            }
        } else {
            
            left[i] = buffer->buffer[i * buffer->channels + 0];
            right[i] = buffer->buffer[i * buffer->channels + 1];
        }
    }

    SpectralFeatures_t features = OD_Classifier_ExtractFeatures(left, right, n, buffer->sample_rate);
    ClassResult_t class_result = OD_Classifier_Classify(&features);

    if (sensitivity < 0.01f) return result;

    float min_thresh = 0.00001f;
    float max_thresh = 0.5f;
    float threshold = max_thresh * powf(min_thresh / max_thresh, sensitivity);

    
    static float peak_energy = 0;
    static int peak_counter = 0;
    float current_max = 0;
    for(uint32_t i=0; i<n; i++) {
        float e = left[i]*left[i] + right[i]*right[i];
        if(e > current_max) current_max = e;
    }
    if (current_max > peak_energy) peak_energy = current_max;
    if (++peak_counter >= 100) {
        if (peak_energy > 0) {
            printf("[DSP Windows] Peak Energy: %.6f, Threshold: %.6f, Entities: %d\n", peak_energy, threshold, result.entity_count);
            fflush(stdout);
        }
        peak_energy = 0;
        peak_counter = 0;
    }

    for (int band = 0; band < NUM_BANDS && result.entity_count < 10; band++) {
        float energy_l = 0.0f;
        float energy_r = 0.0f;

        uint32_t step = (band_end[band] - band_start[band]);
        if (step < 1) step = 1;
        if (step > 8) step = step / 4; 
        
        for (uint32_t bin = band_start[band]; bin < band_end[band]; bin += (step > 1 ? step : 1)) {
            energy_l += dft_bin_energy(left, n, bin, FFT_SIZE);
            energy_r += dft_bin_energy(right, n, bin, FFT_SIZE);
        }

        float total = energy_l + energy_r;
        if (total < threshold) continue;

        float pan = 0.0f;
        if (total > 0.0f) {
            pan = (energy_r - energy_l) / total;
        }

        float azimuth = pan * 90.0f;
        if (azimuth < 0) azimuth += 360.0f;

        float avg = total * 0.5f;
        float distance = 1.0f / (1.0f + sqrtf(avg) * 15.0f);
        if (distance < 0.1f) distance = 0.1f;
        if (distance > 0.95f) distance = 0.95f;

        SoundEntity_t* entity = &result.entities[result.entity_count];
        entity->azimuth_angle = azimuth;
        entity->distance = distance;
        entity->confidence = fabsf(pan);
        entity->signature_match_id = band;
        entity->sound_type = class_result.type;

        bool merged = false;
        for (int e = 0; e < result.entity_count; e++) {
            float diff = result.entities[e].azimuth_angle - azimuth;
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;

            if (fabsf(diff) < separation) {
                result.entities[e].azimuth_angle = (result.entities[e].azimuth_angle + azimuth) * 0.5f;
                if (distance < result.entities[e].distance)
                    result.entities[e].distance = distance;
                merged = true;
                break;
            }
        }

        if (!merged) {
            result.entity_count++;
        }
    }
    
    return result;
}

int OD_DSP_LoadSignature(int id, const char* file_path) {
    printf("[OD Core Windows] Loading DSP Signature ID %d from %s...\n", id, file_path);
    return 1;
}

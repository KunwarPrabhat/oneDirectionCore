#include "capture.h"

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const CLSID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const IID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x3C, 0x17, 0xDB}};
const IID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c2c, {0x81, 0x7E, 0xAD, 0x74, 0x97, 0x81, 0x11, 0x45}};
const IID IID_IAudioCaptureClient = {0xC8ADBD64, 0xE71E, 0x48a0, {0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17}};

static IMMDeviceEnumerator *pEnumerator = NULL;
static IMMDevice *pDevice = NULL;
static IAudioClient *pAudioClient = NULL;
static IAudioCaptureClient *pCaptureClient = NULL;
static WAVEFORMATEX *pFormat = NULL;
static AudioBuffer_t latest_buffer;
static HANDLE capture_thread = NULL;
static volatile int running = 0;

static DWORD WINAPI CaptureThreadProc(LPVOID lpParam) {
    (void)lpParam;

    while (running) {
        UINT32 packetLength = 0;
        HRESULT hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        if (FAILED(hr)) { Sleep(1); continue; }

        while (packetLength > 0) {
            BYTE *pData = NULL;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFrames, &flags, NULL, NULL);
            if (FAILED(hr)) break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFrames > 0) {
                UINT32 channels = pFormat->nChannels;
                UINT32 byte_count = numFrames * channels * sizeof(float);

                
                if (latest_buffer.buffer) {
                    free(latest_buffer.buffer);
                }
                latest_buffer.buffer = (float*)malloc(byte_count);

                if (pFormat->wBitsPerSample == 32) {
                    
                    memcpy(latest_buffer.buffer, pData, byte_count);
                } else if (pFormat->wBitsPerSample == 16) {
                    
                    short *src = (short*)pData;
                    for (UINT32 i = 0; i < numFrames * channels; i++) {
                        latest_buffer.buffer[i] = src[i] / 32768.0f;
                    }
                }

                latest_buffer.num_samples = numFrames;
                latest_buffer.channels = channels;
                latest_buffer.sample_rate = pFormat->nSamplesPerSec;
            }

            pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFrames);

            hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
            if (FAILED(hr)) break;
        }

        Sleep(5); 
    }

    return 0;
}

int OD_Capture_Init(int channels) {
    (void)channels; 
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if (FAILED(hr)) { printf("[OD Win] Failed to create device enumerator\n"); return 0; }

    
    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, &pDevice);
    if (FAILED(hr)) { printf("[OD Win] Failed to get default render device\n"); return 0; }

    hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) { printf("[OD Win] Failed to activate audio client\n"); return 0; }

    hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pFormat);
    if (FAILED(hr)) { printf("[OD Win] Failed to get mix format\n"); return 0; }

    printf("[OD Win] Audio format: %d ch, %d Hz, %d bit\n",
           pFormat->nChannels, (int)pFormat->nSamplesPerSec, pFormat->wBitsPerSample);

    
    hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED,
                                          AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pFormat, NULL);
    if (FAILED(hr)) { printf("[OD Win] Failed to initialize audio client (0x%08lx)\n", hr); return 0; }

    hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient, (void**)&pCaptureClient);
    if (FAILED(hr)) { printf("[OD Win] Failed to get capture client\n"); return 0; }

    memset(&latest_buffer, 0, sizeof(latest_buffer));
    printf("[OD Win] WASAPI loopback initialized successfully\n");

    return 1;
}

int OD_Capture_Start(void) {
    printf("[OD Core Windows] Starting WASAPI loopback capture...\n");
    if (pAudioClient) {
        pAudioClient->lpVtbl->Start(pAudioClient);
        running = 1;
        capture_thread = CreateThread(NULL, 0, CaptureThreadProc, NULL, 0, NULL);
    }
    return 1;
}

void OD_Capture_Stop(void) {
    printf("[OD Core Windows] Stopping WASAPI capture...\n");
    running = 0;
    if (capture_thread) {
        WaitForSingleObject(capture_thread, 2000);
        CloseHandle(capture_thread);
        capture_thread = NULL;
    }
    if (pAudioClient) pAudioClient->lpVtbl->Stop(pAudioClient);
    if (pCaptureClient) pCaptureClient->lpVtbl->Release(pCaptureClient);
    if (pAudioClient) pAudioClient->lpVtbl->Release(pAudioClient);
    if (pDevice) pDevice->lpVtbl->Release(pDevice);
    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    if (pFormat) CoTaskMemFree(pFormat);
    if (latest_buffer.buffer) free(latest_buffer.buffer);
    CoUninitialize();
}

AudioBuffer_t* OD_Capture_GetLatestBuffer(void) {
    if (latest_buffer.buffer == NULL) return NULL;
    return &latest_buffer;
}
#else

int OD_Capture_Init(int channels) { (void)channels; return 0; }
int OD_Capture_Start(void) { return 0; }
void OD_Capture_Stop(void) {}
AudioBuffer_t* OD_Capture_GetLatestBuffer(void) { return NULL; }
#endif

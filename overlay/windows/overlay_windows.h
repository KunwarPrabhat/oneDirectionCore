#ifndef OD_OVERLAY_WINDOWS_H
#define OD_OVERLAY_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

bool OD_Overlay_Windows_Init();


void OD_Overlay_Windows_Tick();

void OD_Overlay_Windows_Start();
void OD_Overlay_Windows_Stop();

#ifdef __cplusplus
}
#endif

#endif 

#ifndef OD_OVERLAY_LINUX_H
#define OD_OVERLAY_LINUX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


bool OD_Overlay_Linux_Init();


void OD_Overlay_Linux_Tick();


void OD_Overlay_Linux_Start();


void OD_Overlay_Linux_Stop();

#ifdef __cplusplus
}
#endif

#endif 

#ifndef OSD_RADAR_H
#define OSD_RADAR_H

#include "../core/dsp/dsp.h"


void DrawRadarHUD(SpatialData_t* data, bool is_fullscreen, float global_opacity, float dot_opacity, int max_entities, float range, int position);

#endif 

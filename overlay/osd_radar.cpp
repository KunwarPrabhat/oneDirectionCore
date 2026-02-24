#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "osd_radar.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

static float sweep_angle = 0.0f;

#define MAX_BLIPS 10
static float blip_azimuth[MAX_BLIPS] = {0};
static float blip_distance[MAX_BLIPS] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
static float blip_alpha[MAX_BLIPS] = {0};
static int blip_type[MAX_BLIPS] = {0}; 

static void DrawSoundIcon(ImDrawList* dl, ImVec2 pos, float size, int type, ImU32 col, int ba) {
    float s = size;
    if (type == 1) { 
        ImVec2 pts[6] = {
            {pos.x - s*0.4f, pos.y + s*0.6f}, {pos.x + s*0.4f, pos.y + s*0.6f},
            {pos.x + s*0.6f, pos.y - s*0.1f}, {pos.x + s*0.2f, pos.y - s*0.6f},
            {pos.x - s*0.2f, pos.y - s*0.6f}, {pos.x - s*0.6f, pos.y - s*0.1f}
        };
        dl->AddConvexPolyFilled(pts, 6, col);
    } else if (type == 2 || type == 3) { 
        dl->AddTriangleFilled({pos.x, pos.y - s*0.8f}, {pos.x - s*0.6f, pos.y + s*0.6f}, {pos.x + s*0.6f, pos.y + s*0.6f}, col);
        dl->AddRectFilled({pos.x - s*0.2f, pos.y + s*0.6f}, {pos.x + s*0.2f, pos.y + s*0.9f}, col);
    } else if (type == 4 || type == 5) { 
        dl->AddQuadFilled({pos.x, pos.y - s}, {pos.x + s, pos.y}, {pos.x, pos.y + s}, {pos.x - s, pos.y}, col);
        dl->AddCircle(pos, s*0.5f, IM_COL32(255, 255, 255, (int)(ba * 0.5f)), 12, 1.0f);
    } else if (type == 8) { 
        dl->AddRectFilled({pos.x - s*0.8f, pos.y - s*0.4f}, {pos.x + s*0.8f, pos.y + s*0.4f}, col, 2.0f);
        dl->AddRectFilled({pos.x + s*0.3f, pos.y - s*0.7f}, {pos.x + s*0.7f, pos.y - s*0.4f}, col);
    } else if (type == 6) { 
        const float PI_LOC = 3.14159265f;
        dl->AddCircleFilled(pos, s*1.2f, col);
        for(int j=0; j<8; j++) {
            float a = j * (PI_LOC/4.0f);
            dl->AddLine(pos, {pos.x + cosf(a)*s*1.8f, pos.y + sinf(a)*s*1.8f}, col, 1.5f);
        }
    } else { 
        dl->AddCircleFilled(pos, s*0.8f, col);
    }
}


void DrawRadarHUD(SpatialData_t* data, bool is_fullscreen, float global_opacity, float radar_opacity, float dot_opacity, int max_entities, float range, int position, float radar_size) {
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                            ImGuiWindowFlags_NoInputs | 
                            ImGuiWindowFlags_NoNav | 
                            ImGuiWindowFlags_NoBackground;

    
    int active_count = data ? data->entity_count : 0;
    
    
    if (active_count > 0 && data) {
        std::sort(data->entities, data->entities + active_count, [](const SoundEntity_t& a, const SoundEntity_t& b) {
            return a.distance < b.distance;
        });
    }

    if (active_count > max_entities) active_count = max_entities;
    for (int i = 0; i < MAX_BLIPS; i++) {
        if (i < active_count) {
            float target_az = data->entities[i].azimuth_angle;
            float target_dist = data->entities[i].distance;

            float diff = target_az - blip_azimuth[i];
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
            blip_azimuth[i] += diff * dt * 8.0f;
            if (blip_azimuth[i] < 0) blip_azimuth[i] += 360.0f;
            if (blip_azimuth[i] >= 360.0f) blip_azimuth[i] -= 360.0f;

            blip_distance[i] += (target_dist - blip_distance[i]) * dt * 8.0f;
            blip_alpha[i] = 1.0f;
            blip_type[i] = data->entities[i].sound_type;
        } else {
            
            
            float fade_speed = (i >= max_entities) ? 10.0f : 3.0f; 
            blip_alpha[i] -= dt * fade_speed;
            if (blip_alpha[i] < 0) blip_alpha[i] = 0;
        }
    }

    int alpha = (int)(255 * global_opacity);

    if (is_fullscreen) {
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("OD_Fullscreen", nullptr, flags);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        float radius = io.DisplaySize.y * 0.45f;

        
        int rh_alpha = (int)(alpha * radar_opacity);

        
        float ch = 8.0f;
        dl->AddLine(ImVec2(center.x - ch, center.y), ImVec2(center.x + ch, center.y), 
                    IM_COL32(255, 255, 255, (int)(rh_alpha * 0.15f)), 1.0f);
        dl->AddLine(ImVec2(center.x, center.y - ch), ImVec2(center.x, center.y + ch), 
                    IM_COL32(255, 255, 255, (int)(rh_alpha * 0.15f)), 1.0f);

        
        for (int i = 0; i < max_entities; i++) {
            if (blip_alpha[i] < 0.01f) continue;

            float angle_rad = (blip_azimuth[i] - 90.0f) * (3.14159f / 180.0f);
            float d = blip_distance[i] * radius * range;
            if (d > radius) d = radius;
            ImVec2 pos = ImVec2(center.x + cosf(angle_rad) * d, center.y + sinf(angle_rad) * d);

            int ba = (int)(blip_alpha[i] * alpha * dot_opacity);
            float t = blip_distance[i];
            int r_col = (int)(255 * (1.0f - t));
            int g_col = (int)(255 * t);

            
            dl->AddCircleFilled(pos, 22.0f, IM_COL32(r_col, g_col, 20, (int)(ba * 0.15f)));
            DrawSoundIcon(dl, pos, 12.0f, blip_type[i], IM_COL32(r_col, g_col, 20, ba), ba);
            dl->AddCircle(pos, 22.0f, IM_COL32(255, 255, 255, (int)(ba * 0.1f)), 24, 1.0f);
        }

        ImGui::End();
    } else {
        
        float half = radar_size * 0.5f;
        float margin = 20.0f;

        
        float px, py;
        switch (position) {
            case 0: px = margin;                                py = margin;                                 break; 
            case 1: px = io.DisplaySize.x * 0.5f - half;       py = margin;                                 break; 
            case 2: px = io.DisplaySize.x - radar_size - margin; py = margin;                               break; 
            case 3: px = margin;                                py = io.DisplaySize.y - radar_size - margin; break; 
            case 4: px = io.DisplaySize.x * 0.5f - half;       py = io.DisplaySize.y - radar_size - margin; break; 
            case 5: px = io.DisplaySize.x - radar_size - margin; py = io.DisplaySize.y - radar_size - margin; break; 
            default: px = io.DisplaySize.x * 0.5f - half;      py = margin;                                 break;
        }

        ImGui::SetNextWindowPos(ImVec2(px, py));
        ImGui::SetNextWindowSize(ImVec2(radar_size, radar_size));

        ImGui::Begin("OD_Radar", nullptr, flags);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 center = ImVec2(wp.x + half, wp.y + half);
        float radius = half - 15.0f;

        
        dl->AddCircleFilled(center, radius + 4, IM_COL32(10, 15, 25, (int)(alpha * radar_opacity * 0.6f)), 64);

        
        dl->AddCircle(center, radius, IM_COL32(0, 220, 180, (int)(alpha * radar_opacity)), 64, 2.0f);
        dl->AddCircle(center, radius * 0.66f, IM_COL32(0, 220, 180, (int)(alpha * radar_opacity * 0.2f)), 48, 1.0f);
        dl->AddCircle(center, radius * 0.33f, IM_COL32(0, 220, 180, (int)(alpha * radar_opacity * 0.2f)), 32, 1.0f);

        
        dl->AddLine(ImVec2(center.x - radius, center.y), ImVec2(center.x + radius, center.y), 
                    IM_COL32(0, 220, 180, (int)(alpha * radar_opacity * 0.15f)), 1.0f);
        dl->AddLine(ImVec2(center.x, center.y - radius), ImVec2(center.x, center.y + radius), 
                    IM_COL32(0, 220, 180, (int)(alpha * radar_opacity * 0.15f)), 1.0f);

        
        dl->AddCircleFilled(center, 3.0f, IM_COL32(0, 220, 180, (int)(alpha * radar_opacity)));

        
        sweep_angle += dt * 90.0f;
        if (sweep_angle >= 360.0f) sweep_angle -= 360.0f;
        float sr = (sweep_angle - 90.0f) * (3.14159f / 180.0f);
        ImVec2 sweep_end = ImVec2(center.x + cosf(sr) * radius, center.y + sinf(sr) * radius);
        dl->AddLine(center, sweep_end, IM_COL32(0, 220, 180, (int)(alpha * radar_opacity * 0.3f)), 1.0f);

        
        dl->AddText(ImVec2(center.x - 3, center.y - radius - 14), IM_COL32(255, 255, 255, (int)(alpha * radar_opacity * 0.5f)), "F");
        dl->AddText(ImVec2(center.x + radius + 4, center.y - 5), IM_COL32(255, 255, 255, (int)(alpha * radar_opacity * 0.5f)), "R");
        dl->AddText(ImVec2(center.x - radius - 12, center.y - 5), IM_COL32(255, 255, 255, (int)(alpha * radar_opacity * 0.5f)), "L");

        
        for (int i = 0; i < max_entities; i++) {
            if (blip_alpha[i] < 0.01f) continue;

            float angle_rad = (blip_azimuth[i] - 90.0f) * (3.14159f / 180.0f);
            float d = blip_distance[i] * radius * range;
            if (d > radius) d = radius;
            ImVec2 pos = ImVec2(center.x + cosf(angle_rad) * d, center.y + sinf(angle_rad) * d);

            int ba = (int)(blip_alpha[i] * alpha * dot_opacity);
            float t = blip_distance[i];
            
            
            int r_col = (int)(255 * (1.0f - t));
            int g_col = (int)(255 * t);
            ImU32 col_glow = IM_COL32(r_col, g_col, 20, (int)(ba * 0.15f));
            ImU32 col_core = IM_COL32(r_col, g_col, 20, ba);

            dl->AddCircleFilled(pos, 12.0f, col_glow);
            DrawSoundIcon(dl, pos, 6.0f, blip_type[i], col_core, ba);
        }

        ImGui::End();
    }
}

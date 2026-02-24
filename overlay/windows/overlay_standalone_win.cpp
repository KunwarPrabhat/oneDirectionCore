#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../osd_radar.h"
#include "../../core/driver/capture.h"
#include "../../core/dsp/dsp.h"
#include "../../core/dsp/classifier.h"
#include "../../hardware/serial_controller.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

static GLFWwindow* overlay_window = nullptr;

int main(int argc, char** argv) {
    
    bool fullscreen_mode = false;
    int osd_position = 0;
    float osd_opacity = 0.8f;
    float dot_opacity = 1.0f;
    float sensitivity = 0.7f;
    float separation = 30.0f;
    float range_scale = 1.0f;
    int poll_rate = 60;
    int max_entities = 4;
    int channels = 2;
    std::string preset = "none";
    std::string hw_port = "";
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--fullscreen") fullscreen_mode = true;
        if (arg.rfind("--pos=", 0) == 0) osd_position = std::atoi(argv[i] + 5);
        if (arg.rfind("--opacity=", 0) == 0) osd_opacity = std::atoi(argv[i] + 10) / 100.0f;
        if (arg.rfind("--dot-opacity=", 0) == 0) dot_opacity = std::atoi(argv[i] + 14) / 100.0f;
        if (arg.rfind("--max-entities=", 0) == 0) max_entities = std::atoi(argv[i] + 15);
        if (arg.rfind("--sensitivity=", 0) == 0) sensitivity = std::atoi(argv[i] + 14) / 100.0f;
        if (arg.rfind("--separation=", 0) == 0) {
            float val = std::atof(argv[i] + 13);
            separation = 60.0f - (val * 0.55f); 
        }
        if (arg.rfind("--range=", 0) == 0) range_scale = std::atof(argv[i] + 8) / 50.0f; 
        if (arg.rfind("--pollrate=", 0) == 0) poll_rate = std::atoi(argv[i] + 11);
        if (arg.rfind("--channels=", 0) == 0) channels = std::atoi(argv[i] + 11);
        if (arg.rfind("--hw-port=", 0) == 0) hw_port = arg.substr(10);
        if (arg.rfind("--preset=", 0) == 0) preset = arg.substr(9);
    }

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

    overlay_window = glfwCreateWindow(1920, 1080, "OD_Overlay", NULL, NULL);
    if (!overlay_window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(overlay_window);
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(overlay_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    OD_Capture_Init(channels);
    OD_Capture_Start();

    OD_Classifier_Init();
    OD_Classifier_SetPreset(preset.c_str());

    bool hw_enabled = false;
    if (!hw_port.empty()) {
        if (OD_Hardware_Init(hw_port.c_str(), 115200)) {
            hw_enabled = true;
        }
    }

    auto frame_duration = std::chrono::duration<double>(1.0 / (poll_rate > 0 ? poll_rate : 60));

    while (!glfwWindowShouldClose(overlay_window)) {
        auto start_time = std::chrono::steady_clock::now();

        glfwPollEvents();

        AudioBuffer_t* buffer = OD_Capture_GetLatestBuffer();
        SpatialData_t dsp_data = {};
        memset(&dsp_data, 0, sizeof(SpatialData_t));
        if (buffer && buffer->buffer) {
            dsp_data = OD_DSP_ProcessBuffer(buffer, sensitivity, separation);
            if (hw_enabled && dsp_data.entity_count > 0) {
                OD_Hardware_SendDirectionLog(dsp_data.entities[0].azimuth_angle);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawRadarHUD(&dsp_data, fullscreen_mode, osd_opacity, dot_opacity, max_entities, range_scale, osd_position);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(overlay_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(overlay_window);

        
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = end_time - start_time;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    OD_Capture_Stop();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (hw_enabled) OD_Hardware_Close();
    glfwDestroyWindow(overlay_window);
    glfwTerminate();

    return 0;
}

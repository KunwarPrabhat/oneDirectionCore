#include "overlay_linux.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../osd_radar.h"
#include "../../core/driver/capture.h"
#include "../../core/dsp/dsp.h"

#include <thread>
#include <atomic>

static GLFWwindow* overlay_window = nullptr;

bool OD_Overlay_Linux_Init() {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); 

    overlay_window = glfwCreateWindow(1920, 1080, "OD_Overlay", NULL, NULL);
    if (!overlay_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(overlay_window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(overlay_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    return true;
}

void OD_Overlay_Linux_Tick() {
    if (!overlay_window || !glfwGetWindowAttrib(overlay_window, GLFW_VISIBLE)) return;

    glfwPollEvents();
    if (glfwWindowShouldClose(overlay_window)) {
        glfwHideWindow(overlay_window);
        return;
    }

    
    AudioBuffer_t* buffer = OD_Capture_GetLatestBuffer();
    SpatialData_t dsp_data = {0};
    if (buffer && buffer->buffer) {
        dsp_data = OD_DSP_ProcessBuffer(buffer);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    
    DrawRadarHUD(&dsp_data, false, 0.8f); 

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(overlay_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(overlay_window);
}

void OD_Overlay_Linux_Start() {
    if (overlay_window) {
        glfwShowWindow(overlay_window);
    }
}

void OD_Overlay_Linux_Stop() {
    if (overlay_window) {
        glfwHideWindow(overlay_window);
    }
}

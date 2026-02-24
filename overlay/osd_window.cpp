#include <iostream>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

void SetWindowMSIAfterburnerStyle(GLFWwindow* window) {
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    
    
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    
    
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    
    
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
    
    
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); 
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#endif
}

void RunOSDLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        
        
        
        
        glfwSwapBuffers(window);
    }
}

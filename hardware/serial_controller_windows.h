#ifndef OD_SERIAL_CONTROLLER_WINDOWS_H
#define OD_SERIAL_CONTROLLER_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif


__declspec(dllexport) int OD_Hardware_Init(const char* com_port, int baud_rate);
__declspec(dllexport) int OD_Hardware_SendDirectionLog(float azimuth);
__declspec(dllexport) void OD_Hardware_Close(void);

#ifdef __cplusplus
}
#endif

#endif 

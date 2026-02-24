#ifndef OD_SERIAL_CONTROLLER_H
#define OD_SERIAL_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif


int OD_Hardware_Init(const char* com_port, int baud_rate);


int OD_Hardware_SendDirectionLog(float azimuth);


void OD_Hardware_Close(void);

#ifdef __cplusplus
}
#endif

#endif 

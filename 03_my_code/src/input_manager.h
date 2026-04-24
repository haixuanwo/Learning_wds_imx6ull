/*
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-30 16:24:09
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-31 13:08:24
 * @Description: file content
 */
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <stdint.h>

typedef enum {
    INPUT_TYPE_NET,
    INPUT_TYPE_KEYBOARD,
}InputType;

typedef struct {
    InputType type;
    uint8_t *data;
    uint32_t dataLen;
}InputEvent, *PInputEvent;

typedef struct InputDevice {
	char *name;
	int (*GetInputEvent)(PInputEvent ptInputEvent);
	int (*DeviceInit)(void);
	int (*DeviceExit)(void);
	struct InputDevice *ptNext;
}InputDevice, *PInputDevice;


void RegisterInputDevice(PInputDevice ptInputDev);
void InputSystemRegister(void);
void IntpuDeviceInit(void);
int GetInputEvent(PInputEvent ptInputEvent);

#endif

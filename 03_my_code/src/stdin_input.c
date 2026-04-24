

#include "stdin_input.h"
#include "input_manager.h"

// #include <stddef.h>
#include <stdio.h>

static int stdinInputGetInputEvent(PInputEvent ptInputEvent)
{
    char str[1024] = {0};
    ptInputEvent->data = NULL;
    ptInputEvent->dataLen = 0;
    size_t len = 0;
    ptInputEvent->dataLen = getline((char **)&ptInputEvent->data, &len, stdin);
    // ptInputEvent->dataLen = getline(&str, sizeof(str), stdin);
    if (0 == ptInputEvent->dataLen)
    {
        return -1;
    }

    ptInputEvent->type = INPUT_TYPE_KEYBOARD;
    printf("T --- %s %d dataLen[%u]\n", __func__, __LINE__, ptInputEvent->dataLen);

	return 0;
}

static int stdinInputDeviceInit(void)
{
    printf("T --- %s %d\n", __func__, __LINE__);
	return 0;
}

static int stdinInputDeviceExit(void)
{
    printf("T --- %s %d\n", __func__, __LINE__);
    return 0;
}

static InputDevice g_tstdinInputDev ={
	.name = "Stdin",
	.GetInputEvent  = stdinInputGetInputEvent,
	.DeviceInit     = stdinInputDeviceInit,
	.DeviceExit     = stdinInputDeviceExit,
};

/**
 * @brief 注册键盘输入设备到输入管理器
 */
void StdinInputRegister(void)
{
	RegisterInputDevice(&g_tstdinInputDev);
}

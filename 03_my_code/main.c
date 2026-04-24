/*
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-30 17:22:22
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-31 13:47:19
 * @Description: file content
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "input_manager.h"
#include "net_input.h"
#include "stdin_input.h"

int main(int argc, char *argv[])
{
    int ret;
	InputEvent event;

	NetInputRegister();
    StdinInputRegister();

	IntpuDeviceInit();

	while (1)
	{
		ret = GetInputEvent(&event);
		if (ret) {
			printf("GetInputEvent err!\n");
			return -1;
		}
		else
		{
			printf("%s %s %d, event.type = %d\n", __FILE__, __FUNCTION__, __LINE__, event.type );
			if (event.type == INPUT_TYPE_NET)
			{
				printf("str       : %s\n", event.data);
			}
            else if (event.type == INPUT_TYPE_KEYBOARD)
			{
				printf("str       : %s\n", event.data);
			}

            free(event.data);
            event.data = NULL;
		}
	}

    return 0;
}

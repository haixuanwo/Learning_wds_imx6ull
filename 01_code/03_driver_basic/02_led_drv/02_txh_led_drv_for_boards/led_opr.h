/*
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-18 11:36:42
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-18 11:37:01
 * @Description: file content
 */
#ifndef _LED_OPR_H
#define _LED_OPR_H

struct led_operations {
	int num;
	int (*init) (int which); /* 初始化LED, which-哪个LED */
	int (*ctl) (int which, char status); /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
};

struct led_operations *get_board_led_opr(void);


#endif


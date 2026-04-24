<!--
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-20 10:09:30
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-20 11:06:56
 * @Description: file content
-->


# adb
Ubuntu中
安装adb
sudo apt install adb -y


查看adb设备
adb devices

登录设备的shell
adb shell

推送文件到adb设备的/sdcard目录
adb push xxx.txt /sdcard/xxx.txt


从adb设备拉取文件
adb pull /root/i2cdetect .

对目录操作也是一样的

# 驱动

insmod led.ko
lsmod
rmmod led.ko

# pinctrl

echo 110 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio110/direction
cat /sys/class/gpio/gpio110/value
echo 110 > /sys/class/gpio/unexport


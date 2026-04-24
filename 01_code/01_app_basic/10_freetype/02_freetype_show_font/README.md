<!--
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2024-09-07 15:27:12
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-12 19:54:47
 * @Description: file content
-->

# 交叉编译freetype库
cd freetype-2.10.2
mkdir install
./configure --prefix=${PWD}/install --host=arm-buildroot-linux-gnueabihf
make -j16
make install

# 编译使用FreeType库的应用程序


cd /home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/include
mv freetype2/* ./

arm-buildroot-linux-gnueabihf-gcc -o freetype_show_font freetype_show_font.c -lfreetype


arm-buildroot-linux-gnueabihf-gcc -o freetype_show_font freetype_show_font.c -lfreetype -I ../freetype-2.10.2/install/include -Lfreetype-2.10.2/install/lib_a

# 在开发板上运行
./freetype_show_font simsun.ttc 24

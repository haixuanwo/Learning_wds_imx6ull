
all:
# 	arm-buildroot-linux-gnueabihf-gcc -o show_line show_line.c -lfreetype
	arm-buildroot-linux-gnueabihf-gcc -o show_line show_line.c -lfreetype -lm -I ../freetype-2.10.2/install/include -Lfreetype-2.10.2/install/lib_a
	arm-buildroot-linux-gnueabihf-gcc -o show_line_angle show_line_angle.c -lfreetype -lm -I ../freetype-2.10.2/install/include -Lfreetype-2.10.2/install/lib_a

clean:
	rm -f show_line
	rm -f show_line_angle


# ./show_line ./simsun.ttc 10 200 80

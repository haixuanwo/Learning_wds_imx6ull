
gpio_keys_100ask {
compatible = "100ask,gpio_key";
gpios = <&gpio5 1 GPIO_ACTIVE_LOW
&gpio4 11 GPIO_ACTIVE_LOW>;
pinctrl-names = "default";
pinctrl-0 = <&key1_100ask &key1_100ask>;
};

snvs
key1_100ask: key1_100ask {        /*!< Function assigned for the core: Cortex-A7[ca7] */
fsl,pins = <
MX6ULL_PAD_SNVS_TAMPER1__GPIO5_IO01        0x000110A0
>;
};

iomuxc
key2_100ask: key2_100ask {                /*!< Function assigned for the core: Cortex-A7[ca7] */
fsl,pins = <
MX6UL_PAD_NAND_CE1_B__GPIO4_IO14           0x000010B0
>;
};

--------------


打开内核日志
echo "7 4 1 7" > /proc/sys/kernel/printk

<!--
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-19 18:03:19
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-19 18:48:55
 * @Description: file content
-->

# qemu

## 1 下载SDK
- book\@100ask:\~\$ git clone https://e.coding.net/codebug8/repo.git
- book\@100ask:\~\$ mkdir -p 100ask_imx6ull-qemu && cd 100ask_imx6ull-qemu
- book\@100ask:\~/100ask_imx6ull-qemu\$ ../repo/repo init -u https://e.coding.net/weidongshan/manifests.git -b linux-sdk -m  - imx6ull100ask-imx6ull_qemu_release_v1.0.xml --no-repo-verify
- book\@100ask:\~/100ask_imx6ull-qemu\$ ../repo/repo sync -j4

编译内核
sudo apt --fix-broken install
sudo apt-get install lzop -y
make mrproper
make 100ask_imx6ull_qemu_defconfig
make zImage -jN //编译zImage内核镜像，其中N参数可以根据CPU个数，来加速编译系统。
make dtbs //编译设备树文件

arch/arm/boot/zImage // 内核
arch/arm/boot/dts/100ask_imx6ull_qemu.dtb // 设备树

## 2 下载模拟器
- git clone https://e.coding.net/weidongshan/ubuntu-18.04_imx6ul_qemu_system.git
- cd ubuntu-18.04_imx6ul_qemu_system
- ./qemu-imx6ull-gui.sh

退出模拟器界面，鼠标才可点击外面
ctrl l g

在qemu中
sudo mount -t nfs -o nolock,vers=3 127.0.0.1:/home/book /mnt
mount -t nfs -o nolock,vers=3 10.0.2.2:/home/book /mnt


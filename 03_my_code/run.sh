

BUILD=build
if [ ! -d "${BUILD}" ]; then
    echo "${BUILD} 目录不存在，正在创建..."
    mkdir -p "${BUILD}"  # -p 参数确保创建父目录

    if [ $? -eq 0 ]; then
        echo "${BUILD}目录创建成功"
    else
        echo "${BUILD}目录创建失败"
        exit 1
    fi
fi

cd build
cmake ..
make -j4

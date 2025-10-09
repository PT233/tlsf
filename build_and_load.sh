#!/bin/sh
# FreeBSD TLSF 编译和测试脚本

echo "=== TLSF Allocator ==="
echo ""

# 清理旧的编译文件
echo "[1/6] clean rabbish..."
make clean
rm -f *.ko *.o

# 编译模块
echo ""
echo "[2/6] compile kernel..."
make
if [ $? -ne 0 ]; then
    echo "compile fail!"
    exit 1
fi
echo "compile success!"

# 检查模块是否已加载
echo ""
echo "[3/6] check module..."
if kldstat | grep -q tlsf_allocator; then
    echo "module loaded,unloading..."
    sudo kldunload tlsf_allocator
    sleep 1
fi

# 加载模块
echo ""
echo "[4/6] loading kernel..."
sudo kldload ./tlsf_allocator.ko
if [ $? -ne 0 ]; then
    echo "loading failed!"
    dmesg | tail -20
    exit 1
fi
echo "loading success!"

# 查看内核日志
echo ""
echo "[5/6] kernel log"
dmesg | tail -5

# 检查设备文件
echo ""
echo "[6/6] check dev..."
if [ -e /dev/my_alloc ]; then
    ls -l /dev/my_alloc
    echo "dev success!"
else
    echo "sev fail!"
    exit 1
fi

echo ""
echo "=== compile and load success! ==="
echo ""
echo "-----------------------------------------"
echo "test:"
echo "----------------------------------------"
echo "  cc -o test_tlsf test_tlsf.c"
echo "  ./test_tlsf"
echo ""
echo "unload:"
echo "  su kldunload tlsf_allocator"

#!/bin/sh
# FreeBSD TLSF 编译和测试脚本

echo "=== TLSF Allocator 编译和测试脚本 ==="
echo ""

# 清理旧的编译文件
echo "[1/6] 清理旧文件..."
make clean
rm -f *.ko *.o

# 编译模块
echo ""
echo "[2/6] 编译内核模块..."
make
if [ $? -ne 0 ]; then
    echo "❌ 编译失败！"
    exit 1
fi
echo "✅ 编译成功！"

# 检查模块是否已加载
echo ""
echo "[3/6] 检查模块状态..."
if kldstat | grep -q tlsf_allocator; then
    echo "⚠️  模块已加载，正在卸载..."
    sudo kldunload tlsf_allocator
    sleep 1
fi

# 加载模块
echo ""
echo "[4/6] 加载内核模块..."
sudo kldload ./tlsf_allocator.ko
if [ $? -ne 0 ]; then
    echo "❌ 模块加载失败！"
    dmesg | tail -20
    exit 1
fi
echo "✅ 模块加载成功！"

# 查看内核日志
echo ""
echo "[5/6] 内核日志："
dmesg | tail -5

# 检查设备文件
echo ""
echo "[6/6] 检查设备文件..."
if [ -e /dev/my_alloc ]; then
    ls -l /dev/my_alloc
    echo "✅ 设备文件创建成功！"
else
    echo "❌ 设备文件不存在！"
    exit 1
fi

echo ""
echo "=== 编译和加载完成！ ==="
echo ""
echo "可以运行以下命令进行测试："
echo "  cc -o test_tlsf test_tlsf.c"
echo "  ./test_tlsf"
echo ""
echo "卸载模块："
echo "  sudo kldunload tlsf_allocator"

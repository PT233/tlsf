#!/bin/sh
# 完整测试脚本 - 从编译到测试一条龙

echo "==================================================================="
echo "    TLSF 完整测试流程 - FreeBSD 14.3"
echo "==================================================================="
echo ""

# 当前目录
WORK_DIR=$(pwd)
echo "工作目录: $WORK_DIR"
echo ""

# ===== 步骤1: 清理 =====
echo "[1/5] 清理旧文件和模块..."
make clean > /dev/null 2>&1
rm -f *.ko *.o test_tlsf 2>/dev/null

# 卸载旧模块（如果存在）
if kldstat | grep -q tlsf_allocator; then
    echo "      发现旧模块，正在卸载..."
    kldunload tlsf_allocator 2>/dev/null
    sleep 1
fi
echo "✅ 清理完成"
echo ""

# ===== 步骤2: 编译内核模块 =====
echo "[2/5] 编译内核模块..."
make
if [ $? -ne 0 ]; then
    echo "❌ 编译失败！请检查代码"
    exit 1
fi

# 检查编译产物
if [ ! -f "tlsf_allocator.ko" ]; then
    echo "❌ 找不到 tlsf_allocator.ko"
    exit 1
fi
echo "✅ 模块编译成功: $(ls -lh tlsf_allocator.ko | awk '{print $5}')"
echo ""

# ===== 步骤3: 加载内核模块 =====
echo "[3/5] 加载内核模块..."
kldload ./tlsf_allocator.ko
if [ $? -ne 0 ]; then
    echo "❌ 模块加载失败！"
    echo ""
    echo "内核日志:"
    dmesg | tail -20
    exit 1
fi

# 等待设备创建
sleep 1

# 验证模块加载
if ! kldstat | grep -q tlsf_allocator; then
    echo "❌ 模块未在 kldstat 中找到"
    exit 1
fi

echo "✅ 模块加载成功"
echo ""
echo "   内核日志:"
dmesg | tail -3 | sed 's/^/   /'
echo ""

# 检查设备文件
if [ ! -e /dev/my_alloc ]; then
    echo "❌ 设备文件 /dev/my_alloc 未创建"
    echo ""
    echo "调试信息:"
    echo "   模块状态:"
    kldstat | grep tlsf | sed 's/^/   /'
    echo ""
    echo "   /dev 目录:"
    ls -l /dev | grep alloc | sed 's/^/   /'
    exit 1
fi

echo "   设备文件:"
ls -l /dev/my_alloc | sed 's/^/   /'
echo ""

# ===== 步骤4: 编译测试程序 =====
echo "[4/5] 编译测试程序..."
cc -o test_tlsf test_tlsf.c
if [ $? -ne 0 ]; then
    echo "❌ 测试程序编译失败！"
    exit 1
fi

if [ ! -x test_tlsf ]; then
    chmod +x test_tlsf
fi

echo "✅ 测试程序编译成功: $(ls -lh test_tlsf | awk '{print $5}')"
echo ""

# ===== 步骤5: 运行测试 =====
echo "[5/5] 运行测试程序..."
echo "==================================================================="
./test_tlsf
TEST_RESULT=$?
echo "==================================================================="
echo ""

if [ $TEST_RESULT -eq 0 ]; then
    echo "🎉 所有测试通过！"
else
    echo "⚠️  测试返回错误代码: $TEST_RESULT"
fi

echo ""
echo "==================================================================="
echo "    测试完成"
echo "==================================================================="
echo ""
echo "有用的命令:"
echo "  查看模块: kldstat | grep tlsf"
echo "  查看设备: ls -l /dev/my_alloc"
echo "  查看日志: dmesg | tail"
echo "  卸载模块: kldunload tlsf_allocator"
echo ""

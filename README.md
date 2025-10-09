# TLSF Memory Allocator - FreeBSD Kernel Module

FreeBSD 14.3 内核可加载模块，实现 TLSF (Two-Level Segregated Fit) 内存分配器。

## 📋 系统要求

- FreeBSD 14.3 Release (amd64)
- 内核源码（用于编译内核模块）
- root 权限（用于加载/卸载模块）

## 🚀 快速开始

### 方法 1: 使用自动脚本

```bash
# 赋予执行权限
chmod +x build_and_load.sh

# 运行脚本
./build_and_load.sh
```

### 方法 2: 手动编译

```bash
# 1. 编译内核模块
make clean
make

# 2. 加载模块
kldload ./tlsf_allocator.ko

# 3. 查看加载信息
dmesg | tail

# 4. 检查设备文件
ls -l /dev/my_alloc
```

## 🧪 测试

### 编译测试程序

```bash
cc -o test_tlsf test_tlsf.c
```

### 运行测试

```bash
./test_tlsf
```

预期输出：
```
=== 测试TLSF分配器 ===
✓ TLSF分配成功: size=1024, ptr=0x...
✓ TLSF释放成功: ptr=0x...

=== 测试基准分配器(malloc) ===
✓ Baseline分配成功: size=1024, ptr=0x...
✓ Baseline释放成功: ptr=0x...

测试完成!
```

## 🔧 卸载模块

```bash
kldunload tlsf_allocator
```

## 📁 文件说明

| 文件 | 说明 |
|------|------|
| `tlsf.c` / `tlsf.h` | TLSF 核心算法实现 |
| `tlsf_allocator.c` | FreeBSD 内核模块包装器 |
| `alloc_ioctl.h` | 内核-用户空间 ioctl 接口定义 |
| `test_tlsf.c` | 用户空间测试程序 |
| `Makefile` | FreeBSD 内核模块构建文件 |
| `build_and_load.sh` | 自动编译和加载脚本 |

## 🐛 故障排除

### 编译错误

**问题**: `KASSERT` 未定义
```
error: use of undeclared identifier 'KASSERT'
```

**解决**: 确保已包含 `<sys/kernel.h>` 头文件

**问题**: 找不到内核头文件
```
sys/param.h: No such file or directory
```

**解决**: 安装内核源码
```bash
# FreeBSD 14.3
pkg install src
# 或从仓库获取
fetch ftp://ftp.freebsd.org/pub/FreeBSD/releases/amd64/14.3-RELEASE/src.txz
tar -C / -xzf src.txz
```

### 加载错误

**问题**: 权限被拒绝
```
kldload: can't load ./tlsf_allocator.ko: Permission denied
```

**解决**: 使用 `sudo` 或以 root 身份运行

**问题**: 模块已加载
```
kldload: can't load ./tlsf_allocator.ko: module already loaded
```

**解决**: 先卸载再加载
```bash
kldunload tlsf_allocator
kldload ./tlsf_allocator.ko
```

### 运行时错误

**问题**: 设备文件不存在
```
Failed to open /dev/my_alloc: No such file or directory
```

**解决**: 检查模块是否正确加载
```bash
kldstat | grep tlsf
dmesg | grep TLSF
```

## 📊 性能测试

可以通过修改 `test_tlsf.c` 添加性能基准测试，比较 TLSF 和 `malloc(9)` 的性能差异。

## ⚙️ 配置

### 修改内存池大小

在 `tlsf_allocator.c` 中修改：
```c
#define TLSF_POOL_SIZE (16 * 1024 * 1024)  // 默认 16MB
```

重新编译后生效。

## 📖 参考资料

- [TLSF 算法原始论文](http://www.gii.upv.es/tlsf/main/docs)
- [FreeBSD 内核开发手册](https://docs.freebsd.org/en/books/developers-handbook/)
- [FreeBSD malloc(9) 手册](https://www.freebsd.org/cgi/man.cgi?query=malloc&sektion=9)

## 📝 许可证

见 `tlsf.h` 文件中的许可证声明。

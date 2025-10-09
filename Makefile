# 模块名
KMOD=   tlsf_allocator

# 源文件列表
SRCS=   tlsf_allocator.c tlsf.c

# 包含 bsd.kmod.mk 来使用内核模块构建系统
.include <bsd.kmod.mk>
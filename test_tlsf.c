/*
 * 用户空间测试程序 - 测试TLSF内核模块
 * 编译: cc -o test_tlsf test_tlsf.c
 * 运行: ./test_tlsf
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "alloc_ioctl.h"

int main() {
    int fd;
    struct alloc_request req;
    int ret;

    // 打开设备
    fd = open("/dev/my_alloc", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/my_alloc");
        return 1;
    }

    printf("=== 测试TLSF分配器 ===\n");

    // 测试1: 分配内存
    req.size = 1024;
    ret = ioctl(fd, ALLOC_IOCTL_TLSF, &req);
    if (ret < 0) {
        perror("TLSF allocation failed");
    } else {
        printf("✓ TLSF分配成功: size=%zu, ptr=%p\n", req.size, req.ptr);

        // 测试2: 释放内存
        ret = ioctl(fd, FREE_IOCTL_TLSF, &req);
        if (ret < 0) {
            perror("TLSF free failed");
        } else {
            printf("✓ TLSF释放成功: ptr=%p\n", req.ptr);
        }
    }

    printf("\n=== 测试基准分配器(malloc) ===\n");

    // 测试3: 基准分配
    req.size = 1024;
    ret = ioctl(fd, ALLOC_IOCTL_BASELINE, &req);
    if (ret < 0) {
        perror("Baseline allocation failed");
    } else {
        printf("✓ Baseline分配成功: size=%zu, ptr=%p\n", req.size, req.ptr);

        // 测试4: 基准释放
        ret = ioctl(fd, FREE_IOCTL_BASELINE, &req);
        if (ret < 0) {
            perror("Baseline free failed");
        } else {
            printf("✓ Baseline释放成功: ptr=%p\n", req.ptr);
        }
    }

    close(fd);
    printf("\n测试完成!\n");
    return 0;
}

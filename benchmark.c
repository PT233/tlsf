#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "alloc_ioctl.h"
#define DEVICE_PATH "/dev/my_alloc"
/* 高精度计时函数 */
long long get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

/* 测试场景：大量小块内存分配与释放 */
void test_many_small_allocs(int fd, int iterations, u_long alloc_cmd, u_long free_cmd) {
    printf("--- Running Test: %d iterations of 64-byte allocations ---\n", iterations);
    void **pointers = malloc(sizeof(void*) * iterations);
    if (!pointers) {
        perror("Failed to allocate host memory for pointers");
        return;
    }

    struct alloc_request req;
    req.size = 64;

    long long start_time = get_time_usec();

    // 分配阶段
    for (int i = 0; i < iterations; ++i) {
        if (ioctl(fd, alloc_cmd, &req)!= 0) {
            fprintf(stderr, "Allocation failed at iteration %d\n", i);
            pointers[i] = NULL;
        } else {
            pointers[i] = req.ptr;
        }
    }

    // 释放阶段
    for (int i = 0; i < iterations; ++i) {
        if (pointers[i]) {
            req.ptr = pointers[i];
            ioctl(fd, free_cmd, &req);
        }
    }

    long long end_time = get_time_usec();
    printf("Total time: %lld microseconds\n", end_time - start_time);
    free(pointers);
}

/* 测试场景：混合大小内存分配与释放 */
void test_mixed_size_allocs(int fd, int iterations, u_long alloc_cmd, u_long free_cmd) {
    printf("--- Running Test: %d iterations of mixed-size (32-4096 bytes) allocations ---\n", iterations);
    void **pointers = malloc(sizeof(void*) * iterations);
    size_t *sizes = malloc(sizeof(size_t) * iterations);
    if (!pointers ||!sizes) {
        perror("Failed to allocate host memory");
        return;
    }

    struct alloc_request req;// 生成随机大小
    for (int i = 0; i < iterations; ++i) {
        sizes[i] = 32 + (rand() % (4096 - 32 + 1));
    }

    long long start_time = get_time_usec();

    // 分配
    for (int i = 0; i < iterations; ++i) {
        req.size = sizes[i];
        if (ioctl(fd, alloc_cmd, &req)!= 0) {
            pointers[i] = NULL;
        } else {
            pointers[i] = req.ptr;
        }
    }

    // 释放
    for (int i = 0; i < iterations; ++i) {
        if (pointers[i]) {
            req.ptr = pointers[i];
            ioctl(fd, free_cmd, &req);
        }
    }

    long long end_time = get_time_usec();
    printf("Total time: %lld microseconds\n", end_time - start_time);
    free(pointers);
    free(sizes);
}


int main(int argc, char *argv) {
    if (argc!= 3) {
        fprintf(stderr, "Usage: %s <test_type: small|mixed> <allocator: tlsf|baseline>\n", argv);
        return 1;
    }

    char *test_type = argv;
    char *allocator_type = argv;
    int iterations = 10000;

    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    u_long alloc_cmd, free_cmd;
    if (strcmp(allocator_type, "tlsf") == 0) {
        alloc_cmd = ALLOC_IOCTL_TLSF;
        free_cmd = FREE_IOCTL_TLSF;
        printf("Testing with TLSF allocator...\n");
    } else if (strcmp(allocator_type, "baseline") == 0) {
        alloc_cmd = ALLOC_IOCTL_BASELINE;
        free_cmd = FREE_IOCTL_BASELINE;
        printf("Testing with baseline kernel malloc(9)...\n");
    } else {
        fprintf(stderr, "Invalid allocator type\n");
        close(fd);
        return 1;
    }

    if (strcmp(test_type, "small") == 0) {
        test_many_small_allocs(fd, iterations, alloc_cmd, free_cmd);
    } else if (strcmp(test_type, "mixed") == 0) {
        test_mixed_size_allocs(fd, iterations, alloc_cmd, free_cmd);
    } else {
        fprintf(stderr, "Invalid test type\n");
    }

    close(fd);
    return 0;
}
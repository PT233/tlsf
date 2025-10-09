/*
 * Userspace test program - Test TLSF kernel module
 * Compile: cc -o test_tlsf test_tlsf.c
 * Run: ./test_tlsf
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

    // Open device
    fd = open("/dev/my_alloc", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/my_alloc");
        return 1;
    }

    printf("=== Testing TLSF Allocator ===\n");

    // Test 1: Allocate memory
    req.size = 1024;
    ret = ioctl(fd, ALLOC_IOCTL_TLSF, &req);
    if (ret < 0) {
        perror("TLSF allocation failed");
    } else {
        printf("✓ TLSF: size=%zu, ptr=%p\n", req.size, req.ptr);

        // Test 2: Free memory
        ret = ioctl(fd, FREE_IOCTL_TLSF, &req);
        if (ret < 0) {
            perror("TLSF free failed");
        } else {
            printf("✓ TLSF free successful: ptr=%p\n", req.ptr);
        }
    }

    printf("\n=== Testing Baseline Allocator (malloc) ===\n");

    // Test 3: Baseline allocation
    req.size = 1024;
    ret = ioctl(fd, ALLOC_IOCTL_BASELINE, &req);
    if (ret < 0) {
        perror("Baseline allocation failed");
    } else {
        printf("✓ Baseline allocation successful: size=%zu, ptr=%p\n", req.size, req.ptr);

        // Test 4: Baseline free
        ret = ioctl(fd, FREE_IOCTL_BASELINE, &req);
        if (ret < 0) {
            perror("Baseline free failed");
        } else {
            printf("✓ Baseline free successful: ptr=%p\n", req.ptr);
        }
    }

    close(fd);
    printf("\nTest completed!\n");
    return 0;
}

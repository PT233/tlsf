#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/conf.h>     // 用于 cdevsw 结构体
#include <sys/uio.h>      // 用于 uiomove
#include "alloc_ioctl.h"  // 包含与用户空间共享的ioctl定义
#include "tlsf.h"         // 包含 TLSF 算法的头文件

/* 步骤 3.1: 定义内核 malloc 使用的内存类型 */
MALLOC_DEFINE(M_TLSF_POOL, "tlsf_pool", "Memory pool for TLSF allocator module");

/* 步骤 3.2: 定义全局变量 */
static struct cdev *my_alloc_cdev;                // 字符设备句柄
static void *tlsf_memory_pool = NULL;             // 指向从内核申请的内存池
static tlsf_t tlsf_instance = NULL;               // TLSF 控制结构体实例
#define TLSF_POOL_SIZE (16 * 1024 * 1024)         // 定义内存池大小为 16MB

/* 步骤 3.3: 声明 ioctl 处理函数 */
static d_ioctl_t my_alloc_ioctl;

/* 步骤 3.4: 定义字符设备操作函数表 */
static struct cdevsw my_alloc_cdevsw = {
   .d_version = D_VERSION,
   .d_ioctl = my_alloc_ioctl,
   .d_name = "my_alloc",
};

/* 步骤 3.5: 实现 ioctl 处理函数 */
static int my_alloc_ioctl(struct cdev *dev, u_long cmd, caddr_t data, int fflag, struct thread *td) {
    struct alloc_request *req = (struct alloc_request *)data;
    int error = 0;

    switch (cmd) {
        case ALLOC_IOCTL_TLSF:
            req->ptr = tlsf_malloc(tlsf_instance, req->size);
            if (req->ptr == NULL) {
                error = ENOMEM;
            }
            break;

        case FREE_IOCTL_TLSF:
            tlsf_free(tlsf_instance, req->ptr);
            break;

        // 为了对比，我们同时提供对内核原生 malloc(9) 的调用
        case ALLOC_IOCTL_BASELINE:
            req->ptr = malloc(req->size, M_DEVBUF, M_NOWAIT);
            if (req->ptr == NULL) {
                error = ENOMEM;
            }
            break;

        case FREE_IOCTL_BASELINE:
            free(req->ptr, M_DEVBUF);
            break;

        default:
            error = EINVAL; // 无效的命令
            break;
    }
    return error;
}

/* 步骤 3.6: 实现模块事件处理函数 (生命周期管理) */
static int tlsf_module_event_handler(struct module *m, int event_type, void *arg) {
    int error = 0;
    switch (event_type) {
        case MOD_LOAD:
            // 1. 为 TLSF 申请内存池
            tlsf_memory_pool = malloc(TLSF_POOL_SIZE, M_TLSF_POOL, M_WAITOK | M_ZERO);
            if (tlsf_memory_pool == NULL) {
                printf("TLSF KLD: Failed to allocate memory pool.\n");
                return ENOMEM;
            }

            // 2. 初始化 TLSF 实例
            tlsf_instance = tlsf_create_with_pool(tlsf_memory_pool, TLSF_POOL_SIZE);
            if (tlsf_instance == NULL) {
                printf("TLSF KLD: Failed to create TLSF instance.\n");
                free(tlsf_memory_pool, M_TLSF_POOL);
                return ENXIO;
            }

            // 3. 创建字符设备 /dev/my_alloc
            error = make_dev_p(MAKEDEV_CHECKNAME, &my_alloc_cdev, &my_alloc_cdevsw, 0,
                               UID_ROOT, GID_WHEEL, 0666, "my_alloc");
            if (error != 0) {
                tlsf_destroy(tlsf_instance);
                free(tlsf_memory_pool, M_TLSF_POOL);
                return error;
            }
            printf("TLSF Allocator Module Loaded. Device /dev/my_alloc created.\n");
            break;

        case MOD_UNLOAD:
            // 卸载顺序与加载时相反
            // 1. 销毁字符设备
            destroy_dev(my_alloc_cdev);

            // 2. 销毁 TLSF 实例并释放内存池
            if (tlsf_instance) {
                tlsf_destroy(tlsf_instance);
            }
            if (tlsf_memory_pool) {
                free(tlsf_memory_pool, M_TLSF_POOL);
            }
            printf("TLSF Allocator Module Unloaded.\n");
            break;

        default:
            error = EOPNOTSUPP;
            break;
    }
    return error;
}


/* 步骤 3.7: 定义模块元数据 */
static moduledata_t tlsf_moduledata = {
    "tlsf_allocator",           // 模块名
    tlsf_module_event_handler,  // 事件处理函数
    NULL                        // 额外数据
};

/* 步骤 3.8: 声明模块 */
DECLARE_MODULE(tlsf_allocator, tlsf_moduledata, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
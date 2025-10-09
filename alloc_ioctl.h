#ifndef _ALLOC_IOCTL_H_
#define _ALLOC_IOCTL_H_
#include <sys/ioccom.h>
// 定义用于交互的数据结构
struct alloc_request 
{
    size_t size; // 输入: 请求分配的大小
    void* ptr;   // 输入: 待释放的指针; 输出: 分配成功后的指针
};

// 'a' 是一个任意选择的魔数，用于区分我们的 ioctl 和其他驱动的 ioctl
// 1,2,3,4 是唯一的命令编号
// _IOWR 表示这个 ioctl 会向内核写入数据，并从内核读回数据
#define ALLOC_IOCTL_TLSF     _IOWR('a', 1, struct alloc_request)
#define FREE_IOCTL_TLSF      _IOW('a', 2, struct alloc_request)
#define ALLOC_IOCTL_BASELINE _IOWR('a', 3, struct alloc_request)
#define FREE_IOCTL_BASELINE  _IOW('a', 4, struct alloc_request)
#endif /* _ALLOC_IOCTL_H_ */
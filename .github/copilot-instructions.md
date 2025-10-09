# TLSF Allocator - FreeBSD Kernel Module

## Project Overview

This is a **FreeBSD kernel loadable module (KLD)** that implements the **Two-Level Segregated Fit (TLSF)** memory allocator as a character device driver. The module exposes TLSF allocation operations via `/dev/my_alloc` for performance comparison with FreeBSD's native `malloc(9)`.

**Key Architecture:**
- `tlsf.c` / `tlsf.h`: Core TLSF algorithm (portable, standalone implementation)
- `tlsf_allocator.c`: FreeBSD kernel module wrapper that creates the character device
- `alloc_ioctl.h`: Shared ioctl definitions between kernel and userspace
- `Makefile`: Uses FreeBSD's `bsd.kmod.mk` build system

## Critical Build & Development Workflow

### Building the Kernel Module
```bash
make                    # Compiles to tlsf_allocator.ko
make clean              # Remove build artifacts
```

### Loading/Unloading the Module (requires root)
```bash
kldload ./tlsf_allocator.ko     # Load module, creates /dev/my_alloc
kldunload tlsf_allocator        # Unload module
kldstat | grep tlsf             # Check if module is loaded
dmesg | tail                    # View kernel module messages
```

### Testing from Userspace
User applications call `ioctl()` on `/dev/my_alloc` with commands from `alloc_ioctl.h`:
- `ALLOC_IOCTL_TLSF` / `FREE_IOCTL_TLSF`: TLSF allocator operations
- `ALLOC_IOCTL_BASELINE` / `FREE_IOCTL_BASELINE`: Native `malloc(9)` for comparison

## Architecture Details

### Memory Pool Management
- **Fixed 16MB pool** (`TLSF_POOL_SIZE`) allocated via `malloc(9)` with `M_TLSF_POOL` type
- Pool initialized in `MOD_LOAD` event, destroyed in `MOD_UNLOAD`
- TLSF instance created with `tlsf_create_with_pool()` managing the entire pool

### TLSF Algorithm Core (`tlsf.c`)
- **O(1) malloc/free** via two-level bitmap segregation
- Platform-specific bit manipulation: Uses compiler intrinsics (`__builtin_ffs`, `__builtin_clz`) on GCC/Clang, falls back to generic implementation
- **64-bit support**: Conditional compilation with `TLSF_64BIT` macro
- Block header encodes size + status bits (free/used, prev_free) in the `size` field's low bits
- Linked list structure: `block_header_t` with physical/free list pointers

### Key Data Structures
```c
// Block metadata - stores allocation state
typedef struct block_header_t {
    struct block_header_t* prev_phys_block;  // Physical predecessor (only valid if prev is free)
    size_t size;                              // Size + status bits (bits 0-1)
    struct block_header_t* next_free;         // Free list next (only valid if free)
    struct block_header_t* prev_free;         // Free list prev (only valid if free)
} block_header_t;

// TLSF control structure - manages free lists via bitmaps
typedef struct control_t {
    block_header_t block_null;                // Sentinel for empty lists
    unsigned int fl_bitmap;                   // First-level bitmap
    unsigned int sl_bitmap[FL_INDEX_COUNT];   // Second-level bitmaps
    block_header_t* blocks[FL_INDEX_COUNT][SL_INDEX_COUNT];  // Free list heads
} control_t;
```

## Project-Specific Conventions

### FreeBSD Kernel Headers
- **Order matters**: Include `<sys/types.h>`, `<sys/param.h>`, `<sys/module.h>` before others
- Use `MALLOC_DEFINE()` to declare custom memory types for `malloc(9)`
- Module lifecycle: `MOD_LOAD` → allocate resources; `MOD_UNLOAD` → cleanup in reverse order

### Character Device Pattern
```c
static struct cdevsw my_alloc_cdevsw = {
    .d_version = D_VERSION,
    .d_ioctl = my_alloc_ioctl,
    .d_name = "my_alloc",
};
// Create device with: make_dev_p(MAKEDEV_CHECKNAME, ...)
// Destroy with: destroy_dev()
```

### Code Style
- **Chinese comments** mixed with English code (existing style, preserve in edits)
- Kernel-specific: Use `printf()` not `printk()` (this is FreeBSD, not Linux)
- Error handling: Return POSIX error codes (`ENOMEM`, `EINVAL`, `ENXIO`)

## Integration Points

### Kernel APIs Used
- `malloc(9)` / `free(9)` with `M_WAITOK | M_ZERO` for pool allocation
- `make_dev_p()` / `destroy_dev()` for character device lifecycle
- `ioctl` handler signature: `d_ioctl_t(struct cdev*, u_long, caddr_t, int, struct thread*)`

### Userspace Interface
- Device file: `/dev/my_alloc` (mode 0666, root:wheel)
- Communication: `ioctl()` with `struct alloc_request { size_t size; void* ptr; }`
- Magic number `'a'` used for ioctl commands (`_IOWR`, `_IOW` macros)

## Important Notes

- **Platform**: This is FreeBSD-specific, not Linux. `sys/systm.h`, `sys/module.h`, `bsd.kmod.mk` are FreeBSD kernel APIs
- **Testing context**: This appears to be for comparing TLSF vs. native allocator performance in kernel space
- **No dynamic pool resizing**: Fixed 16MB pool; allocation fails when exhausted (returns `ENOMEM`)
- **Thread safety**: Not explicitly handled - assumes kernel-level synchronization if needed

## Common Tasks

### Adding new ioctl commands
1. Define command in `alloc_ioctl.h` using `_IOWR`/`_IOW` macros
2. Add case in `my_alloc_ioctl()` switch statement in `tlsf_allocator.c`
3. Rebuild module and reload

### Modifying pool size
Change `TLSF_POOL_SIZE` macro in `tlsf_allocator.c` (default: 16MB)

### Debugging
- Check `dmesg` output for module load/unload messages
- Use `kldstat -v` for detailed module info
- TLSF has built-in integrity checks: `tlsf_check()`, `tlsf_check_pool()`

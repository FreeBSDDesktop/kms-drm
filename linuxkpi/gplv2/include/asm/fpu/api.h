#ifndef _ASM_X86_FPU_API_H
#define _ASM_X86_FPU_API_H

#if defined(__i386__)

/*
 * Allow build on i386. Use of these functions by i915
 * is disabled since CONFIG_AS_MOVNTDQA is amd64 only.
 * Other users are amdgpu, assume no one is using amdgpu
 * driver on 32bit hardware...
 */
#define	kernel_fpu_begin()
#define	kernel_fpu_end()

#else

#include <machine/fpu.h>

static inline struct fpu_kern_ctx *
bsd_kernel_fpu_begin() {
	struct fpu_kern_ctx *ctx;
	ctx = fpu_kern_alloc_ctx(0);
	fpu_kern_enter(curthread, ctx,
	    FPU_KERN_NORMAL);
	return ctx;
}

static inline void
bsd_kernel_fpu_end(struct fpu_kern_ctx *ctx) {
	fpu_kern_leave(curthread, ctx);	\
	fpu_kern_free_ctx(ctx);
}

/*
 * The linux versions of these does not require
 * any local variables, but the bsd ones do.
 *
 * The result is that if kernel_fpu_begin is used
 * twice in one function then the variable it
 * creates (__fpu_ctx) will be redefined
 *
 * the bsd_* functions above allow a fallback
 * option in the case that kernel_fpu_begin is
 * used twice. ctx = bsd_kernel_fpu_begin can
 * be used instead.
 */
#define	kernel_fpu_begin()			\
	struct fpu_kern_ctx *__fpu_ctx;		\
	__fpu_ctx = bsd_kernel_fpu_begin();

#define	kernel_fpu_end()			\
	bsd_kernel_fpu_end(__fpu_ctx);

#endif

#endif /* _ASM_X86_FPU_API_H */

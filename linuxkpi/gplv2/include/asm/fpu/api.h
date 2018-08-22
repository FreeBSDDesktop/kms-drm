#ifndef _ASM_X86_FPU_API_H
#define _ASM_X86_FPU_API_H

#include <machine/fpu.h>

#define	kernel_fpu_begin()			\
	struct fpu_kern_ctx *__fpu_ctx;		\
	__fpu_ctx = fpu_kern_alloc_ctx(0);	\
	fpu_kern_enter(curthread, __fpu_ctx,	\
	    FPU_KERN_NORMAL);

#define	kernel_fpu_end()			\
	fpu_kern_leave(curthread, __fpu_ctx);	\
	fpu_kern_free_ctx(__fpu_ctx);

#endif /* _ASM_X86_FPU_API_H */

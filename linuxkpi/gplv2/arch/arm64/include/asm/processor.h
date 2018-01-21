#ifndef _ASM_X86_PROCESSOR_H
#define _ASM_X86_PROCESSOR_H

#define smp_rmb() rmb()
#define smp_wmb() wmb()

static inline void cpu_relax(void)
{
	__asm __volatile("yield" ::: "memory");
}

#endif

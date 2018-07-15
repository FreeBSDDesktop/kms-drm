#ifndef _ASM_GPLV2_PROCESSOR_H_
#define _ASM_GPLV2_PROCESSOR_H_

#define smp_mb() mb()
#define smp_rmb() rmb()
#define smp_wmb() wmb()

static inline void cpu_relax(void)
{
	__asm __volatile("yield" ::: "memory");
}

#endif

#ifndef LINUX_MEMORY_H
#define LINUX_MEMORY_H

#include <linux/mm.h>
#include "asm-generic/getorder.h"

/* note: taken from drm_os_freebsd.h off base. similar definitions are in the current drm_os_freebsd.h */
static inline long
__copy_to_user(void __user *to, const void *from, unsigned long n)
{
        return (copyout(from, to, n) != 0 ? n : 0);
}
#define copy_to_user(to, from, n) __copy_to_user((to), (from), (n))

static inline unsigned long
__copy_from_user(void *to, const void __user *from, unsigned long n)
{
        return ((copyin(__DECONST(void *, from), to, n) != 0 ? n : 0));
}
#define copy_from_user(to, from, n) __copy_from_user((to), (from), (n))

/* this should be in mm.h */
extern struct vm_area_struct * find_vma(struct mm_struct * mm, unsigned long addr);

/*  this should be in asm-generic/page.h */
#define __pa(x) ((unsigned long) (x))

#endif

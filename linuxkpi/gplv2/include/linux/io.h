/*
 * Copyright 2006 PathScale, Inc.  All Rights Reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef	_LINUX_GPLV2_IO_H_
#define	_LINUX_GPLV2_IO_H_

#include_next <linux/io.h>
 
#if defined(__amd64__) || defined(__i386__)
extern int arch_io_reserve_memtype_wc(resource_size_t start, resource_size_t size);
extern void arch_io_free_memtype_wc(resource_size_t start, resource_size_t size);
#endif

/*
 * Some systems (x86 without PAT) have a somewhat reliable way to mark a
 * physical address range such that uncached mappings will actually
 * end up write-combining.  This facility should be used in conjunction
 * with pgprot_writecombine, ioremap-wc, or set_memory_wc, since it has
 * no effect if the per-page mechanisms are functional.
 * (On x86 without PAT, these functions manipulate MTRRs.)
 *
 * arch_phys_del_wc(0) or arch_phys_del_wc(any error code) is guaranteed
 * to have no effect.
 */
#ifndef arch_phys_wc_add
static inline int __must_check arch_phys_wc_add(unsigned long base,
						unsigned long size)
{
	return 0;  /* It worked (i.e. did nothing). */
}

static inline void arch_phys_wc_del(int handle)
{
}

#define arch_phys_wc_add arch_phys_wc_add
#ifndef arch_phys_wc_index
static inline int arch_phys_wc_index(int handle)
{
	return -1;
}
#define arch_phys_wc_index arch_phys_wc_index
#endif
#endif

#endif	/* _LINUX_IO_H_ */

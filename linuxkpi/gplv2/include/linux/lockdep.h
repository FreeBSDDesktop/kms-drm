/*
 * Runtime locking correctness validator
 *
 *  Copyright (C) 2006,2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra
 *
 * see Documentation/locking/lockdep-design.txt for more details.
 */
#ifndef __LINUX_LOCKDEP_H
#define __LINUX_LOCKDEP_H

struct lock_class_key { };

# define lock_release(l, n, i)			do { } while (0)

#define lockdep_assert_held(l)			do { (void)(l); } while (0)
#define lockdep_assert_held_once(l)		do { (void)(l); } while (0)

# define might_lock(lock) do { } while (0)

#endif /* __LINUX_LOCKDEP_H */

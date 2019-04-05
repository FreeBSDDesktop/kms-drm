#ifndef	_LINUX_GPLV2_MODULE_H_
#define	_LINUX_GPLV2_MODULE_H_

#define	EXPORT_SYMBOL(name)
#define	EXPORT_SYMBOL_GPL(name)

#include_next <linux/module.h>

#define	LKPI_DRIVER_MODULE(mod, init, exit)				\
	static int mod##_evh(module_t m, int e, void *a)		\
	{								\
		switch (e) {						\
		case MOD_LOAD:						\
			(init)();					\
			break;						\
		case MOD_UNLOAD:					\
			(exit)();					\
			break;						\
		}							\
		return (0);						\
	}								\
	static moduledata_t mod##_md =					\
		{							\
		 .name = #mod,						\
		 .evhand = mod##_evh,					\
		};							\
	DECLARE_MODULE(mod, mod##_md, SI_SUB_DRIVERS, SI_ORDER_ANY);

#endif

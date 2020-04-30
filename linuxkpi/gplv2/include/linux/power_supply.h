#ifndef _LINUX_POWER_SUPPLY_H_
#define _LINUX_POWER_SUPPLY_H_

#include <sys/types.h>
#include <sys/power.h>

static inline int power_supply_is_system_supplied(void)
{
	return (power_profile_get_state() == POWER_PROFILE_PERFORMANCE);
}

#endif


#include <linux/device.h>
#include <contrib/dev/acpica/include/acpi.h>
#include <dev/acpica/acpivar.h>

extern ACPI_HANDLE acpi_bsd_get_handle(struct device *dev);

ACPI_HANDLE
acpi_bsd_get_handle(struct device *dev)
{
	return (acpi_get_handle(dev->bsddev));
}


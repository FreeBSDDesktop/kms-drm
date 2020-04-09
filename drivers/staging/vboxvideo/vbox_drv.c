// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2013-2017 Oracle Corporation
 * This file is based on ast_drv.c
 * Copyright 2012 Red Hat Inc.
 * Authors: Dave Airlie <airlied@redhat.com>
 *          Michael Thayer <michael.thayer@oracle.com,
 *          Hans de Goede <hdegoede@redhat.com>
 */
#include <linux/module.h>
#include <linux/console.h>
/* #include <linux/vt_kern.h> */

#include <drm/drm_crtc_helper.h>

#include "vbox_drv.h"

static int vbox_modeset = -1;

MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, vbox_modeset, int, 0400);

static struct drm_driver driver;

static const struct pci_device_id pciidlist[] = {
	{ PCI_DEVICE(0x80ee, 0xbeef) },
	{ }
};
MODULE_DEVICE_TABLE(pci, pciidlist);

static struct drm_fb_helper_funcs vbox_fb_helper_funcs = {
	.fb_probe = vboxfb_create,
};

static int vbox_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct vbox_private *vbox;
	int ret = 0;

	if (!vbox_check_supported(VBE_DISPI_ID_HGSMI))
		return -ENODEV;

	vbox = kzalloc(sizeof(*vbox), GFP_KERNEL);
	if (!vbox)
		return -ENOMEM;

	ret = drm_dev_init(&vbox->ddev, &driver, &pdev->dev);
	if (ret) {
		kfree(vbox);
		return ret;
	}

	vbox->ddev.pdev = pdev;
	vbox->ddev.dev_private = vbox;
	pci_set_drvdata(pdev, vbox);
	mutex_init(&vbox->hw_mutex);

	ret = pci_enable_device(pdev);
	if (ret)
		goto err_dev_put;

	ret = vbox_hw_init(vbox);
	if (ret)
		goto err_pci_disable;

	ret = vbox_mm_init(vbox);
	if (ret)
		goto err_hw_fini;

	ret = vbox_mode_init(vbox);
	if (ret)
		goto err_mm_fini;

	ret = vbox_irq_init(vbox);
	if (ret)
		goto err_mode_fini;

	ret = drm_fb_helper_fbdev_setup(&vbox->ddev, &vbox->fb_helper,
					&vbox_fb_helper_funcs, 32,
					vbox->num_crtcs);
	if (ret)
		goto err_irq_fini;

	ret = drm_dev_register(&vbox->ddev, 0);
	if (ret)
		goto err_fbdev_fini;

	return 0;

err_fbdev_fini:
	vbox_fbdev_fini(vbox);
err_irq_fini:
	vbox_irq_fini(vbox);
err_mode_fini:
	vbox_mode_fini(vbox);
err_mm_fini:
	vbox_mm_fini(vbox);
err_hw_fini:
	vbox_hw_fini(vbox);
err_pci_disable:
	pci_disable_device(pdev);
err_dev_put:
	drm_dev_put(&vbox->ddev);
	return ret;
}

static void vbox_pci_remove(struct pci_dev *pdev)
{
	struct vbox_private *vbox = pci_get_drvdata(pdev);

	drm_dev_unregister(&vbox->ddev);
	vbox_fbdev_fini(vbox);
	vbox_irq_fini(vbox);
	vbox_mode_fini(vbox);
	vbox_mm_fini(vbox);
	vbox_hw_fini(vbox);
	drm_dev_put(&vbox->ddev);
}

#ifdef CONFIG_PM_SLEEP
static int vbox_pm_suspend(struct device *dev)
{
	struct vbox_private *vbox = dev_get_drvdata(dev);
	int error;

	error = drm_mode_config_helper_suspend(&vbox->ddev);
	if (error)
		return error;

	pci_save_state(vbox->ddev.pdev);
	pci_disable_device(vbox->ddev.pdev);
	pci_set_power_state(vbox->ddev.pdev, PCI_D3hot);

	return 0;
}

static int vbox_pm_resume(struct device *dev)
{
	struct vbox_private *vbox = dev_get_drvdata(dev);

	if (pci_enable_device(vbox->ddev.pdev))
		return -EIO;

	return drm_mode_config_helper_resume(&vbox->ddev);
}

static int vbox_pm_freeze(struct device *dev)
{
	struct vbox_private *vbox = dev_get_drvdata(dev);

	return drm_mode_config_helper_suspend(&vbox->ddev);
}

static int vbox_pm_thaw(struct device *dev)
{
	struct vbox_private *vbox = dev_get_drvdata(dev);

	return drm_mode_config_helper_resume(&vbox->ddev);
}

static int vbox_pm_poweroff(struct device *dev)
{
	struct vbox_private *vbox = dev_get_drvdata(dev);

	return drm_mode_config_helper_suspend(&vbox->ddev);
}

static const struct dev_pm_ops vbox_pm_ops = {
	.suspend = vbox_pm_suspend,
	.resume = vbox_pm_resume,
	.freeze = vbox_pm_freeze,
	.thaw = vbox_pm_thaw,
	.poweroff = vbox_pm_poweroff,
	.restore = vbox_pm_resume,
};
#endif

static struct pci_driver vbox_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = vbox_pci_probe,
	.remove = vbox_pci_remove,
#ifdef CONFIG_PM_SLEEP
	.driver.pm = &vbox_pm_ops,
#endif
};

static const struct file_operations vbox_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.mmap = vbox_mmap,
	.poll = drm_poll,
	.read = drm_read,
};

static int vbox_master_set(struct drm_device *dev,
			   struct drm_file *file_priv, bool from_open)
{
	struct vbox_private *vbox = dev->dev_private;

	/*
	 * We do not yet know whether the new owner can handle hotplug, so we
	 * do not advertise dynamic modes on the first query and send a
	 * tentative hotplug notification after that to see if they query again.
	 */
	vbox->initial_mode_queried = false;

	return 0;
}

static void vbox_master_drop(struct drm_device *dev, struct drm_file *file_priv)
{
	struct vbox_private *vbox = dev->dev_private;

	/* See vbox_master_set() */
	vbox->initial_mode_queried = false;
}

static struct drm_driver driver = {
	.driver_features =
	    DRIVER_MODESET | DRIVER_GEM | DRIVER_PRIME | DRIVER_ATOMIC,
	.dev_priv_size = 0,

	.lastclose = drm_fb_helper_lastclose,
	.master_set = vbox_master_set,
	.master_drop = vbox_master_drop,

	.fops = &vbox_fops,
	.irq_handler = vbox_irq_handler,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,

	.gem_free_object_unlocked = vbox_gem_free_object,
	.dumb_create = vbox_dumb_create,
	.dumb_map_offset = vbox_dumb_mmap_offset,
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_pin = vbox_gem_prime_pin,
	.gem_prime_unpin = vbox_gem_prime_unpin,
	.gem_prime_get_sg_table = vbox_gem_prime_get_sg_table,
	.gem_prime_import_sg_table = vbox_gem_prime_import_sg_table,
	.gem_prime_vmap = vbox_gem_prime_vmap,
	.gem_prime_vunmap = vbox_gem_prime_vunmap,
	.gem_prime_mmap = vbox_gem_prime_mmap,
};

static int __init vbox_init(void)
{
#ifdef CONFIG_VGA_CONSOLE
	if (vgacon_text_force() && vbox_modeset == -1)
		return -EINVAL;
#endif

	if (vbox_modeset == 0)
		return -EINVAL;

#ifdef __linux__
	return pci_register_driver(&vbox_pci_driver);
#else
	int ret;
	vbox_pci_driver.bsdclass = drm_devclass;
	ret = linux_pci_register_drm_driver(&vbox_pci_driver);
	if (ret)
		DRM_ERROR("Failed initializing DRM.\n");
	return ret;
#endif
}

static void __exit vbox_exit(void)
{
#ifdef __linux__
	pci_unregister_driver(&vbox_pci_driver);
#else
	linux_pci_unregister_drm_driver(&vbox_pci_driver);
#endif
}

#ifdef __linux__
module_init(vbox_init);
module_exit(vbox_exit);

MODULE_AUTHOR("Oracle Corporation");
MODULE_AUTHOR("Hans de Goede <hdegoede@redhat.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");

#else
LKPI_DRIVER_MODULE(vboxvideo, vbox_init, vbox_exit);
LKPI_PNP_INFO(pci, vboxvideo, pciidlist);
MODULE_DEPEND(vboxvideo, drmn, 2, 2, 2);
MODULE_DEPEND(vboxvideo, ttm, 1, 1, 1);
MODULE_DEPEND(vboxvideo, agp, 1, 1, 1);
MODULE_DEPEND(vboxvideo, linuxkpi, 1, 1, 1);
MODULE_DEPEND(vboxvideo, linuxkpi_gplv2, 1, 1, 1);
MODULE_DEPEND(vboxvideo, debugfs, 1, 1, 1);
#endif

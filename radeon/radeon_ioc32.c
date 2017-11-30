/**
 * \file radeon_ioc32.c
 *
 * 32-bit ioctl compatibility routines for the Radeon DRM.
 *
 * \author Paul Mackerras <paulus@samba.org>
 *
 * Copyright (C) Paul Mackerras 2005
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <linux/compat.h>

#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include <drm/drm_ioctl.h>
#include "radeon_drv.h"

#ifdef CONFIG_COMPAT
typedef struct drm_radeon_init32 {
	int func;
	u32 sarea_priv_offset;
	int is_pci;
	int cp_mode;
	int gart_size;
	int ring_size;
	int usec_timeout;

	unsigned int fb_bpp;
	unsigned int front_offset, front_pitch;
	unsigned int back_offset, back_pitch;
	unsigned int depth_bpp;
	unsigned int depth_offset, depth_pitch;

	u32 fb_offset;
	u32 mmio_offset;
	u32 ring_offset;
	u32 ring_rptr_offset;
	u32 buffers_offset;
	u32 gart_textures_offset;
} drm_radeon_init32_t;

static int compat_radeon_cp_init(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	drm_radeon_init32_t init32;
	drm_radeon_init_t __user *init = (void __user *)arg;

	if (copy_from_user(&init32, init, sizeof(init32)))
		return -EFAULT;

	if (put_user(init32.func, &init->func)
	    || put_user(init32.sarea_priv_offset, &init->sarea_priv_offset)
	    || put_user(init32.is_pci, &init->is_pci)
	    || put_user(init32.cp_mode, &init->cp_mode)
	    || put_user(init32.gart_size, &init->gart_size)
	    || put_user(init32.ring_size, &init->ring_size)
	    || put_user(init32.usec_timeout, &init->usec_timeout)
	    || put_user(init32.fb_bpp, &init->fb_bpp)
	    || put_user(init32.front_offset, &init->front_offset)
	    || put_user(init32.front_pitch, &init->front_pitch)
	    || put_user(init32.back_offset, &init->back_offset)
	    || put_user(init32.back_pitch, &init->back_pitch)
	    || put_user(init32.depth_bpp, &init->depth_bpp)
	    || put_user(init32.depth_offset, &init->depth_offset)
	    || put_user(init32.depth_pitch, &init->depth_pitch)
	    || put_user(init32.fb_offset, &init->fb_offset)
	    || put_user(init32.mmio_offset, &init->mmio_offset)
	    || put_user(init32.ring_offset, &init->ring_offset)
	    || put_user(init32.ring_rptr_offset, &init->ring_rptr_offset)
	    || put_user(init32.buffers_offset, &init->buffers_offset)
	    || put_user(init32.gart_textures_offset,
			  &init->gart_textures_offset))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_CP_INIT, (unsigned long)init);
}

typedef struct drm_radeon_clear32 {
	unsigned int flags;
	unsigned int clear_color;
	unsigned int clear_depth;
	unsigned int color_mask;
	unsigned int depth_mask;	/* misnamed field:  should be stencil */
	u32 depth_boxes;
} drm_radeon_clear32_t;

static int compat_radeon_cp_clear(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	drm_radeon_clear32_t clr32;
	drm_radeon_clear_t __user *clr = (void __user *)arg;

	if (copy_from_user(&clr32, clr, sizeof(clr32)))
		return -EFAULT;

	if (put_user(clr32.flags, &clr->flags)
	    || put_user(clr32.clear_color, &clr->clear_color)
	    || put_user(clr32.clear_depth, &clr->clear_depth)
	    || put_user(clr32.color_mask, &clr->color_mask)
	    || put_user(clr32.depth_mask, &clr->depth_mask)
	    || put_user(clr32.depth_boxes,
			  &clr->depth_boxes))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_CLEAR, (unsigned long)clr);
}

typedef struct drm_radeon_stipple32 {
	u32 mask;
} drm_radeon_stipple32_t;

static int compat_radeon_cp_stipple(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	drm_radeon_stipple32_t __user *argp = (void __user *)arg;
	drm_radeon_stipple_t __user *request = (void __user *)arg;
	u32 mask;

	if (get_user(mask, &argp->mask))
		return -EFAULT;

	if (put_user(mask, &request->mask))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_STIPPLE, (unsigned long)request);
}

typedef struct drm_radeon_tex_image32 {
	unsigned int x, y;	/* Blit coordinates */
	unsigned int width, height;
	u32 data;
} drm_radeon_tex_image32_t;

typedef struct drm_radeon_texture32 {
	unsigned int offset;
	int pitch;
	int format;
	int width;		/* Texture image coordinates */
	int height;
	u32 image;
} drm_radeon_texture32_t;

static int compat_radeon_cp_texture(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	drm_radeon_texture32_t req32;
	drm_radeon_texture_t __user *request = (void __user *)arg;
	drm_radeon_tex_image32_t img32;
	drm_radeon_tex_image_t __user *image = (void __user *)(unsigned long)req32.image;

	if (req32.image == 0)
		return -EINVAL;

	image = (drm_radeon_tex_image_t __user *) (request + 1);

	if (put_user(req32.offset, &request->offset)
	    || put_user(req32.pitch, &request->pitch)
	    || put_user(req32.format, &request->format)
	    || put_user(req32.width, &request->width)
	    || put_user(req32.height, &request->height)
	    || put_user(image, &request->image)
	    || put_user(img32.x, &image->x)
	    || put_user(img32.y, &image->y)
	    || put_user(img32.width, &image->width)
	    || put_user(img32.height, &image->height)
	    || put_user(img32.data, &image->data))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_TEXTURE, (unsigned long)request);
}

typedef struct drm_radeon_vertex2_32 {
	int idx;		/* Index of vertex buffer */
	int discard;		/* Client finished with buffer? */
	int nr_states;
	u32 state;
	int nr_prims;
	u32 prim;
} drm_radeon_vertex2_32_t;

static int compat_radeon_cp_vertex2(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	drm_radeon_vertex2_32_t req32;
	drm_radeon_vertex2_t __user *request = (void __user *)arg;

	if (put_user(req32.idx, &request->idx)
	    || put_user(req32.discard, &request->discard)
	    || put_user(req32.nr_states, &request->nr_states)
	    || put_user(req32.state, &request->state)
	    || put_user(req32.nr_prims, &request->nr_prims)
	    || put_user(req32.prim, &request->prim))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_VERTEX2, (unsigned long)request);
}

typedef struct drm_radeon_cmd_buffer32 {
	int bufsz;
	u32 buf;
	int nbox;
	u32 boxes;
} drm_radeon_cmd_buffer32_t;

static int compat_radeon_cp_cmdbuf(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	drm_radeon_cmd_buffer32_t req32;
	drm_radeon_cmd_buffer_t __user *request = (void __user *)arg;

	if (put_user(req32.bufsz, &request->bufsz)
	    || put_user(req32.buf, &request->buf)
	    || put_user(req32.nbox, &request->nbox)
	    || put_user(req32.boxes, &request->boxes))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_CMDBUF, (unsigned long)request);
}

typedef struct drm_radeon_getparam32 {
	int param;
	u32 value;
} drm_radeon_getparam32_t;

static int compat_radeon_cp_getparam(struct file *file, unsigned int cmd,
				     unsigned long arg)
{
	drm_radeon_getparam32_t req32;
	drm_radeon_getparam_t __user *request = (void __user *)arg;

	if (put_user(req32.param, &request->param)
	    || put_user(req32.value, &request->value))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_GETPARAM, (unsigned long)request);
}

typedef struct drm_radeon_mem_alloc32 {
	int region;
	int alignment;
	int size;
	u32 region_offset;	/* offset from start of fb or GART */
} drm_radeon_mem_alloc32_t;

static int compat_radeon_mem_alloc(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	drm_radeon_mem_alloc32_t req32;
	drm_radeon_mem_alloc_t __user *request = (void __user *)arg;

	if (put_user(req32.region, &request->region)
	    || put_user(req32.alignment, &request->alignment)
	    || put_user(req32.size, &request->size)
	    || put_user(req32.region_offset, &request->region_offset))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_ALLOC, (unsigned long)request);
}

typedef struct drm_radeon_irq_emit32 {
	u32 irq_seq;
} drm_radeon_irq_emit32_t;

static int compat_radeon_irq_emit(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	drm_radeon_irq_emit32_t req32;
	drm_radeon_irq_emit_t __user *request = (void __user *)arg;

	if (put_user(req32.irq_seq, &request->irq_seq))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_IRQ_EMIT, (unsigned long)request);
}

/* The two 64-bit arches where alignof(u64)==4 in 32-bit code */
#if defined (CONFIG_X86_64) || defined(CONFIG_IA64)
typedef struct drm_radeon_setparam32 {
	int param;
	u64 value;
} __attribute__((packed)) drm_radeon_setparam32_t;

static int compat_radeon_cp_setparam(struct file *file, unsigned int cmd,
				     unsigned long arg)
{
	drm_radeon_setparam32_t req32;
	drm_radeon_setparam_t __user *request = (void __user *) arg;

	if (put_user(req32.param, &request->param)
	    || put_user(req32.value, &request->value))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_RADEON_SETPARAM, (unsigned long) request);
}
#else
#define compat_radeon_cp_setparam NULL
#endif /* X86_64 || IA64 */

static drm_ioctl_compat_t *radeon_compat_ioctls[] = {
	[DRM_RADEON_CP_INIT] = compat_radeon_cp_init,
	[DRM_RADEON_CLEAR] = compat_radeon_cp_clear,
	[DRM_RADEON_STIPPLE] = compat_radeon_cp_stipple,
	[DRM_RADEON_TEXTURE] = compat_radeon_cp_texture,
	[DRM_RADEON_VERTEX2] = compat_radeon_cp_vertex2,
	[DRM_RADEON_CMDBUF] = compat_radeon_cp_cmdbuf,
	[DRM_RADEON_GETPARAM] = compat_radeon_cp_getparam,
	[DRM_RADEON_SETPARAM] = compat_radeon_cp_setparam,
	[DRM_RADEON_ALLOC] = compat_radeon_mem_alloc,
	[DRM_RADEON_IRQ_EMIT] = compat_radeon_irq_emit,
};

/**
 * Called whenever a 32-bit process running under a 64-bit kernel
 * performs an ioctl on /dev/dri/card<n>.
 *
 * \param filp file pointer.
 * \param cmd command.
 * \param arg user argument.
 * \return zero on success or negative number on failure.
 */
long radeon_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int nr = DRM_IOCTL_NR(cmd);
	drm_ioctl_compat_t *fn = NULL;
	int ret;

	if (nr < DRM_COMMAND_BASE)
		return drm_compat_ioctl(filp, cmd, arg);

	if (nr < DRM_COMMAND_BASE + ARRAY_SIZE(radeon_compat_ioctls))
		fn = radeon_compat_ioctls[nr - DRM_COMMAND_BASE];

	if (fn != NULL)
		ret = (*fn) (filp, cmd, arg);
	else
		ret = drm_ioctl(filp, cmd, arg);

	return ret;
}

long radeon_kms_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int nr = DRM_IOCTL_NR(cmd);
	int ret;

	if (nr < DRM_COMMAND_BASE)
		return drm_compat_ioctl(filp, cmd, arg);

	ret = radeon_drm_ioctl(filp, cmd, arg);

	return ret;
}
#endif

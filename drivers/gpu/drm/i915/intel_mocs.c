/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions: *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "intel_mocs.h"
#include "intel_lrc.h"
#include "intel_ringbuffer.h"

/* structures required */
struct drm_i915_mocs_entry {
	u32 control_value;
	u16 l3cc_value;
};

struct drm_i915_mocs_table {
	u32 size;
	const struct drm_i915_mocs_entry *table;
};

/* Defines for the tables (XXX_MOCS_0 - XXX_MOCS_63) */
#define _LE_CACHEABILITY(value)	((value) << 0)
#define _LE_TGT_CACHE(value)	((value) << 2)
#define LE_LRUM(value)		((value) << 4)
#define LE_AOM(value)		((value) << 6)
#define LE_RSC(value)		((value) << 7)
#define LE_SCC(value)		((value) << 8)
#define LE_PFM(value)		((value) << 11)
#define LE_SCF(value)		((value) << 14)

/* Defines for the tables (LNCFMOCS0 - LNCFMOCS31) - two entries per word */
#define L3_ESC(value)		((value) << 0)
#define L3_SCC(value)		((value) << 1)
#define _L3_CACHEABILITY(value)	((value) << 4)

/* Helper defines */
#define GEN9_NUM_MOCS_ENTRIES	62  /* 62 out of 64 - 63 & 64 are reserved. */

/* (e)LLC caching options */
#define LE_0_PAGETABLE		_LE_CACHEABILITY(0)
#define LE_1_UC			_LE_CACHEABILITY(1)
#define LE_2_WT			_LE_CACHEABILITY(2)
#define LE_3_WB			_LE_CACHEABILITY(3)

/* Target cache */
#define LE_TC_0_PAGETABLE	_LE_TGT_CACHE(0)
#define LE_TC_1_LLC		_LE_TGT_CACHE(1)
#define LE_TC_2_LLC_ELLC	_LE_TGT_CACHE(2)
#define LE_TC_3_LLC_ELLC_ALT	_LE_TGT_CACHE(3)

/* L3 caching options */
#define L3_0_DIRECT		_L3_CACHEABILITY(0)
#define L3_1_UC			_L3_CACHEABILITY(1)
#define L3_2_RESERVED		_L3_CACHEABILITY(2)
#define L3_3_WB			_L3_CACHEABILITY(3)

/*
 * MOCS tables
 *
 * These are the MOCS tables that are programmed across all the rings.
 * The control value is programmed to all the rings that support the
 * MOCS registers. While the l3cc_values are only programmed to the
 * LNCFCMOCS0 - LNCFCMOCS32 registers.
 *
 * These tables are intended to be kept reasonably consistent across
 * platforms. However some of the fields are not applicable to all of
 * them.
 *
 * Entries not part of the following tables are undefined as far as
 * userspace is concerned and shouldn't be relied upon.  For the time
 * being they will be initialized to PTE.
 *
 * NOTE: These tables MUST start with being uncached and the length
 *       MUST be less than 63 as the last two registers are reserved
 *       by the hardware.  These tables are part of the kernel ABI and
 *       may only be updated incrementally by adding entries at the
 *       end.
 */
static const struct drm_i915_mocs_entry skylake_mocs_table[] = {
	[I915_MOCS_UNCACHED] = {
	  /* 0x00000009 */
	  .control_value = LE_1_UC | LE_TC_2_LLC_ELLC,
	  /* 0x0010 */
	  .l3cc_value =    L3_1_UC,
	},
	[I915_MOCS_PTE] = {
	  /* 0x00000038 */
	  .control_value = LE_0_PAGETABLE | LE_TC_2_LLC_ELLC | LE_LRUM(3),
	  /* 0x0030 */
	  .l3cc_value =    L3_3_WB,
	},
	[I915_MOCS_CACHED] = {
	  /* 0x0000003b */
	  .control_value = LE_3_WB | LE_TC_2_LLC_ELLC | LE_LRUM(3),
	  /* 0x0030 */
	  .l3cc_value =   L3_3_WB,
	},
};

/* NOTE: the LE_TGT_CACHE is not used on Broxton */
static const struct drm_i915_mocs_entry broxton_mocs_table[] = {
	[I915_MOCS_UNCACHED] = {
	  /* 0x00000009 */
	  .control_value = LE_1_UC | LE_TC_2_LLC_ELLC,
	  /* 0x0010 */
	  .l3cc_value = L3_1_UC,
	},
	[I915_MOCS_PTE] = {
	  /* 0x00000038 */
	  .control_value = LE_0_PAGETABLE | LE_TC_2_LLC_ELLC | LE_LRUM(3),
	  /* 0x0030 */
	  .l3cc_value = L3_3_WB,
	},
	[I915_MOCS_CACHED] = {
	  /* 0x00000039 */
	  .control_value = LE_1_UC | LE_TC_2_LLC_ELLC | LE_LRUM(3),
	  /* 0x0030 */
	  .l3cc_value = L3_3_WB,
	},
};

/**
 * get_mocs_settings()
 * @dev_priv:	i915 device.
 * @table:      Output table that will be made to point at appropriate
 *	      MOCS values for the device.
 *
 * This function will return the values of the MOCS table that needs to
 * be programmed for the platform. It will return the values that need
 * to be programmed and if they need to be programmed.
 *
 * Return: true if there are applicable MOCS settings for the device.
 */
static bool get_mocs_settings(struct drm_i915_private *dev_priv,
			      struct drm_i915_mocs_table *table)
{
	bool result = false;

	if (IS_GEN9_BC(dev_priv) || IS_CANNONLAKE(dev_priv) ||
	    IS_ICELAKE(dev_priv)) {
		table->size  = ARRAY_SIZE(skylake_mocs_table);
		table->table = skylake_mocs_table;
		result = true;
	} else if (IS_GEN9_LP(dev_priv)) {
		table->size  = ARRAY_SIZE(broxton_mocs_table);
		table->table = broxton_mocs_table;
		result = true;
	} else {
		WARN_ONCE(INTEL_GEN(dev_priv) >= 9,
			  "Platform that should have a MOCS table does not.\n");
	}

	/* WaDisableSkipCaching:skl,bxt,kbl,glk */
	if (IS_GEN(dev_priv, 9)) {
		int i;

		for (i = 0; i < table->size; i++)
			if (WARN_ON(table->table[i].l3cc_value &
				    (L3_ESC(1) | L3_SCC(0x7))))
				return false;
	}

	return result;
}

static i915_reg_t mocs_register(enum intel_engine_id engine_id, int index)
{
	switch (engine_id) {
	case RCS:
		return GEN9_GFX_MOCS(index);
	case VCS:
		return GEN9_MFX0_MOCS(index);
	case BCS:
		return GEN9_BLT_MOCS(index);
	case VECS:
		return GEN9_VEBOX_MOCS(index);
	case VCS2:
		return GEN9_MFX1_MOCS(index);
	case VCS3:
		return GEN11_MFX2_MOCS(index);
	default:
		MISSING_CASE(engine_id);
		return INVALID_MMIO_REG;
	}
}

/**
 * intel_mocs_init_engine() - emit the mocs control table
 * @engine:	The engine for whom to emit the registers.
 *
 * This function simply emits a MI_LOAD_REGISTER_IMM command for the
 * given table starting at the given address.
 */
void intel_mocs_init_engine(struct intel_engine_cs *engine)
{
	struct drm_i915_private *dev_priv = engine->i915;
	struct drm_i915_mocs_table table;
	unsigned int index;

	if (!get_mocs_settings(dev_priv, &table))
		return;

	GEM_BUG_ON(table.size > GEN9_NUM_MOCS_ENTRIES);

	for (index = 0; index < table.size; index++)
		I915_WRITE(mocs_register(engine->id, index),
			   table.table[index].control_value);

	/*
	 * Now set the unused entries to PTE. These entries are officially
	 * undefined and no contract for the contents and settings is given
	 * for these entries.
	 */
	for (; index < GEN9_NUM_MOCS_ENTRIES; index++)
		I915_WRITE(mocs_register(engine->id, index),
			   table.table[I915_MOCS_PTE].control_value);
}

/**
 * emit_mocs_control_table() - emit the mocs control table
 * @rq:	Request to set up the MOCS table for.
 * @table:	The values to program into the control regs.
 *
 * This function simply emits a MI_LOAD_REGISTER_IMM command for the
 * given table starting at the given address.
 *
 * Return: 0 on success, otherwise the error status.
 */
static int emit_mocs_control_table(struct i915_request *rq,
				   const struct drm_i915_mocs_table *table)
{
	enum intel_engine_id engine = rq->engine->id;
	unsigned int index;
	u32 *cs;

	if (WARN_ON(table->size > GEN9_NUM_MOCS_ENTRIES))
		return -ENODEV;

	cs = intel_ring_begin(rq, 2 + 2 * GEN9_NUM_MOCS_ENTRIES);
	if (IS_ERR(cs))
		return PTR_ERR(cs);

	*cs++ = MI_LOAD_REGISTER_IMM(GEN9_NUM_MOCS_ENTRIES);

	for (index = 0; index < table->size; index++) {
		*cs++ = i915_mmio_reg_offset(mocs_register(engine, index));
		*cs++ = table->table[index].control_value;
	}

	/*
	 * Now set the unused entries to PTE. These entries are officially
	 * undefined and no contract for the contents and settings is given
	 * for these entries.
	 */
	for (; index < GEN9_NUM_MOCS_ENTRIES; index++) {
		*cs++ = i915_mmio_reg_offset(mocs_register(engine, index));
		*cs++ = table->table[I915_MOCS_PTE].control_value;
	}

	*cs++ = MI_NOOP;
	intel_ring_advance(rq, cs);

	return 0;
}

static inline u32 l3cc_combine(const struct drm_i915_mocs_table *table,
			       u16 low,
			       u16 high)
{
	return table->table[low].l3cc_value |
	       table->table[high].l3cc_value << 16;
}

/**
 * emit_mocs_l3cc_table() - emit the mocs control table
 * @rq:	Request to set up the MOCS table for.
 * @table:	The values to program into the control regs.
 *
 * This function simply emits a MI_LOAD_REGISTER_IMM command for the
 * given table starting at the given address. This register set is
 * programmed in pairs.
 *
 * Return: 0 on success, otherwise the error status.
 */
static int emit_mocs_l3cc_table(struct i915_request *rq,
				const struct drm_i915_mocs_table *table)
{
	unsigned int i;
	u32 *cs;

	if (WARN_ON(table->size > GEN9_NUM_MOCS_ENTRIES))
		return -ENODEV;

	cs = intel_ring_begin(rq, 2 + GEN9_NUM_MOCS_ENTRIES);
	if (IS_ERR(cs))
		return PTR_ERR(cs);

	*cs++ = MI_LOAD_REGISTER_IMM(GEN9_NUM_MOCS_ENTRIES / 2);

	for (i = 0; i < table->size / 2; i++) {
		*cs++ = i915_mmio_reg_offset(GEN9_LNCFCMOCS(i));
		*cs++ = l3cc_combine(table, 2 * i, 2 * i + 1);
	}

	if (table->size & 0x01) {
		/* Odd table size - 1 left over */
		*cs++ = i915_mmio_reg_offset(GEN9_LNCFCMOCS(i));
		*cs++ = l3cc_combine(table, 2 * i, I915_MOCS_PTE);
		i++;
	}

	/*
	 * Now set the unused entries to PTE. These entries are officially
	 * undefined and no contract for the contents and settings is given
	 * for these entries.
	 */
	for (; i < GEN9_NUM_MOCS_ENTRIES / 2; i++) {
		*cs++ = i915_mmio_reg_offset(GEN9_LNCFCMOCS(i));
		*cs++ = l3cc_combine(table, I915_MOCS_PTE, I915_MOCS_PTE);
	}

	*cs++ = MI_NOOP;
	intel_ring_advance(rq, cs);

	return 0;
}

/**
 * intel_mocs_init_l3cc_table() - program the mocs control table
 * @dev_priv:      i915 device private
 *
 * This function simply programs the mocs registers for the given table
 * starting at the given address. This register set is  programmed in pairs.
 *
 * These registers may get programmed more than once, it is simpler to
 * re-program 32 registers than maintain the state of when they were programmed.
 * We are always reprogramming with the same values and this only on context
 * start.
 *
 * Return: Nothing.
 */
void intel_mocs_init_l3cc_table(struct drm_i915_private *dev_priv)
{
	struct drm_i915_mocs_table table;
	unsigned int i;

	if (!get_mocs_settings(dev_priv, &table))
		return;

	for (i = 0; i < table.size / 2; i++)
		I915_WRITE(GEN9_LNCFCMOCS(i),
			   l3cc_combine(&table, 2 * i, 2 * i + 1));

	/* Odd table size - 1 left over */
	if (table.size & 0x01) {
		I915_WRITE(GEN9_LNCFCMOCS(i),
			   l3cc_combine(&table, 2 * i, I915_MOCS_PTE));
		i++;
	}

	/* Now set the rest of the table to PTE */
	for (; i < (GEN9_NUM_MOCS_ENTRIES / 2); i++)
		I915_WRITE(GEN9_LNCFCMOCS(i),
			   l3cc_combine(&table, I915_MOCS_PTE, I915_MOCS_PTE));
}

/**
 * intel_rcs_context_init_mocs() - program the MOCS register.
 * @rq:	Request to set up the MOCS tables for.
 *
 * This function will emit a batch buffer with the values required for
 * programming the MOCS register values for all the currently supported
 * rings.
 *
 * These registers are partially stored in the RCS context, so they are
 * emitted at the same time so that when a context is created these registers
 * are set up. These registers have to be emitted into the start of the
 * context as setting the ELSP will re-init some of these registers back
 * to the hw values.
 *
 * Return: 0 on success, otherwise the error status.
 */
int intel_rcs_context_init_mocs(struct i915_request *rq)
{
	struct drm_i915_mocs_table t;
	int ret;

	if (get_mocs_settings(rq->i915, &t)) {
		/* Program the RCS control registers */
		ret = emit_mocs_control_table(rq, &t);
		if (ret)
			return ret;

		/* Now program the l3cc registers */
		ret = emit_mocs_l3cc_table(rq, &t);
		if (ret)
			return ret;
	}

	return 0;
}

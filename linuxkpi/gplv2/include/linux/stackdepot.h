#pragma once

// i915 uses stackdepot with CONFIG_DRM_I915_DEBUG_RUNTIME_PM
// but needs the type regardless

typedef bool depot_stack_handle_t;

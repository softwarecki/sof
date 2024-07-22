/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __MODULE_MODULE_INSTANCE__
#define __MODULE_MODULE_INSTANCE__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct module_instance {
	uint32_t module_id;
	uint32_t instance_id;
	uint32_t core_id;
};

/* Initialize new module_instance */
void module_instance_init(struct module_instance *instance, uint32_t module_id,
			  uint32_t instance_id, uint32_t core_id);

/* Free module_instance object */
void module_instance_free(struct module_instance *instance);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __MODULE_MODULE_INSTANCE__ */

/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#include <stddef.h>
#include <stdint.h>

#include <sof/audio/module_adapter/module/instance.h>

void module_instance_init(struct module_instance *instance, uint32_t module_id,
			  uint32_t instance_id, uint32_t core_id)
{
	instance->module_id = module_id;
	instance->instance_id = instance_id;
	instance->core_id = core_id;
}

/* Free module_instance object */
void module_instance_free(struct module_instance *instance) { };

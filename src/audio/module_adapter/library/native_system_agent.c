// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 - 2024 Intel Corporation. All rights reserved.
 *
 * Author: Pawel Dobrowolski <pawelx.dobrowolski@intel.com>
 *	   Adrian Warecki <adrian.warecki@intel.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/util.h>

#include <utilities/array.h>

#include <native_system_service.h>
#include <native_system_agent.h>
#include <sof/audio/module_adapter/module/instance.h>


static int native_system_agent_check_in(struct system_agent *agent,
					const struct module_interface *interface, void *placeholder,
					size_t size)
{
	struct native_system_agent *self = CONTAINER_OF(agent, struct native_system_agent, public);
	self->mod_instance = (struct module_instance *)placeholder;
	(void)interface;

	self->module_size = size;

	module_instance_init(self->mod_instance, self->public.module_id, self->public.instance_id,
			     self->public.core_id);
	return 0;
}

/* The create_instance_f is a function call type known in module. The module entry_point
 * points to this type of function which starts module creation.
 */

typedef int (*native_create_instance_f)(const void *mod_cfg, uint32_t instance_id, void **mod_ptr);


int native_system_agent_start(uint32_t entry_point, uint32_t module_id, uint32_t instance_id,
			      uint32_t core_id, uint32_t log_handle, void *mod_cfg,
			      const struct module_interface **interface,
			      struct module_instance **instance)
{
	struct native_system_agent agent;
	int ret;

	agent.public.system_service = &native_system_service.basic;
	agent.public.check_in = native_system_agent_check_in;
	agent.public.module_id = module_id;
	agent.public.instance_id = instance_id;
	agent.public.core_id = core_id;
	agent.public.log_handle = log_handle;

	void *system_agent_p = &agent.public;

	native_create_instance_f ci = (native_create_instance_f)entry_point;

	ret = ci(mod_cfg, instance_id, &system_agent_p);
	*interface = (const struct module_interface *)system_agent_p;
	*instance = agent.mod_instance;
	return ret;
}

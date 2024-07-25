// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

/* The source code of the loadable module entry point is provided for building convenience.
 * However it is not expected to be modified as its content is tightly tied to the SOF.
 */

#include <module/module/handle.h>
#include <module/module/system_agent.h>

struct module_interface;

int loadable_module_main(const void *mod_cfg, uint32_t instance_id,
			 void **system_agent_p, const struct module_interface *interface,
			 void *module_placeholder, size_t module_size)
{
	struct module_handle *handle = (struct module_handle *)module_placeholder;
	struct system_agent *agent = (struct system_agent *)*system_agent_p;

	handle->system_service = agent->system_service;
	handle->log_handle = agent->log_handle;

	*system_agent_p = (void*)interface;

	return agent->check_in(agent, interface, &handle->buffer, module_size);
}

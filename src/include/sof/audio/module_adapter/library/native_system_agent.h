/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 * Author: Pawel Dobrowolski <pawelx.dobrowolski@intel.com>
 */

#ifndef __NATIVE_SYSTEM_AGENT_H__
#define __NATIVE_SYSTEM_AGENT_H__

#include <stdint.h>
#include <sof/audio/module_adapter/module/module_interface.h>
#include <sof/audio/module_adapter/module/instance.h>
#include <native_system_service.h>
#include <module/module/system_agent.h>

struct native_system_agent {
	struct system_agent public;
	size_t module_size;
	struct module_instance *mod_instance;
};

int native_system_agent_start(uint32_t entry_point, uint32_t module_id, uint32_t instance_id,
			      uint32_t core_id, uint32_t log_handle, void *mod_cfg,
			      const struct module_interface **interface,
			      struct module_instance **instance);

#endif /* __NATIVE_SYSTEM_AGENT_H__ */

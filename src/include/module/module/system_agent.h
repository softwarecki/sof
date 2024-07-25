/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 * Author: Pawel Dobrowolski <pawelx.dobrowolski@intel.com>
 *	   Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __MODULE_SYSTEM_AGENT_H__
#define __MODULE_SYSTEM_AGENT_H__

#include <stdint.h>

#include "interface.h"
#include "system_service.h"

struct system_agent {
	const struct system_service* system_service;

	int (*check_in)(struct system_agent *agent, const struct module_interface *interface,
			void *placeholder, size_t size);

	uint32_t log_handle;
	uint32_t core_id;
	uint32_t module_id;
	uint32_t instance_id;
};

#endif /* __MODULE_SYSTEM_AGENT_H__ */

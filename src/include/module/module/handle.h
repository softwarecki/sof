/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __MODULE_MODULE_HANDLE__
#define __MODULE_MODULE_HANDLE__

#include <stdint.h>
#include <stddef.h>
#include <rimage/sof/user/manifest.h>
#include "api_ver.h"

struct system_service;
struct module_interface;

/*
 * Structure module_handle allows a loadable modules handling by the SOF.
 *
 * It contains actually some buffer which the module shall provide to the SOF.
 *
 * The size of the buffer may depend on the API version exposed by the SOF. So a loadable module
 * needs to be recompiled when major or middle number of the System API version is changed
 * (API version have format [major number].[middle number].[minor number])
 */
struct module_handle {
	intptr_t buffer[(MODULE_PASS_BUFFER_SIZE + sizeof(intptr_t)-1) / sizeof(intptr_t)];
	const struct system_service *system_service;
	uint32_t log_handle;
};

#define MODULE_ALIGN_SIZE(size, align)	(((size) + (align) - 1) / (align))

/* Defines variable name for the first instance of module placeholder - internal purpose. */
#define MODULE_PLACEHOLDER_NAME(module) \
	module ## _placeholder

/* Defines variable name for module placeholder size - internal purpose. */
#define MODULE_PLACEHOLDER_SIZE_NAME(module) \
	module ## _placeholder_size

/* Defines variable name for module placeholder array length - internal purpose. */
#define MODULE_PLACEHOLDER_LENGTH_NAME(module) \
	module ## _placeholder_length

/* Defines the module manifest name - internal purpose. */
#define MODULE_MANIFEST_NAME(module) \
	module ## _module_manifest

/* Defines the module entry point function name - internal purpose. */
#define MODULE_ENTRY_POINT_NAME(module) \
	module ## _module_entry_point

/* Defines module placeholder structure - internal purpose. */
#define DECLARE_LOADABLE_MODULE_PLACEHOLDER(module, data_size, max_instances)			\
	enum {											\
		MODULE_PLACEHOLDER_SIZE_NAME(module) =						\
			((data_size + MODULE_INSTANCE_ALIGNMENT) &				\
			(~(MODULE_INSTANCE_ALIGNMENT - 1))),					\
		MODULE_PLACEHOLDER_LENGTH_NAME(module) = MODULE_ALIGN_SIZE(			\
			MODULE_PLACEHOLDER_SIZE_NAME(module), sizeof(intptr_t))			\
	};											\
	static intptr_t MODULE_PLACEHOLDER_NAME(module)[					\
		MODULE_PLACEHOLDER_LENGTH_NAME(module) * max_instances];

/* Loadable module default entry point function - internal purpose. */
int loadable_module_main(const void *mod_cfg, uint32_t instance_id,
			 void **system_agent_p, const struct module_interface *interface,
			 void *module_placeholder, size_t module_size);

/* Loadable module entry point function - internal purpose. */
#define DECLARE_LOADABLE_MODULE_ENTRY_POINT(module, interface, data_size)			\
	int MODULE_ENTRY_POINT_NAME(module)(const void *mod_cfg, uint32_t instance_id,		\
					    void **system_agent_p)				\
	{											\
		return loadable_module_main(mod_cfg, instance_id, system_agent_p, interface,	\
					    &MODULE_PLACEHOLDER_NAME(module)[			\
						MODULE_PLACEHOLDER_LENGTH_NAME(module) *	\
						instance_id],					\
					    data_size);						\
	}

/* Loadable module manifest structure - internal purpose. */
#define DECLARE_LOADABLE_MODULE_MANIFEST(mod, manifest_name, affinity, instances, mod_uuid...)	\
	__attribute__((section(".module")))							\
	const struct sof_man_module_manifest MODULE_MANIFEST_NAME(mod) = {			\
		.module = {									\
			.name = manifest_name,							\
			.uuid = { mod_uuid },							\
			.entry_point = (uint32_t)&MODULE_ENTRY_POINT_NAME(mod),			\
			.instance_max_count = instances,					\
			.type = {								\
				.load_type = SOF_MAN_MOD_TYPE_MODULE,				\
				.domain_ll = 1,							\
			},									\
			.affinity_mask = (affinity),						\
		}										\
	};



/* Declare a loadable module for the SOF.
 *
 * @param module Name of the module
 * @param interface Pointer to the module interface structure
 * @param manifest_name Name of the module presented in the modules manifest
 * @param affinity Module affinity mask
 * @param mod_uuid UUID of the module
 * @param max_instances Maximum allowed module instance count
 */
#define DECLARE_LOADABLE_MODULE(module, interface, data_size, manifest_name, affinity, mod_uuid,\
				max_instances)							\
	DECLARE_LOADABLE_MODULE_API_VERSION(module);						\
	DECLARE_LOADABLE_MODULE_PLACEHOLDER(module, data_size, max_instances);			\
	DECLARE_LOADABLE_MODULE_ENTRY_POINT(module, interface, data_size);			\
	DECLARE_LOADABLE_MODULE_MANIFEST(module, manifest_name, affinity, max_instances, mod_uuid);

#endif /* __MODULE_MODULE_HANDLE__ */

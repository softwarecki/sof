/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 */

#ifndef MODULE_LLEXT_H
#define MODULE_LLEXT_H

#define SOF_LLEXT_MODULE_MANIFEST(manifest_name, entry, affinity, mod_uuid, instances) \
{ \
	.module = { \
		.name = manifest_name, \
		.uuid = {mod_uuid},			  \
		.entry_point = (uint32_t)(entry), \
		.instance_max_count = instances, \
		.type = { \
			.load_type = SOF_MAN_MOD_TYPE_LLEXT, \
			.domain_ll = 1, \
		}, \
		.affinity_mask = (affinity), \
	} \
}

#define SOF_LLEXT_MOD_ENTRY(name, interface) \
static int name##_llext_entry(const void *mod_cfg, uint32_t instance_id, void **mod_ptr) \
{ \
	*(const struct module_interface **)mod_ptr = interface; \
	return 0; \
}

#define SOF_LLEXT_BUILDINFO \
static const struct sof_module_api_build_info buildinfo __section(".mod_buildinfo") __used = { \
	.format = SOF_MODULE_API_BUILD_INFO_FORMAT, \
	.api_version_number.full = SOF_MODULE_API_CURRENT_VERSION, \
}

#endif

// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#include <module/base.h>
#include <module/handle.h>

#include "example.h"

/*
 * Module specific initialization procedure, called as part of
 * module_adapter component creation in .new()
 */
static int example_init(struct processing_module *mod)
{
	return 0;
}

/*
 * Module specific processing procedure
 * This procedure is responsible to consume
 * samples provided by the module_adapter and produce/output the processed
 * ones back to module_adapter.
 *
 * there are 3 versions of the procedure, the difference is the format of
 * input/output data
 *
 * the module MUST implement one and ONLY one of them
 *
 * process_audio_stream and process_raw_data are depreciated and will be removed
 * once pipeline learns to use module API directly (without module adapter)
 * modules that need such processing should use proper wrappers
 *
 * process
 *	- sources are handlers to source API struct source*[]
 *	- sinks are handlers to sink API struct sink*[]
 */
static int example_process(struct processing_module *mod,
			   struct sof_source **sources, int num_of_sources,
			   struct sof_sink **sinks, int num_of_sinks)
{
	return 0;
}

static const struct module_interface example_interface = {
	.init = example_init,
	.process = example_process
};

DECLARE_LOADABLE_MODULE(example, &example_interface, sizeof(struct example_data), "EXAMPLE",
			EXAMPLE_AFFINITY, EXAMPLE_UUID, EXAMPLE_MAX_INSTANCES)

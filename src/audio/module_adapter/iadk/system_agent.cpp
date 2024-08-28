// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Jaroslaw Stelter <jaroslaw.stelter@linux.intel.com>

/*
 * SOF System Agent - register IADK Loadable Library in SOF infrastructure.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <rtos/string.h>
#include <utilities/array.h>
#include <module/iadk/adsp_error_code.h>
#include <system_service.h>
#include <system_agent_interface.h>
#include <module_initial_settings_concrete.h>
#include <iadk_module_adapter.h>
#include <system_agent.h>
#include <sof/audio/module_adapter/library/native_system_service.h>

using namespace intel_adsp;
using namespace intel_adsp::system;
using namespace dsp_fw;

void* operator new(size_t size, intel_adsp::ModuleHandle *placeholder) throw()
{
	return placeholder;
}

namespace intel_adsp
{
namespace system
{

/* Structure storing handles to system service operations */
const AdspSystemService SystemAgent::system_service_ = {
	native_system_service_log_message,
	native_system_service_safe_memcpy,
	native_system_service_safe_memmove,
	native_system_service_vec_memset,
	native_system_service_create_notification,
	native_system_service_send_notif_msg,
	native_system_service_get_interface,
};

SystemAgent::SystemAgent(uint32_t module_id,
			 uint32_t instance_id,
			 uint32_t core_id,
			 uint32_t log_handle) :
			     log_handle_(log_handle),
			     core_id_(core_id),
			     module_id_(module_id),
			     instance_id_(instance_id),
			     module_handle_(NULL),
			     module_size_(0)
{}

void SystemAgent::CheckIn(ProcessingModuleInterface& processing_module,
			  ModuleHandle &module_handle,
			  LogHandle *&log_handle)
{
	module_handle_ = &module_handle;
	/* Initializes the ModuleAdapter into the ModuleHandle */
	IadkModuleAdapter* module_adapter =
		new (module_handle_)IadkModuleAdapter(processing_module,
		NULL,
		module_id_,
		instance_id_,
		core_id_,
		module_size_
		);
	(void)module_adapter;
	log_handle = reinterpret_cast<LogHandle*>(log_handle_);
}

int SystemAgent::CheckIn(ProcessingModuleFactoryInterface& module_factory,
			 ModulePlaceholder *module_placeholder,
			 size_t processing_module_size,
			 uint32_t core_id,
			 const void *obfuscated_mod_cfg,
			 void *obfuscated_parent_ppl,
			 void **obfuscated_modinst_p)
{
	IoPinsInfo pins_info;
	const dsp_fw::DwordArray& cfg_ipc_msg =
			*reinterpret_cast<const dsp_fw::DwordArray*>(obfuscated_mod_cfg);
	ModuleInitialSettingsConcrete settings(cfg_ipc_msg);

	ProcessingModulePrerequisites prerequisites;
	module_factory.GetPrerequisites(prerequisites);

	/* Note: If module has no output pins, it requires HungryRTSink */
	/*       to terminate parent Pipeline */
	const bool hungry_rt_sink_required = prerequisites.output_pins_count == 0;
	if (hungry_rt_sink_required) prerequisites.output_pins_count = 1;

	if ((prerequisites.input_pins_count < 1) ||
		(prerequisites.input_pins_count > INPUT_PIN_COUNT) ||
		(prerequisites.output_pins_count < 1) ||
		(prerequisites.output_pins_count > OUTPUT_PIN_COUNT))
		return -1;

	/* Deduce BaseModuleCfgExt if it was not part of the INIT_INSTANCE IPC message */
	settings.DeduceBaseModuleCfgExt(prerequisites.input_pins_count,
					prerequisites.output_pins_count);

	module_factory.Create(*this, module_placeholder, ModuleInitialSettings(settings), pins_info);
	IadkModuleAdapter& module_adapter = *reinterpret_cast<IadkModuleAdapter*>(module_handle_);
	*obfuscated_modinst_p = &module_adapter;
	reinterpret_cast<intel_adsp::ProcessingModuleInterface*>(module_placeholder)->Init();
	return 0;
}

} /* namespace system */
} /* namespace intel_adsp */

/* The create_instance_f is a function call type known in IADK module. The module entry_point
 * points to this type of function which starts module creation.
 */
typedef int (*create_instance_f)(uint32_t module_id, uint32_t instance_id, uint32_t core_id,
				 void *mod_cfg, void *parent_ppl, void **mod_ptr);

int system_agent_start(uint32_t entry_point, uint32_t module_id, uint32_t instance_id,
		       uint32_t core_id, uint32_t log_handle, void* mod_cfg,
		       void **adapter, struct module_instance **instance)
{
	uint32_t ret;
	SystemAgent system_agent(module_id, instance_id, core_id, log_handle);
	void* system_agent_p = reinterpret_cast<void*>(&system_agent);

	create_instance_f ci = (create_instance_f)(entry_point);
	ret = ci(module_id, instance_id, core_id, mod_cfg, NULL, &system_agent_p);

	IadkModuleAdapter* module_adapter = reinterpret_cast<IadkModuleAdapter*>(system_agent_p);
	*adapter = module_adapter;
	*instance = static_cast<struct module_instance*>(module_adapter);
	return ret;
}

extern "C" void __cxa_pure_virtual()  __attribute__((weak));

void __cxa_pure_virtual()
{
}


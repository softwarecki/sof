#include <stdint.h>

#include "../include/loadable_processing_module.h"
#include <rimage/sof/user/manifest.h>

//struct _module_example{
//	int cos;
//	}module_example;

DECLARE_LOADABLE_MODULE(module_example)

int loadable_module_main(void){
	while(1);
	return 0;
}

__attribute__((section(".module")))
const struct sof_man_module_manifest main_manifest = {
	.module = {
		.name = "MODEXMPL",
		.uuid = {0x2d, 0x2d, 0x2d, 0x73, 0x6f, 0x66, 0x74, 0x77,
			 0x61, 0x72, 0x65, 0x63, 0x6b, 0x69, 0x2d, 0x2d},
		.entry_point = (uint32_t)MODULE_PACKAGE_ENTRY_POINT_NAME(module_example),
		.type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
		.domain_ll = 1 },
		.affinity_mask = 3,
}
};
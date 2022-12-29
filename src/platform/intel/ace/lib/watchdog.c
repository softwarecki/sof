// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2023 Intel Corporation. All rights reserved.
//
// Author: Adrian Warecki <adrian.warecki@intel.com>

#include <sof/lib/uuid.h>
#include <sof/drivers/idc.h>
#include <sof/schedule/ll_schedule_domain.h>
#include <sof/ipc/msg.h>
#include <ipc4/notification.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>

#include <adsp_watchdog.h>
#include <intel_adsp_ipc.h>
#include <intel_adsp_ipc_devtree.h>

LOG_MODULE_REGISTER(wdt, CONFIG_SOF_LOG_LEVEL);

/* 13c8bc59-c4fa-4ad1-b93a-ce97cd30acc7 */
DECLARE_SOF_UUID("wdt", wdt_uuid, 0x13c8bc59, 0xc4fa, 0x4ad1,
		 0xb9, 0x3a, 0xce, 0x97, 0xcd, 0x30, 0xac, 0xc7);

DECLARE_TR_CTX(wdt_tr, SOF_UUID(wdt_uuid), LOG_LEVEL_INFO);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adsp_watchdog), okay)
const struct device *const watchdog = DEVICE_DT_GET(DT_NODELABEL(adsp_watchdog));
struct ipc_msg slave_timeout_ipc;

void wdt_slave_core_action_on_timeout()
{
	struct idc_msg msg;

	msg.header = IDC_MSG_SLAVE_CORE_CRASHED | IDC_SCC_CORE(cpu_get_id()) |
		     IDC_SCC_REASON(IDC_SCC_REASON_WATCHDOG);
	msg.extension = 0;
	msg.core = 0;
	msg.size = 0;
	msg.payload = NULL;
	idc_send_msg(&msg, IDC_NON_BLOCKING);
}

void wdt_master_core_action_on_timeout()
{
	struct ipc4_watchdog_timeout_notification notif;

	/* Send Watchdog Timeout IPC notification */
	ipc4_notification_watchdog_init(&notif, cpu_get_id(), true);
	intel_adsp_ipc_send_message_emergency(INTEL_ADSP_IPC_HOST_DEV,
					      notif.primary.dat, notif.extension.dat);
}

static void wdt_timeout(const struct device *dev, int core)
{
	if (!core)
		wdt_master_core_action_on_timeout();
	else
		wdt_slave_core_action_on_timeout();
}

void watchdog_init(void)
{
	int i, ret;
	const struct wdt_timeout_cfg watchdog_config = {
		.window.max = 2 * LL_TIMER_PERIOD_US / 1000,
		.callback = wdt_timeout,
	};

	slave_timeout_ipc.tx_data = NULL;
	slave_timeout_ipc.tx_size = 0;
	list_init(&slave_timeout_ipc.list);

	ret = wdt_install_timeout(watchdog, &watchdog_config);
	if (ret) {
		tr_warn(&wdt_tr, "Watchdog install timeout error %d", ret);
		return;
	}

	for (i = 0; i < CONFIG_CORE_COUNT; i++)
		intel_adsp_watchdog_pause(watchdog, i);

	ret = wdt_setup(watchdog, 0);
	if (ret)
		tr_warn(&wdt_tr, "Watchdog setup error %d", ret);
}

void watchdog_enable(int core)
{
	intel_adsp_watchdog_resume(watchdog, core);
}

void watchdog_disable(int core)
{
	intel_adsp_watchdog_pause(watchdog, core);
}

void watchdog_feed(int core)
{
	wdt_feed(watchdog, core);
}

void watchdog_slave_core_timeout(int core)
{
	struct ipc4_watchdog_timeout_notification notif;

	/* Send Watchdog Timeout IPC notification */
	ipc4_notification_watchdog_init(&notif, core, true);
	slave_timeout_ipc.header = notif.primary.dat;
	slave_timeout_ipc.extension = notif.extension.dat;
	ipc_msg_send(&slave_timeout_ipc, NULL, true);
}
#else
void watchdog_enable(int core) {}
void watchdog_disable(int core) {}
void watchdog_feed(int core) {}
void watchdog_init() {}
void watchdog_slave_core_timeout(int core) {}
#endif

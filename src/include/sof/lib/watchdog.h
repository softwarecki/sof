/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __SOF_LIB_WATCHDOG_H__
#define __SOF_LIB_WATCHDOG_H__

#include <sof/common.h>

#if IS_ENABLED(CONFIG_LL_WATCHDOG)
void watchdog_enable(int core);
void watchdog_disable(int core);
void watchdog_feed(int core);
void watchdog_init();
void watchdog_slave_core_timeout(int core);
#else
static inline void watchdog_enable(int core) {}
static inline void watchdog_disable(int core) {}
static inline void watchdog_feed(int core) {}
static inline void watchdog_init() {}
static inline void watchdog_slave_core_timeout(int core) {}
#endif

#endif /* __SOF_LIB_WATCHDOG_H__ */

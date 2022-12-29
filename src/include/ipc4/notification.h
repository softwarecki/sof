/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */

/*
 * This file contains structures that are exact copies of an existing ABI used
 * by IOT middleware. They are Intel specific and will be used by one middleware.
 *
 * Some of the structures may contain programming implementations that makes them
 * unsuitable for generic use and general usage.
 *
 * This code is mostly copied "as-is" from existing C++ interface files hence the use of
 * different style in places. The intention is to keep the interface as close as possible to
 * original so it's easier to track changes with IPC host code.
 */

/**
 * \file include/ipc4/notification.h
 * \brief IPC4 notification definitions
 */

#ifndef __IPC4_NOTIFICATION_H__
#define __IPC4_NOTIFICATION_H__

#include <stdint.h>
#include <ipc/header.h>

/* ipc4 notification msg */
enum sof_ipc4_notification_type {
	SOF_IPC4_NOTIFY_PHRASE_DETECTED		= 4,
	SOF_IPC4_NOTIFY_RESOURCE_EVENT		= 5,
	SOF_IPC4_NOTIFY_LOG_BUFFER_STATUS	= 6,
	SOF_IPC4_NOTIFY_TIMESTAMP_CAPTURED	= 7,
	SOF_IPC4_NOTIFY_FW_READY		= 8,
	SOF_IPC4_FW_AUD_CLASS_RESULT		= 9,
	SOF_IPC4_EXCEPTION_CAUGHT		= 10,
	SOF_IPC4_MODULE_NOTIFICATION		= 12,
	SOF_IPC4_UAOL_RSVD_		= 13,
	SOF_IPC4_PROBE_DATA_AVAILABLE		= 14,
	SOF_IPC4_WATCHDOG_TIMEOUT		= 15,
	SOF_IPC4_MANAGEMENT_SERVICE		= 16,
};

#define SOF_IPC4_GLB_NOTIFY_DIR_MASK		BIT(29)
#define SOF_IPC4_REPLY_STATUS_MASK		0xFFFFFF
#define SOF_IPC4_GLB_NOTIFY_TYPE_SHIFT		16
#define SOF_IPC4_GLB_NOTIFY_MSG_TYPE_SHIFT	24

#define SOF_IPC4_FW_READY \
		(((SOF_IPC4_NOTIFY_FW_READY) << (SOF_IPC4_GLB_NOTIFY_TYPE_SHIFT)) |\
		((SOF_IPC4_GLB_NOTIFICATION) << (SOF_IPC4_GLB_NOTIFY_MSG_TYPE_SHIFT)))

#define SOF_IPC4_NOTIF_HEADER(notif_type) \
		((notif_type) << (SOF_IPC4_GLB_NOTIFY_TYPE_SHIFT) | \
		((SOF_IPC4_GLB_NOTIFICATION) << (SOF_IPC4_GLB_NOTIFY_MSG_TYPE_SHIFT)))

#endif

/**
 * \brief IPC MAJOR 4 notification header. All IPC4 notifications use this header.
 */
union ipc4_notification_header {
	uint32_t dat;

	struct {
		uint32_t rsvd0 : 16;

		/**< Notification::MODULE_EVENT */
		uint32_t notif_type : 8;

		/**< One of Global::Type */
		uint32_t type : 5;

		/**< Msg::MSG_REQUEST */
		uint32_t rsp : 1;

		/**< Msg::FW_GEN_MSG */
		uint32_t msg_tgt : 1;

		uint32_t _reserved_0 : 1;
	} r;
} __packed __aligned(4);

/**
 * \brief This notification is reported by the Detector module upon key phrase detection.
 */
struct ipc4_voice_cmd_notification {
	union {
		uint32_t dat;

		struct {
			/**< ID of detected keyword */
			uint32_t word_id : 16;
			/**< Notification::PHRASE_DETECTED */
			uint32_t notif_type : 8;
			/**< Global::NOTIFICATION */
			uint32_t type : 5;
			/**< Msg::MSG_NOTIFICATION */
			uint32_t rsp : 1;
			/**< Msg::FW_GEN_MSG */
			uint32_t msg_tgt : 1;
			uint32_t _reserved_0 : 1;
		} r;
	} primary;

	union {
		uint32_t dat;

		struct {
			/**< Final speaker verification score in range of 0..8192 */
			uint32_t sv_score : 16;
			uint32_t rsvd1 : 14;
			uint32_t _reserved_2 : 2;
		} r;
	} extension;
} __packed __aligned(4);

/**
 * \brief This notification is reported by the Base FW when DSP core receive WDT timeout interrupt.
 */
struct ipc4_watchdog_timeout_notification {
	union {
		uint32_t dat;

		struct {
			/* ID of a core that timeouted. */
			uint32_t core_id : 4;
			/* Indicates that it was first timeout and crash dump was done */
			uint32_t first_timeout : 1;
			uint32_t rsvd : 11;
			/* Notification::WATCHDOG_TIMEOUT */
			uint32_t notif_type : 8;
			/* Global::NOTIFICATION */
			uint32_t type : 5;
			/* Msg::MSG_NOTIFICATION (0) */
			uint32_t rsp : 1;
			/* Msg::FW_GEN_MSG */
			uint32_t msg_tgt : 1;
			uint32_t _hw_rsvd_0 : 1;
		} r;
	} primary;

	union {
		uint32_t dat;

		struct {
			uint32_t rsvd1 : 30;
			uint32_t _hw_rsvd_2 : 2;
		} r;
	} extension;
} __packed __aligned(4);

static inline void ipc4_notification_watchdog_init(struct ipc4_watchdog_timeout_notification* notif,
						   uint32_t core_id, bool first_timeout)
{
	notif->primary.dat = 0;
	notif->extension.dat = 0;

	/* ID of a core that timeouted. */
	notif->primary.r.core_id = core_id;

	/* Indicates that it was first timeout and crash dump was done */
	notif->primary.r.first_timeout = first_timeout;

	notif->primary.r.notif_type = SOF_IPC4_WATCHDOG_TIMEOUT;
	notif->primary.r.type = SOF_IPC4_GLB_NOTIFICATION;
	notif->primary.r.rsp = SOF_IPC4_MESSAGE_DIR_MSG_REQUEST;
	notif->primary.r.msg_tgt = SOF_IPC4_MESSAGE_TARGET_FW_GEN_MSG;
}

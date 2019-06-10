/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ brief PTP data sets
 *
 * This is not to be included by the application.
 */

#ifndef __PTP_DS_H
#define __PTP_DS_H

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION_PTP       	2

#define PTP_PORT_START 1
#define PTP_PORT_END (ptp_domain.default_ds.nb_ports + PTP_PORT_START)

#define PTP_PORT_INDEX (port - PTP_PORT_START)

#define PTP_DEFAULT_DS()         (&ptp_domain.default_ds)
#define PTP_CURRENT_DS()         (&ptp_domain.current_ds)
#define PTP_PARENT_DS()          (&ptp_domain.parent_ds)
#define PTP_TIME_PROPERTIES_DS() (&ptp_domain.time_properties_ds)
//#define PTP_STATE() (&ptp_domain.state)

#define PTP_PORT_IFACE(port)      ptp_domain.iface[port - PTP_PORT_START]

//@TODO: Move to PTP types header. See IEEE 1588 5.3
typedef u8_t Octet;
typedef Octet[8] ClockIdentity;

enum {
	E2E=1,P2P=2,DELAY_DISABLED=0xFE
};

typedef struct {
	s32_t seconds;
	s32_t nanoseconds;
} TimeInternal;

// 5.3.2
typedef struct {
	s64_t scaled_ns;
} TimeInterval;

typedef struct {
	u8_t clock_class;
	u8_t clock_accuracy;
	u16_t offset_scaled_log_variance;
} ClockQuality;

typedef struct {
	ClockIdentity clock_identity[PTP_CLOCK_ID_LEN];
	u16_t port_number;
} PortIdentity;

/**
 * @brief Default Parameter Data Set.
 *
 * Data Set representing capabilities of the time-aware system.
 */
struct ptp_defaultDS {

	bool twoStepFlag;

	ClockIdentity clock_identity[PTP_CLOCK_ID_LEN];

	/** Number of ports of the time-aware system. */
	u8_t nb_ports;

	/** Quality of the local clock. */
	ClockQuality clock_quality;

	u8_t priority1;
	u8_t priority2;
	u8_t domain_number;
	bool slave_only;
};

struct ptp_currentDS {

	u16_t steps_removed;
	TimeInterval offset_from_master;
	TimeInterval mean_path_delay;
};

struct ptp_parentDS {

	PortIdentity parent_port_identity;
	bool parent_stats;
	u16_t observed_parent_offset_scaled_log_variance;
	s32_t observed_parent_clock_phase_change_rate;
	ClockIdentity grandmaster_identity;
	ClockQuality grandmaster_clock_quality;
	u8_t grandmaster_priority1;
	u8_t grandmaster_priority2;
};

struct ptp_timePropertiesDS {

	u16_t current_utc_offset;
	bool current_utc_ofset_valid;
	bool leap59;
	bool leap61;
	bool time_traceable;
	bool frequency_traceable;
	bool ptp_timescale;
	u8_t time_source;
};

struct ptp_portDS {

	PortIdentity port_identity;
	u8_t         port_state;
	s8_t         log_min_delay_req_interval;
	TimeInternal peer_mean_path_delay;
	s8_t         log_announce_interval;
	u8_t         announce_receipt_timeout;
	s8_t         log_sync_interval;
	u8_t         delay_mechanism;
	s8_t         log_min_pdelay_req_interval;
	u8_t         version_number;
};

struct ptp_domain {

	/* PTP datasets */
	struct ptp_defaultDS default_ds;
	struct ptp_currentDS current_ds;
	struct ptp_parentDS parent_ds;
	struct ptp_timePropertiesDS time_properties_ds;
	struct ptp_portDS port_ds[CONFIG_NET_PTP_NUM_PORTS];

	struct net_if *iface[CONFIG_NET_PTP_NUM_PORTS];
};

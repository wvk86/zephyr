/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ptp, CONFIG_NET_PTP_LOG_LEVEL);


#include "ptp_data_set.h"


/*
 * Use the given port to generate the clock identity
 * for the device.
 * The clock identity is unique for one time-aware system.
 */
static void ptp_compute_clock_identity(int port)
{
	struct net_if *iface = PTP_PORT_IFACE(port);
	struct ptp_defaultDS *default_ds;

	default_ds = PTP_DEFAULT_DS();

	if (iface) {
		default_ds->clock_identity[0] = net_if_get_link_addr(iface)->addr[0];
		default_ds->clock_identity[1] = net_if_get_link_addr(iface)->addr[1];
		default_ds->clock_identity[2] = net_if_get_link_addr(iface)->addr[2];
		/
		default_ds->clock_identity[3] = 0xFF;
		default_ds->clock_identity[4] = 0xFE;
		default_ds->clock_identity[5] = net_if_get_link_addr(iface)->addr[3];
		default_ds->clock_identity[6] = net_if_get_link_addr(iface)->addr[4];
		default_ds->clock_identity[7] = net_if_get_link_addr(iface)->addr[5];
	}
}

static void ptp_init_clock_ds(void)
{
	struct ptp_defaultDS        *default_ds;
	struct ptp_currentDS        *current_ds;
	struct ptp_parentDS         *parent_ds;
	struct ptp_timePropertiesDS *time_properties_ds;
	struct ptp_portDS           *port_ds;

	default_ds = PTP_DEFAULT_DS();
	current_ds = PTP_CURRENT_DS();
	parent_ds  = PTP_PARENT_DS();
	prop_ds    = PTP_PROPERTIES_DS();

	/* Initialize default data set. */

	/* Compute the clock identity from the first port MAC address. */
	ptp_compute_clock_identity(GPTP_PORT_START);

	default_ds->twoStepFlag = true;

	default_ds->clock_quality.clock_accuracy = 0xFE;
	default_ds->clock_quality.clock_class = 248;
	default_ds->clock_quality.offset_scaled_log_var = 0xFFFF;

	default_ds->priority1 = 128;
	default_ds->priority2 = 128;

	default_ds->domain_number = 0; //VK: 1 for zephyr maybe?

#if defined(CONFIG_NET_PTP_SLAVE_ONLY)
	default_ds->slave_only = true;
	default_ds->clock_quality.clock_class = 255;
#endif

	memcpy(port_ds->port_identity.clock_identity,
		default_ds->clock_identity, PTP_CLOCK_ID_LEN);

	port_ds->port_identity.port_number = 1;

	port_ds->log_min_delay_req_interval = 0;

	port_ds->peer_mean_path_delay.seconds = 0;
	port_ds->peer_mean_path_delay.nanoseconds = 0;

	port_ds->log_announce_interval = 0;
	port_ds->announce_receipt_timeout = 6;
	port_ds->log_sync_interval = 0;
	port_ds->delay_mechanism = P2P; // Zephyr only support P2P for now.
	port_ds->log_min_pdelay_req_interval = 1;
	port_ds->version_number = VERSION_PTP;


//------------GPTP codes below this line
	default_ds->gm_capable = IS_ENABLED(CONFIG_NET_GPTP_GM_CAPABLE);
	default_ds->clk_quality.clock_class = GPTP_CLASS_OTHER;
	default_ds->clk_quality.clock_accuracy =
		CONFIG_NET_GPTP_CLOCK_ACCURACY;
	default_ds->clk_quality.offset_scaled_log_var =
		GPTP_OFFSET_SCALED_LOG_VAR_UNKNOWN;

	if (default_ds->gm_capable) {
		default_ds->priority1 = GPTP_PRIORITY1_GM_CAPABLE;
	} else {
		default_ds->priority1 = GPTP_PRIORITY1_NON_GM_CAPABLE;
	}

	default_ds->priority2 = GPTP_PRIORITY2_DEFAULT;

	default_ds->cur_utc_offset = 37U; /* Current leap seconds TAI - UTC */
	default_ds->flags.all = 0;
	default_ds->flags.octets[1] = GPTP_FLAG_TIME_TRACEABLE;
	default_ds->time_source = GPTP_TS_INTERNAL_OSCILLATOR;

	/* Initialize current data set. */
	(void)memset(current_ds, 0, sizeof(struct gptp_current_ds));

	/* Initialize parent data set. */

	/* parent clock id is initialized to default_ds clock id. */
	memcpy(parent_ds->port_id.clk_id, default_ds->clk_id,
	       GPTP_CLOCK_ID_LEN);
	memcpy(parent_ds->gm_id, default_ds->clk_id, GPTP_CLOCK_ID_LEN);
	parent_ds->port_id.port_number = 0;

	/* TODO: Check correct value for below field. */
	parent_ds->cumulative_rate_ratio = 0;

	parent_ds->gm_clk_quality.clock_class =
		default_ds->clk_quality.clock_class;
	parent_ds->gm_clk_quality.clock_accuracy =
		default_ds->clk_quality.clock_accuracy;
	parent_ds->gm_clk_quality.offset_scaled_log_var =
		default_ds->clk_quality.offset_scaled_log_var;
	parent_ds->gm_priority1 = default_ds->priority1;
	parent_ds->gm_priority2 = default_ds->priority2;

	/* Initialize properties data set. */

	/* TODO: Get accurate values for below. From the GM. */
	prop_ds->cur_utc_offset = 37U; /* Current leap seconds TAI - UTC */
	prop_ds->cur_utc_offset_valid = false;
	prop_ds->leap59 = false;
	prop_ds->leap61 = false;
	prop_ds->time_traceable = false;
	prop_ds->freq_traceable = false;
	prop_ds->time_source = GPTP_TS_INTERNAL_OSCILLATOR;

	/* Set system values. */
	global_ds->sys_flags.all = default_ds->flags.all;
	global_ds->sys_current_utc_offset = default_ds->cur_utc_offset;
	global_ds->sys_time_source = default_ds->time_source;
	global_ds->clk_master_sync_itv =
		NSEC_PER_SEC * GPTP_POW2(CONFIG_NET_GPTP_INIT_LOG_SYNC_ITV);
}


/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_gptp_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/gptp.h>
#include <shell/shell.h>
#include <ptp_clock.h>

static struct gptp_phase_dis_cb phase_dis;

#if defined(CONFIG_NET_GPTP_VLAN)
/* User data for the interface callback */
struct ud {
	struct net_if *first;
	struct net_if *second;
	struct net_if *third;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (!ud->first) {
		ud->first = iface;
		return;
	}

	if (!ud->second) {
		ud->second = iface;
		return;
	}

	if (!ud->third) {
		ud->third = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface, const char *ipv6_addr,
		       const char *ipv4_addr, u16_t vlan_tag)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	ret = net_eth_vlan_enable(iface, vlan_tag);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)", vlan_tag, ret);
	}

	if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
		LOG_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p", ipv6_addr, iface);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
		LOG_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
		return -EINVAL;
	}

	LOG_DBG("Interface %p VLAN tag %d setup done.", iface, vlan_tag);

	return 0;
}

static int init_vlan(void)
{
	struct ud ud;
	int ret;

	(void)memset(&ud, 0, sizeof(ud));

	net_if_foreach(iface_cb, &ud);

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = setup_iface(ud.second,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(ud.third,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
#endif /* CONFIG_NET_GPTP_VLAN */

static void gptp_phase_dis_cb(u8_t *gm_identity,
			      u16_t *time_base,
			      struct gptp_scaled_ns *last_gm_ph_change,
			      double *last_gm_freq_change)
{
	char output[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
	static u8_t id[8];

	if (memcmp(id, gm_identity, sizeof(id))) {
		memcpy(id, gm_identity, sizeof(id));

		LOG_DBG("GM %s last phase %d.%lld",
			log_strdup(gptp_sprint_clock_id(gm_identity, output,
							sizeof(output))),
			last_gm_ph_change->high,
			last_gm_ph_change->low);
	}
}

static int init_app(void)
{
#if defined(CONFIG_NET_GPTP_VLAN)
	if (init_vlan() < 0) {
		LOG_ERR("Cannot setup VLAN");
	}
#endif

	gptp_register_phase_dis_cb(&phase_dis, gptp_phase_dis_cb);

	return 0;
}

static int cmd_get_ptp_clk(const struct shell *shell, size_t argc, char **argv)
{
	struct net_if *iface;
	struct net_ptp_time time = {
		.second = 0,
		.nanosecond = 0,
	};

	struct device *clk;

	if (argc != 2) {
		shell_help(shell);
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(atoi(argv[1]));
	if (!iface) {
		shell_error(shell, "Interface not found");
		return -ENODEV;
	}

	clk = net_eth_get_ptp_clock(iface);
	if (!clk) {
		shell_error(shell, "Interface no PTP support");
		return -ENODEV;
	}

	ptp_clock_get(clk, &time);

	printk("ptp time(second):        %llu\n", time.second);
	shell_print(shell, "ptp time (nanosecond):  %09u", time.nanosecond);

	return 0;
}
SHELL_CMD_REGISTER(get_ptp, NULL, "[interface index]", cmd_get_ptp_clk);

void main(void)
{
	init_app();
}

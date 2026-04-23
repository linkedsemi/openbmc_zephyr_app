/* SPDX-License-Identifier: Apache-2.0 */

#include "net_stack_cfg.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>

#include <errno.h>

LOG_MODULE_REGISTER(NET_STACK_CFG, LOG_LEVEL_INF);

int net_stack_cfg_do_once(void)
{
	const struct device *const eth1 = DEVICE_DT_GET(DT_NODELABEL(eth1));
	struct net_if *iface;
	int nerr;

	LOG_INF("net_stack_cfg: delay before bring-up");
	k_msleep(2000);

	if (!device_is_ready(eth1)) {
		LOG_ERR("eth1 not ready");
		return -ENODEV;
	}

	iface = net_if_lookup_by_dev(eth1);
	if (iface == NULL) {
		LOG_ERR("No net_if for eth1");
		return -ENODEV;
	}

	/* 打印网络接口信息，确认实际名称 */
	printf("[NET_STACK_CFG] ============================================\n");
	printf("[NET_STACK_CFG] Device tree label: eth1\n");
	printf("[NET_STACK_CFG] Device name: %s\n", eth1->name);
	printf("[NET_STACK_CFG] Interface index: %d\n", net_if_get_by_iface(iface));
	printf("[NET_STACK_CFG] Interface is up: %s\n", net_if_is_up(iface) ? "yes" : "no");
	printf("[NET_STACK_CFG] ============================================\n");

	if (!net_if_is_up(iface)) {
		nerr = net_if_up(iface);
		if (nerr < 0 && nerr != -EALREADY) {
			LOG_WRN("net_if_up(eth1): %d", nerr);
		}
	}

	nerr = net_config_init_by_iface(iface, "Initializing network (eth1)", NET_CONFIG_NEED_IPV4,
					(int32_t)(CONFIG_NET_CONFIG_INIT_TIMEOUT * (int32_t)MSEC_PER_SEC));
	if (nerr < 0) {
		LOG_ERR("net_config_init_by_iface failed: %d", nerr);
		return nerr;
	}

	printf("eth1 IPv4 configured (static from Kconfig)");
	return 0;
}

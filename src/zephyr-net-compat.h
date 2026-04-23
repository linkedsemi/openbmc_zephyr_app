/*
 * Compatibility header for Zephyr network functions
 *
 * When CONFIG_USERSPACE is disabled, the syscall wrappers are not available.
 * This header provides macros to map net_addr_pton/net_addr_ntop to their
 * implementation functions (z_impl_net_addr_pton/z_impl_net_addr_ntop).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_COMPAT_H
#define ZEPHYR_NET_COMPAT_H

/* Workaround for missing syscall wrappers when CONFIG_USERSPACE=n */
#ifndef CONFIG_USERSPACE
#define net_addr_pton(family, src, dst) z_impl_net_addr_pton(family, src, dst)
#define net_addr_ntop(family, src, dst, size) z_impl_net_addr_ntop(family, src, dst, size)
#endif

#endif /* ZEPHYR_NET_COMPAT_H */

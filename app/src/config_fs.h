/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief File system default config initialization.
 *
 * Creates default IPMI configuration files on first boot.
 *
 * @return 0 on success, negative errno on failure.
 */
int config_fs_init(void);

#ifdef __cplusplus
}
#endif

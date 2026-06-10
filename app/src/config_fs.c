/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Filesystem default config initialization.
 *
 * Creates default IPMI configuration files on first boot.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <ff.h>
#define FS_RET_OK FR_OK
#else
#define FS_RET_OK 0
#endif

// #define PATH_MAX        4096

// LOG_MODULE_DECLARE(TEST_BROKER, LOG_LEVEL_DBG);

/* -- Forward declarations -- */
static int lsdir_recursive(const char *path);
static int mkdir_dir(const char *path);
static int write_file(const char *file_path, const char *content);
static int read_file(const char *file_path);

/* -- IPMI configuration file paths -- */
#define CFG_DIR "/SD2:/usr/share/ipmi-providers"

/* -- Default IPMI configuration JSON content -- */

static const char *dev_id_path = CFG_DIR "/dev_id.json";

static const char devid_json[] =
    "{\n"
    "    \"id\": 42,\n"
    "    \"revision\": 1,\n"
    "    \"addn_dev_support\": 0,\n"
    "    \"manuf_id\": 0,\n"
    "    \"prod_id\": 0,\n"
    "    \"aux\": 52,\n"
    "    \"firmware_revision\": {\n"
    "        \"major\": 1,\n"
    "        \"minor\": 0\n"
    "    }\n"
    "}";

static const char *channel_config_path = CFG_DIR "/channel_config.json";

static const char channel_config_json[] =
    "{\n"
    "    \"0\": {\n"
    "        \"name\": \"IPMB\",\n"
    "        \"is_valid\": true,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"ipmb\",\n"
    "            \"protocol_type\": \"ipmb-1.0\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"1\": {\n"
    "        \"name\": \"eth0\",\n"
    "        \"is_valid\": true,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"lan-802.3\",\n"
    "            \"protocol_type\": \"ipmb-1.0\",\n"
    "            \"session_supported\": \"multi-session\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"2\": {\n"
    "        \"name\": \"eth1\",\n"
    "        \"is_valid\": true,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"lan-802.3\",\n"
    "            \"protocol_type\": \"ipmb-1.0\",\n"
    "            \"session_supported\": \"multi-session\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"3\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"4\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"5\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"6\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"7\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"8\": {\n"
    "        \"name\": \"INTRABMC\",\n"
    "        \"is_valid\": true,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"oem\",\n"
    "            \"protocol_type\": \"oem\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"9\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"10\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"11\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"12\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"13\": {\n"
    "        \"name\": \"RESERVED\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"reserved\",\n"
    "            \"protocol_type\": \"na\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"14\": {\n"
    "        \"name\": \"SELF\",\n"
    "        \"is_valid\": false,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"ipmb\",\n"
    "            \"protocol_type\": \"ipmb-1.0\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    },\n"
    "    \"15\": {\n"
    "        \"name\": \"kcs0\",\n"
    "        \"is_valid\": true,\n"
    "        \"active_sessions\": 0,\n"
    "        \"channel_info\": {\n"
    "            \"medium_type\": \"system-interface\",\n"
    "            \"protocol_type\": \"kcs\",\n"
    "            \"session_supported\": \"session-less\",\n"
    "            \"is_ipmi\": true\n"
    "        }\n"
    "    }\n"
    "}";

static const char *channel_access_path = CFG_DIR "/channel_access.json";

static const char channel_access_json[] =
    "{\n"
    "    \"1\": {\n"
    "        \"access_mode\": \"always_available\",\n"
    "        \"user_auth_disabled\": false,\n"
    "        \"per_msg_auth_disabled\": false,\n"
    "        \"alerting_disabled\": false,\n"
    "        \"priv_limit\": \"priv-admin\"\n"
    "    },\n"
    "    \"2\": {\n"
    "        \"access_mode\": \"always_available\",\n"
    "        \"user_auth_disabled\": false,\n"
    "        \"per_msg_auth_disabled\": false,\n"
    "        \"alerting_disabled\": false,\n"
    "        \"priv_limit\": \"priv-admin\"\n"
    "    }\n"
    "}";

static const char *cipher_list_path = CFG_DIR "/cipher_list.json";

static const char cipher_list_json[] =
    "{\n"
    "    \"b\": {\n"
    "        \"cipher\": 17,\n"
    "        \"authentication\": 3,\n"
    "        \"integrity\": 4,\n"
    "        \"confidentiality\": 1\n"
    "    }\n"
    "}";

static const char *cs_privilege_levels_path = CFG_DIR "/cs_privilege_levels.json";

static const char cs_privilege_levels_json[] =
"{\n"
"    \"Channel0\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel1\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel10\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel11\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel12\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel13\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel14\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel15\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel2\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel3\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel4\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel5\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel6\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel7\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel8\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    },\n"
"    \"Channel9\": {\n"
"        \"CipherID0\": \"priv-admin\",\n"
"        \"CipherID1\": \"priv-admin\",\n"
"        \"CipherID10\": \"priv-admin\",\n"
"        \"CipherID11\": \"priv-admin\",\n"
"        \"CipherID12\": \"priv-admin\",\n"
"        \"CipherID13\": \"priv-admin\",\n"
"        \"CipherID14\": \"priv-admin\",\n"
"        \"CipherID15\": \"priv-admin\",\n"
"        \"CipherID2\": \"priv-admin\",\n"
"        \"CipherID3\": \"priv-admin\",\n"
"        \"CipherID4\": \"priv-admin\",\n"
"        \"CipherID5\": \"priv-admin\",\n"
"        \"CipherID6\": \"priv-admin\",\n"
"        \"CipherID7\": \"priv-admin\",\n"
"        \"CipherID8\": \"priv-admin\",\n"
"        \"CipherID9\": \"priv-admin\"\n"
"    }\n"
"}";

/* -- Utility functions -- */

int fs_rm_rf(const char *path)
{
    struct fs_dir_t dir;
    struct fs_dirent entry;
    char full_path[1024];
    int ret = 0;

    fs_dir_t_init(&dir);

    ret = fs_opendir(&dir, path);
    if (ret != 0) {
        printk("opendir failed: %s, ret=%d\n", path, ret);
        return ret;
    }

    while (true) {
        ret = fs_readdir(&dir, &entry);
        if (ret != 0 || entry.name[0] == '\0') {
            break;
        }

        if (!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry.name);

        if (entry.type == FS_DIR_ENTRY_DIR) {
            fs_closedir(&dir);

            ret = fs_rm_rf(full_path);
            if (ret != 0) {
                return ret;
            }

            fs_dir_t_init(&dir);
            ret = fs_opendir(&dir, path);
            if (ret != 0) {
                printk("reopen failed: %s %d\n", path, ret);
                return ret;
            }
        } else {
            ret = fs_unlink(full_path);
            if (ret == 0) {
                printk("deleted: %s\n", full_path);
            }
        }
    }

    fs_closedir(&dir);

    ret = fs_unlink(path);
    if (ret == 0) {
        printk("deleted dir: %s\n", path);
    }

    return ret;
}

static int lsdir_recursive(const char *path)
{
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;
    char full_path[PATH_MAX];

    if (strlen(path) >= PATH_MAX) {
        printk("path too long\n");
        return -ENAMETOOLONG;
    }

    fs_dir_t_init(&dirp);

    res = fs_opendir(&dirp, path);
    if (res) {
        printk("Error opening dir %s [%d]\n", path, res);
        return res;
    }

    printk("Listing dir %s ...\n", path);

    while (1) {
        res = fs_readdir(&dirp, &entry);
        if (res != 0 || entry.name[0] == 0) {
            break;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry.name);

        if (entry.type == FS_DIR_ENTRY_DIR) {
            printk("[DIR ] %s\n", full_path);

            /* Skip . and .. to prevent infinite recursion */
            if (strcmp(entry.name, ".") == 0 ||
                strcmp(entry.name, "..") == 0) {
                continue;
            }

            lsdir_recursive(full_path);
        } else {
            printk("[FILE] %s (size = %zu)\n", full_path, entry.size);
        }
    }

    fs_closedir(&dirp);
    return 0;
}

static int mkdir_dir(const char *path)
{
    char tmp[PATH_MAX];
    char *p;
    int ret;
    size_t start_idx = 1;  // skip first '/'

    if (path == NULL || *path == '\0') {
        return -EINVAL;
    }

    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* Skip the leading mount prefix (e.g. /SD2:) to avoid creating
     * an empty directory entry.
     */
    if (tmp[0] == '/') {
        char *colon = strchr(tmp + 1, ':');
        if (colon != NULL && *(colon + 1) == '/') {
            /* Skip the "/xxx:/" prefix, pointing to the first valid directory character */
            start_idx = (colon - tmp) + 2;  // Position after ':' and '/'
        }
    }

    /* If the starting index is beyond the string length, the path is just the mount point */
    if (start_idx >= strlen(tmp)) {
        return 0;
    }

    /* Create directories level by level starting from the mount point */
    for (p = tmp + start_idx; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            ret = fs_mkdir(tmp);
            if (ret < 0 && ret != -EEXIST) {
                printk("mkdir failed: %s, %d\n", tmp, ret);
                return ret;
            }
            *p = '/';
        }
    }

    /* Create the last path component */
    ret = fs_mkdir(tmp);
    if (ret < 0 && ret != -EEXIST) {
        printk("mkdir %s failed: %d\n", tmp, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief Check if a file already exists.
 *
 * @param file_path  Path to check.
 * @return true if the file exists, false otherwise.
 */
__unused static bool file_exists(const char *file_path)
{
    struct fs_file_t file;

    if (fs_open(&file, file_path, FS_O_READ) == 0) {
        fs_close(&file);
        return true;
    }

    return false;
}

static int write_file(const char *file_path, const char *content)
{
    struct fs_file_t file;
    int ret;

    if (strlen(file_path) >= PATH_MAX) {
        printk("Path too long: %s\n", file_path);
        return -ENAMETOOLONG;
    }

    if (content == NULL) {
        printk("content is NULL\n");
        return -EINVAL;
    }
    /* Ensure parent directory exists */
    char dir_path[PATH_MAX];
    strncpy(dir_path, file_path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        ret = mkdir_dir(dir_path);
        if (ret < 0) {
            return ret;
        }
    }

    fs_file_t_init(&file);
    ret = fs_open(&file, file_path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
    if (ret < 0) {
        printk("Failed to open %s for writing: %d\n", file_path, ret);
        return ret;
    }

    ret = fs_write(&file, content, strlen(content));
    if (ret < 0) {
        printk("Failed to write %s: %d\n", file_path, ret);
    }

    fs_close(&file);
    return (ret < 0) ? ret : 0;
}

static int read_file(const char *file_path)
{
    struct fs_file_t file;
    char buf[1024];
    int ret, rd;
    int total = 0;

    fs_file_t_init(&file);
    ret = fs_open(&file, file_path, FS_O_READ);
    if (ret < 0) {
        printk("Cannot open %s: %d\n", file_path, ret);
        return ret;
    }

    printk("Reading file: %s\n", file_path);
    while ((rd = fs_read(&file, buf, sizeof(buf) - 1)) > 0) {
        buf[rd] = '\0';
        printk("%s", buf);
        total += rd;
    }
    printk("\n");

    printk("--- end of %s (%d bytes) ---\n", file_path, total);
    fs_close(&file);
    return 0;
}

/**
 * @brief Write a config file.
 *
 * @param path     Path to the config file.
 * @param content  JSON content to write.
 * @return 0 on success, negative errno on error.
 */
static int write_config(const char *path, const char *content)
{
    // if (file_exists(path)) {
    //     printk("Config already exists, skipping: %s\n", path);
    //     return 0;
    // }

    printk("Creating default config: %s\n", path);
    int ret = write_file(path, content);
    if (ret == 0) {
        read_file(path);
    }
    return ret;
}

/* -- Main entry point -- */

int config_fs_init(void)
{
    int ret;

    printk("Initializing filesystem config...\n");

    /* List current filesystem contents */
    lsdir_recursive("/SD2:");
    // fs_rm_rf("/SD2:/var");
    // fs_rm_rf("/SD2:/run");
    // fs_rm_rf("/SD2:/usr");
    // lsdir_recursive("/SD2:");

    /* Write default IPMI configuration files (idempotent) */
    ret = write_config(dev_id_path, devid_json);
    if (ret < 0) {
        return ret;
    }

    ret = write_config(channel_config_path, channel_config_json);
    if (ret < 0) {
        return ret;
    }

    ret = write_config(channel_access_path, channel_access_json);
    if (ret < 0) {
        return ret;
    }

    ret = write_config(cipher_list_path, cipher_list_json);
    if (ret < 0) {
        return ret;
    }

    ret = write_config(cs_privilege_levels_path, cs_privilege_levels_json);
    if (ret < 0) {
        return ret;
    }

    /* Create runtime directories */
    // phosphor-host-ipmid
    mkdir_dir("/SD2:/var/lib/ipmi");
    mkdir_dir("/SD2:/run/ipmi");
    mkdir_dir("/SD2:/usr/share/ipmi-providers");

    //phosphor-logging
    mkdir_dir("/SD2:/var/lib/phosphor-logging/extensions");
    mkdir_dir("/SD2:/var/lib/phosphor-logging/errors");

    printk("Filesystem config initialization complete.\n");
    return 0;
}

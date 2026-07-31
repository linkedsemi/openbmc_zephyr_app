/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <exception>
#include <unwind.h>

/* dbus-broker headers */
// #include "broker/broker.h"

/* Deployment implementation */
// #include "test_deployment.h"

/* D-Bus broker subsystem */
#include "dbus_broker.h"

/* SD-Event test suite */
#include "test_sd_event.h"

/* Filesystem default config initialization */
#include "config_fs.h"

LOG_MODULE_REGISTER(TEST_BROKER, LOG_LEVEL_DBG);

/*
 * Test Thread - Tests the deployed broker
 */
// static void test_thread_entry(void *p1, void *p2, void *p3)
// {
//     ARG_UNUSED(p1);
//     ARG_UNUSED(p2);
//     ARG_UNUSED(p3);
//     int r;
//     int retry_count = 0;
//     const int max_retries = 100; /* Wait up to 10 seconds */

//     LOG_INF("[Test Thread] Starting - waiting for broker initialization...");

//     /* Wait for broker to be ready */
//     while ((g_broker == NULL || g_controller_fds[0] < 0 || !deploy_state.broker_ready)
//             && retry_count < max_retries) {
//         // LOG_DBG("[Test Thread] Waiting... retry %d, g_broker: %p, fd: %d",
//         //         retry_count, g_broker, g_controller_fds[0]);
//         k_msleep(100);
//         retry_count++;
//     }

//     if (g_broker == NULL) {
//         LOG_ERR("[Test Thread] Broker failed to initialize within timeout");
//         return;
//     }

//     if (g_controller_fds[0] < 0) {
//         LOG_ERR("[Test Thread] Controller FDs not properly initialized");
//         return;
//     }

//     LOG_INF("[Test Thread] Broker is ready! g_broker=%p", g_broker);
//     LOG_INF("[Test Thread] Controller FDs: %d, %d", g_controller_fds[0], g_controller_fds[1]);

//     /* Run deployment verification tests */
//     r = test_broker_deployment();

//     if (r == 0) {
//         LOG_INF("[Test Thread] Deployment verification passed!");
//     } else {
//         LOG_WRN("[Test Thread] Deployment verification failed: %d", r);
//     }

//     /* Wait a bit more for listener to be fully ready */
//     k_msleep(500);

//     /* Wait for service provider to be ready */
//     LOG_INF("[Test Thread] Waiting for service provider to be ready...");
//     retry_count = 0;
//     while (!deploy_state.service_provider_ready && retry_count < 100) {
//         k_msleep(100);
//         retry_count++;
//     }
//     if (deploy_state.service_provider_ready) {
//         LOG_INF("[Test Thread] Service provider is ready!");
//     } else {
//         LOG_WRN("[Test Thread] Service provider not ready after timeout");
//     }

//     /* Start client threads to test broker functionality */
//     LOG_INF("[Test Thread] Starting client threads...");
//     r = start_client_threads();
//     if (r < 0) {
//         LOG_ERR("[Test Thread] Failed to start client threads: %d", r);
//     } else {
//         LOG_INF("[Test Thread] Client threads started successfully");
//     }
// }

/* Thread stacks with appropriate priorities */
// K_THREAD_DEFINE(test_thread, 4096, test_thread_entry, NULL, NULL, NULL, 6, 0, 0);

/* SD-Event test thread will be defined in test_sd_event.c */


#include "task_def.hpp"
#include "task_enable.hpp"
#include "thread_dependency_mgr.hpp"
#include <printk_thread.h>
#include "common_io.hpp"

#include <ff.h>
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <zephyr/fs/littlefs.h>
#endif

thread_local boost::asio::io_context io;

// Define threads and their dependencies
#ifdef ENABLE_DBUS_BROKER
THREAD_DEFINE(dbus_broker, dbus_broker_init, dbus_broker_ready_sem);
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_OBJMGR
THREAD_DEFINE(objmgr, objmgr_init, objmgr_ready_sem, "dbus_broker");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_LOGGING
THREAD_DEFINE(logging, logging_init, logging_ready_sem, "dbus_broker");
#endif

#ifdef ENABLE_PHOSPHOR_USER_MANAGER
THREAD_DEFINE(user_manager, user_manager_init, user_manager_ready_sem, "zbus_broker", "objmgr");
#endif

// 网络栈配置线程 - 必须在 net-ipmid 之前初始化
#ifdef ENABLE_OPENBMC_PHOSPHOR_NET_IPMID
THREAD_DEFINE(net_stack_cfg, net_stack_cfg_init, net_stack_cfg_ready_sem);
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_HOST_IPMID
// THREAD_DEFINE(ipmid, ipmid_init, host_ipmid_ready_sem, "dbus_broker", "objmgr", "network_manager");
THREAD_DEFINE(ipmid, ipmid_init, host_ipmid_ready_sem, "dbus_broker", "net_stack_cfg");
#endif

#ifdef ENABLE_OPENBMC_IPMITOOL
THREAD_DEFINE(ipmitool, ipmitool_init, ipmitool_ready_sem, "zbus_broker", "ipmid");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_LED_SYSFS
THREAD_DEFINE(led_sysfs, led_sysfs_init, led_sysfs_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_LED_MANAGER
THREAD_DEFINE(led_manager, led_manager_init, led_manager_ready_sem, "zbus_broker", "led_sysfs");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_SETTINGS_MANAGER
THREAD_DEFINE(setting_manager, setting_manager_init, setting_manager_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_IPMBBRIDGE
THREAD_DEFINE(ipmbbridge, ipmbbridge_init, ipmbbridge_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_OBMC_CONSOLE
THREAD_DEFINE(obmc_console, obmc_console_init, obmc_console_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_SYSTEMD_NETWORKD
THREAD_DEFINE(systemd_networkd, systemd_networkd_init, systemd_networkd_ready_sem, "zbus_broker", "objmgr");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_NETWORK
THREAD_DEFINE(network_manager, network_manager_init, network_manager_ready_sem, "zbus_broker", "objmgr", "systemd_networkd");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_NET_IPMID
// net-ipmid 依赖于 net_stack_cfg，确保网络已配置
THREAD_DEFINE(net_ipmid, net_ipmid_init, net_ipmid_ready_sem, "dbus_broker", "net_stack_cfg");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_WATCHDOG
THREAD_DEFINE(watchdog, watchdog_init, watchdog_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_TIME_MANAGER
THREAD_DEFINE(timedated, timedated_init, timedated_ready_sem, "zbus_broker");

THREAD_DEFINE(time_manager, time_manager_init, time_manager_ready_sem, "zbus_broker", "objmgr", "timedated", "setting_manager");
#endif

#ifdef ENABLE_OPENBMC_BIOS_SETTINGS_MGR
THREAD_DEFINE(bios_settings_mgr, bios_settings_mgr_init, bios_settings_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_BMCWEB
THREAD_DEFINE(bmcweb, bmcweb_init, bmcweb_ready_sem, "zbus_broker", "user_manager");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_BMC_CODE_MGT
THREAD_DEFINE(software_updater, software_updater_init, software_updater_ready_sem, "zbus_broker", "objmgr");
THREAD_DEFINE(image_manager, image_manager_init, image_manager_ready_sem, "zbus_broker", "objmgr");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_SEL_LOGGER
THREAD_DEFINE(sel_logger, sel_logger_init, sel_logger_ready_sem, "zbus_broker", "logging");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_BMC_STATE_MANAGER
THREAD_DEFINE(bmc_state, bmc_state_init, bmc_state_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_DEBUG_COLLECTOR
THREAD_DEFINE(dump_manager, dump_manager_init, dump_manager_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_FRU_DEVICE
THREAD_DEFINE(fru_device, fru_device_init, fru_device_ready_sem, "dbus_broker");
#endif

#ifdef ENABLE_OPENBMC_ENTITY_MANAGER
THREAD_DEFINE(entity_manager, entity_manager_init, entity_manager_ready_sem, "dbus_broker", "objmgr", "fru_device");
#endif

/*begin of dbus sensor related thread*/
#ifdef ENABLE_OPENBMC_DBUS_SENSORS_ADC
THREAD_DEFINE(adc_sensor, adc_sensor_init, adc_sensor_ready_sem, "dbus_broker", "objmgr", "entity_manager");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_EXTERNAL
THREAD_DEFINE(external_sensor, external_sensor_init, external_sensor_ready_sem, "dbus_broker", "objmgr", "entity_manager");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_FAN
THREAD_DEFINE(fan_sensor, fan_sensor_init, fan_sensor_ready_sem, "zbus_broker", "objmgr");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_HWMON_TEMP
THREAD_DEFINE(hwmon_temp_sensor, hwmon_temp_sensor_init, hwmon_temp_sensor_ready_sem, "dbus_broker", "objmgr", "entity_manager");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_INTELCPU
THREAD_DEFINE(intel_cpu_sensor, intel_cpu_sensor_init, intel_cpu_sensor_ready_sem, "zbus_broker", "objmgr");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_INTRUSION
THREAD_DEFINE(intrusion_sensor, intrusion_sensor_init, intrusion_sensor_ready_sem, "dbus_broker", "objmgr", "entity_manager");
#endif

#ifdef ENABLE_OPENBMC_DBUS_SENSORS_PSU
THREAD_DEFINE(psu_sensor, psu_sensor_init, psu_sensor_ready_sem, "dbus_broker", "objmgr", "entity_manager");
#endif
/* end of dbus sensor related thread*/

#ifdef ENABLE_OPENBMC_CALLBACK_MANAGER
THREAD_DEFINE(callback_manager, callback_manager_init, callback_manager_ready_sem, "zbus_broker", "objmgr", "led_manager", "sel_logger");
#endif

#ifdef ENABLE_OPENBMC_KCSBRIDGE
THREAD_DEFINE(kcs_bridge, kcs_bridge_init, kcs_bridge_ready_sem, "dbus_broker", "ipmid");
#endif

#ifdef ENABLE_OPENBMC_CERTIFICATE_MANAGER
THREAD_DEFINE(certificate_manager, certificate_manager_init, certificate_manager_ready_sem, "zbus_broker");
#endif

#ifdef ENABLE_OPENBMC_PHOSPHOR_HOSTLOGGER
THREAD_DEFINE(phosphor_hostlogger, phosphor_hostlogger_init, phosphor_hostlogger_ready_sem, "zbus_broker","obmc_console");
#endif

#ifdef ENABLE_BUSCTL
THREAD_DEFINE(busctl, busctl_init, busctl_ready_sem, "zbus_broker");
#endif

/*** begin of test thread ***/
// THREAD_DEFINE_NO_SEM(i2ctest, i2ctest_init);
// THREAD_DEFINE_NO_SEM(kcstest, kcstest_init);
// #ifdef CONFIG_UART_TEST
// THREAD_DEFINE_NO_SEM(uarttest, uarttest_init);
// #endif
// THREAD_DEFINE_NO_SEM(hwmontest, hwmontest_init);

// #ifdef CONFIG_EEPROM
// THREAD_DEFINE_NO_SEM(eepromtest, eepromtest_init);
// #endif
/*** end of test thread ***/


extern "C" {
#include <overlay_fs.h>
// #include "zephyr/drivers/misc/linkedsemi/mbox_linkedsemi.h"
}

// #ifdef CONFIG_ZEPHYRBMC_FILESYSTEM
static struct fs_mount_t *mp_ro_lfs;
static struct fs_mount_t *mp_rw_lfs;

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t sd2mp = {
	.type = FS_FATFS,
    .mnt_point = "/SD2:",
	.fs_data = &fat_fs,
};


// FileSystemParams fs_params;
// extern "C"
// {
// extern uint8_t flash_ls_client_read_ear(const struct device* dev);
// }

// static FATFS mnt_fat_fs;
// static struct fs_mount_t mnt_mp = {
//     .type = FS_FATFS,
//     .mnt_point = "/mnt",
//     .fs_data = &mnt_fat_fs,
// };

// static FATFS run_fat_fs;
// static struct fs_mount_t run_mp = {
//     .type = FS_FATFS,
//     .mnt_point = "/run",
//     .fs_data = &run_fat_fs,
// };

void fs_params_init()
{
#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#define PARTITION_NODE_RO_A DT_NODELABEL(ro_lfs_a)
// #define PARTITION_NODE_RO_B DT_NODELABEL(ro_lfs_b)
    // const struct device* const flash_dev = DEVICE_DT_GET(DT_NODELABEL(qspi1));

    // uint8_t ear = flash_ls_client_read_ear(flash_dev);
    // if (ear == 0x0)
    // {
        // LOG_INF("Selecting RO_A partition (EAR=0x%02X)", ear);
        FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE_RO_A);
        mp_ro_lfs = &FS_FSTAB_ENTRY(PARTITION_NODE_RO_A);
    // }
    // else
    // {
    //     LOG_INF("Selecting RO_B partition (EAR=0x%02X)", ear);
    //     FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE_RO_B);
    //     fs_params.mp_ro_lfs = &FS_FSTAB_ENTRY(PARTITION_NODE_RO_B);
    // }

#define PARTITION_NODE_RW DT_NODELABEL(rw_lfs)
    FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE_RW);
    mp_rw_lfs = &FS_FSTAB_ENTRY(PARTITION_NODE_RW);
#endif

}

static struct overlay_mount_data overlay_data;
struct fs_mount_t mp_overlay = {
    .type = FS_OVERLAYFS,
    .mnt_point = CONFIG_FS_ROOT_OVERLAY,
    .fs_data = &overlay_data,
};

int overlayfs_init()
{
    int ret = 0;
    struct overlay_mount_data* fs_data =  (struct overlay_mount_data*)mp_overlay.fs_data;
    fs_data->ro_mnt = mp_ro_lfs;
    fs_data->rw_mnt = mp_rw_lfs;

    ret = fs_mount(&mp_overlay);
    if (ret) {
        while(1);
        printf("Overlay FS mount failed, exit.\n");
    }

    return ret;
}

/*** end of test thread ***/
static int filesystem_init()
{
    int ret = 0;

// #ifdef CONFIG_ZEPHYRBMC_FILESYSTEM
    fs_params_init();
    // ret = fs_mount(&mnt_mp);
    // if (ret) {
    //     while(1);
    // }
    // ret = fs_mount(&run_mp);
    // if (ret) {
    //     while(1);
    // }
    // ret = storage_fs_init();
#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
    ret = fs_mount(mp_ro_lfs);
    if (ret) {
        while(1);
        goto err;
    }
    ret = fs_mount(mp_rw_lfs);
    if (ret) {
        while(1);
        goto err;
    }
#endif
    ret = fs_mount(&sd2mp);
    if (ret) {
        while(1);
    }
    ret = overlayfs_init();
    if (ret) {
        while(1);
        goto err;
    }
// #endif

err:
    return ret;
}

static int filesystem_init_rslt = filesystem_init();
// #endif

/*
 * Main entry point
 */
int main(void)
{
    // LOG_INF("Thread deployment:");
    // LOG_INF("  - Broker thread (priority 7): Running standard broker_run()");
    // LOG_INF("  - Service thread (priority 6): Name service provider");
    // LOG_INF("  - Test thread (priority 6): Tests broker functionality for clients");
    // LOG_INF("  - SD-Event test thread (priority 5): Runs sd-event test suite concurrently");
    // LOG_INF("");
    // LOG_INF("Starting concurrent stress testing...");
    // LOG_INF("========================================");


        // z_switch_hook_register(task_switch_hook);

    // printk_thread("in...");

// #ifndef CONFIG_BOARD_QEMU_RISCV32_QEMU_VIRT_RISCV32
//     irq_enable(DT_IRQN(DT_NODELABEL(mbox)));
// #endif

    // k_sem_take(&net_config_init_ready_sem, K_SECONDS(20));
    // k_sleep(K_SECONDS(5));

    //only use once at the first time to sue filesysem, or you need to add new jsonfile or path.
    // config_fs_init();

    LOG_INF("Starting BMC application with dependency management...\n");

    // TODO: Ensure that the ip is not modified during the thread initialization startup stage

    // Print thread status
    print_thread_status();

    // Start all automatically started threads (in dependency order)
    start_all_auto_threads();

    // Print the final status
    print_thread_status();

    LOG_INF("BMC application startup completed\n");
    // extern int objmgr_test_init(void);
    // objmgr_test_init();

    extern int ipmid_test(void);

    ipmid_test();
    /* Keep main thread alive so worker threads can run */
    while (1) {
        k_msleep(20000); /* Check every 2 seconds */
    }

    return 0;
}

#pragma once

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_QEMU_TARGET
#define THREAD_STACK_SECTION
#else
// #define THREAD_STACK_SECTION(name) __attribute__((section(".noinit" "." #name "." "stack")))
#define THREAD_STACK_SECTION(name) __attribute__((section(CONFIG_APP_STACK_SECTION "." #name "." "stack")))
#endif

int create_task_with_pthread(pthread_t *thread, const char *name,
                             void *stack, size_t stack_size,
                             void* (*routine)(void *), bool join_flag);

#define CREATE_TASK_WITH_PTHREAD(thread, name, stack_size) \
    static Z_KERNEL_STACK_DEFINE_IN(name##_stack, stack_size, THREAD_STACK_SECTION(name)); \
    create_task_with_pthread(thread, #name, name##_stack, stack_size, name##_handler, false);

#define CREATE_TASK_WITH_PTHREAD_JOIN(thread, name, stack_size) \
    static Z_KERNEL_STACK_DEFINE_IN(name##_stack, stack_size, THREAD_STACK_SECTION(name)); \
    create_task_with_pthread(thread, #name, name##_stack, stack_size, name##_handler, true);

//net config init
// extern struct k_sem net_config_init_ready_sem;

// zbus broker
#define CONFIG_THREAD_PRI_DBUS_BROKER 10
#define DBUS_BROKER_THREAD_STACK_SIZE (1 * 1024)
int dbus_broker_init();
extern struct k_sem dbus_broker_ready_sem;

// phosphor-host-ipmid
#define CONFIG_THREAD_PRI_IPMI 10
#define IPMI_THREAD_STACK_SIZE (32 * 1024)
int ipmid_init();
extern struct k_sem host_ipmid_ready_sem;

// busctl
#define CONFIG_THREAD_PRI_BUSCTL 10
#define BUSCTL_THREAD_STACK_SIZE (32 * 1024)
int busctl_init();
extern struct k_sem busctl_ready_sem;

// phosphor-watchdog
#define CONFIG_THREAD_PRI_WATCHDOG 10
#define WATCHDOG_THREAD_STACK_SIZE (32 * 1024)
int watchdog_init();
extern struct k_sem watchdog_ready_sem;

// phosphor-led-sysfs
#define CONFIG_THREAD_PRI_LED_SYSFS 10
#define LED_SYSFS_THREAD_STACK_SIZE (32 * 1024)
int led_sysfs_init();
extern struct k_sem led_sysfs_ready_sem;

// phosphor-led-manager
#define CONFIG_THREAD_PRI_LED_MANAGER 10
#define LED_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int led_manager_init();
extern struct k_sem led_manager_ready_sem;

// phosphor-settings
#define CONFIG_THREAD_PRI_SETTING_MANAGER 10
#define SETTING_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int setting_manager_init();
extern struct k_sem setting_manager_ready_sem;

// phosphor-network
#define CONFIG_THREAD_PRI_NETWORK_MANAGER 10
#define NETWORK_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int network_manager_init();
extern struct k_sem network_manager_ready_sem;

//ipmbbridge
#define CONFIG_THREAD_PRI_IPMBBRIDGE 10
#define IPMBBRIDGE_THREAD_STACK_SIZE (32 * 1024)
int ipmbbridge_init();
extern struct k_sem ipmbbridge_ready_sem;

//obmc_console
#define CONFIG_THREAD_PRI_OBMC_CONSOLE 10
#define OBMC_CONSOLE_THREAD_STACK_SIZE (32 * 1024)
int obmc_console_init();
extern struct k_sem obmc_console_ready_sem;

//timedated
#define CONFIG_THREAD_PRI_TIME_DATED 10
#define TIME_DATED_THREAD_STACK_SIZE (32 * 1024)
int timedated_init();
extern struct k_sem timedated_ready_sem;

//time-manager
#define CONFIG_THREAD_PRI_TIME_MANAGER 10
#define TIME_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int time_manager_init();
extern struct k_sem time_manager_ready_sem;

//bios-setting-manager
#define CONFIG_THREAD_PRI_BIOS_SETTINGS_MGR 10
#define BIOS_SETTINGS_MGR_THREAD_STACK_SIZE (32 * 1024)
int bios_settings_mgr_init();
extern struct k_sem bios_settings_ready_sem;

//ipmitool
#define CONFIG_THREAD_PRI_IPMITOOL 10
#define IPMITOOL_THREAD_STACK_SIZE (32 * 1024)
int ipmitool_init();
extern struct k_sem ipmitool_ready_sem;

//user_manager
#define CONFIG_THREAD_PRI_USER_MANAGER 10
#define USER_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int user_manager_init();
extern struct k_sem user_manager_ready_sem;

// net_stack_cfg - 网络栈配置（必须在 net-ipmid 之前初始化）
#define CONFIG_THREAD_PRI_NET_STACK_CFG 5
#define NET_STACK_CFG_THREAD_STACK_SIZE (16 * 1024)
int net_stack_cfg_init();
extern struct k_sem net_stack_cfg_ready_sem;

// phosphor-net-ipmid
#define CONFIG_THREAD_PRI_NET_IPMID 10
#define NET_IPMID_THREAD_STACK_SIZE (64 * 1024)
int net_ipmid_init();
extern struct k_sem net_ipmid_ready_sem;

//bmcweb
#define CONFIG_THREAD_PRI_BMCWEB 10
#define BMCWEB_THREAD_STACK_SIZE (32 * 1024)
int bmcweb_init();
extern struct k_sem bmcweb_ready_sem;

// phosphor-objmgr
#define CONFIG_THREAD_PRI_OBJMGR 10
#define OBJMGR_THREAD_STACK_SIZE (32 * 1024)
int objmgr_init();
extern struct k_sem objmgr_ready_sem;

// phosphor-bmc-code-mgt
#define CONFIG_THREAD_PRI_UPDATER 10
#define UPDATER_THREAD_STACK_SIZE (32 * 1024)
int software_updater_init();
extern struct k_sem software_updater_ready_sem;

#define CONFIG_THREAD_PRI_IMAGE 10
#define IMAGE_THREAD_STACK_SIZE (20 * 1024)
int image_manager_init();
extern struct k_sem image_manager_ready_sem;

//adcsensors
#define CONFIG_THREAD_PRI_ADC_SENSOR 10
#define ADC_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int adc_sensor_init();
extern struct k_sem adc_sensor_ready_sem;

// phosphor-logging
#define CONFIG_THREAD_PRI_LOGGING 10
#define PHOSPHOR_LOGGING_THREAD_STACK_SIZE (32 * 1024)
int logging_init();
extern struct k_sem logging_ready_sem;

#define CONFIG_THREAD_PRI_EXTERNAL_SENSOR 10
#define EXTERNAL_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int external_sensor_init();
extern struct k_sem external_sensor_ready_sem;

#define CONFIG_THREAD_PRI_FAN_SENSOR 10
#define FAN_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int fan_sensor_init();
extern struct k_sem fan_sensor_ready_sem;

#define CONFIG_THREAD_PRI_HWMON_TEMP_SENSOR 10
#define HWMON_TEMP_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int hwmon_temp_sensor_init();
extern struct k_sem hwmon_temp_sensor_ready_sem;

#define CONFIG_THREAD_PRI_INTEL_CPU_SENSOR 10
#define INTEL_CPU_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int intel_cpu_sensor_init();
extern struct k_sem intel_cpu_sensor_ready_sem;

#define CONFIG_THREAD_PRI_INTRUSION_SENSOR 10
#define INTRUSION_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int intrusion_sensor_init();
extern struct k_sem intrusion_sensor_ready_sem;

#define CONFIG_THREAD_PRI_PSU_SENSOR 10
#define PSU_SENSOR_THREAD_STACK_SIZE (32 * 1024)
int psu_sensor_init();
extern struct k_sem psu_sensor_ready_sem;

// phosphor-sel-logger
#define CONFIG_THREAD_PRI_SEL_LOGGER 10
#define SEL_LOGGER_THREAD_STACK_SIZE (32 * 1024)
int sel_logger_init();
extern struct k_sem sel_logger_ready_sem;

// phosphor-state-manager
#define CONFIG_THREAD_PRI_BMC_STATE 10
#define BMC_STATE_THREAD_STACK_SIZE (32 * 1024)
int bmc_state_init();
extern struct k_sem bmc_state_ready_sem;

// phosphor-debug-collector
#define CONFIG_THREAD_PRI_DUMP_MANAGER 10
#define DUMP_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int dump_manager_init();
extern struct k_sem dump_manager_ready_sem;

// systemd-networkd
#define CONFIG_THREAD_PRI_SYSTEMD_NETWORKD 10
#define SYSTEMD_NETWORKD_THREAD_STACK_SIZE (32 * 1024)
int systemd_networkd_init();
extern struct k_sem systemd_networkd_ready_sem;

#define CONFIG_THREAD_PRI_ENTITY 10
#define ENTITY_MANAGER_THREAD_STACK_SIZE (20 * 1024)
int entity_manager_init();
extern struct k_sem entity_manager_ready_sem;

#define CONFIG_THREAD_PRI_FRU 10
#define FRU_DEVICE_THREAD_STACK_SIZE (12 * 1024)
int fru_device_init();
extern struct k_sem fru_device_ready_sem;

#define CONFIG_THREAD_PRI_CALLBACK_MANAGER 10
#define CALLBACK_MANAGER_THREAD_STACK_SIZE (32 * 1024)
int callback_manager_init();
extern struct k_sem callback_manager_ready_sem;

#define CONFIG_THREAD_PRI_PHOSPHOR_HOSTLOGGER 10
#define PHOSPHOR_HOSTLOGGER_THREAD_STACK_SIZE 64 * 1024)
int phosphor_hostlogger_init();
extern struct k_sem phosphor_hostlogger_ready_sem;

//kcs bridge
#define CONFIG_THREAD_PRI_KCS_BRIDGE 10
#define KCS_BRIDGE_THREAD_STACK_SIZE (32 * 1024)
int kcs_bridge_init();
extern struct k_sem kcs_bridge_ready_sem;

//certificate manager
#define CONFIG_THREAD_PRI_CERTIFICATE_MANAGER 10
#define CERTIFICATE_MANAGER_THREAD_STACK_SIZE (20 * 1024)
int certificate_manager_init();
extern struct k_sem certificate_manager_ready_sem;

//i2ctest
int i2ctest_init();
//kcstest
int kcstest_init();
//eepromtest
int eepromtest_init();
//gpiotest
int gpiotest_init();
//ledtest
int ledtest_init();
//uarttest
int uarttest_init();
//hwmontest
int hwmontest_init();

#ifdef __cplusplus
}
#endif

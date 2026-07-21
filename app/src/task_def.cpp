#include <zephyr/kernel.h>
// #include <systemd/sd-bus.h>
#include "task_def.hpp"
#include "task_enable.hpp"
#include <printk_thread.h>
#include <string.h>

#define APP_PRI 10 // zephyr priority = 14 - APP_PRI

int create_task_with_pthread(pthread_t *thread, const char *name, void *stack, size_t stack_size, void* (*routine)(void *), bool join_flag)
{
    int ret;
    pthread_attr_t attr;
    static uint32_t thread_count = 1;

    printk_thread("[%u] Start %s thread ...", thread_count, name);

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printk_thread("pthread_attr_init failed: task %s, %d, %s\n", name, ret, strerror(ret));
        return ret;
    }

    ret = pthread_attr_setstack(&attr, stack, stack_size);
    if (ret != 0)
    {
        printk_thread("pthread_attr_setstacksize failed: task %s, %d, %s\n", name, ret, strerror(ret));
        return ret;
    }

    ret = pthread_create(thread, &attr, routine, NULL);
    if (ret != 0)
    {
        printk_thread("pthread_create failed: task %s, %d, %s\n", name, ret, strerror(ret));
        return ret;
    }

    pthread_setname_np(*thread, name);

    /* Set priority for non-broker threads using POSIX API */
    if (strncmp(name, "broker", 6) != 0)
    {
        struct sched_param param = { .sched_priority = APP_PRI };
        pthread_setschedparam(*thread, SCHED_OTHER, &param);
    }

    if (join_flag)
    {
        pthread_join(*thread, NULL);
    }

    thread_count++;

    return 0;
}

//net config init
// K_SEM_DEFINE(net_config_init_ready_sem, 0, 1);

// #if defined(CONFIG_OPENBMC_ZBUS_BROKER) && defined(ENABLE_OPENBMC_ZBUS_BROKER)
#if defined(ENABLE_DBUS_BROKER)
// zbus broker
// MODULE_DEFINE_CHAN_OBSERVER(zbus_broker);
extern "C" {
    extern int dbus_broker_main(void);
}
K_SEM_DEFINE(dbus_broker_ready_sem, 0, 1);

void* dbus_broker_thread_handler(void *arg)
{
    // dbus_broker_main();
    return NULL;
}

int dbus_broker_init()
{
    // pthread_t thread;

    // CREATE_TASK_WITH_PTHREAD(&thread, dbus_broker_thread, DBUS_BROKER_THREAD_STACK_SIZE);

    k_sem_give(&dbus_broker_ready_sem);

    return 0;
}
#else
int dbus_broker_init()
{
    printk_thread("dbus broker not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_HOST_IPMID) && defined(ENABLE_OPENBMC_PHOSPHOR_HOST_IPMID)
// phosphor-host-ipmid
// MODULE_DEFINE_CHAN_OBSERVER(ipmid_host);
// MODULE_DEFINE_CHAN_OBSERVER_MATCH(ipmid_host);
// MODULE_DEFINE_CHAN_OBSERVER(control_host);
extern int ipmid_main();
K_SEM_DEFINE(host_ipmid_ready_sem, 0, 1);

void* ipmi_thread_handler(void *arg)
{
    ipmid_main();
    return NULL;
}

int ipmid_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, ipmi_thread, IPMI_THREAD_STACK_SIZE);

    return 0;
}
#else
int ipmid_init()
{
    printk_thread("ipmid not support\n");
    return 0;
}
#endif

#if defined(CONFIG_BUSCTL) && defined(ENABLE_BUSCTL)
// busctl
// MODULE_DEFINE_CHAN_OBSERVER(busctl);
extern int busctl_main();
K_SEM_DEFINE(busctl_ready_sem, 0, 1);

void* busctl_thread_handler(void *arg)
{
    busctl_main();
    return NULL;
}

int busctl_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, busctl_thread, BUSCTL_THREAD_STACK_SIZE);

    return 0;
}
#else
int busctl_init()
{
    printk_thread("busctl not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_LED_SYSFS) && defined(ENABLE_OPENBMC_PHOSPHOR_LED_SYSFS)
// phosphor-led-sysfs
MODULE_DEFINE_CHAN_OBSERVER(led_sysfs);
extern int led_sysfs_main(int argc, char** argv);
K_SEM_DEFINE(led_sysfs_ready_sem, 0, 1);

void* led_sysfs_thread_handler(void *arg)
{
    char* argv[] = {
        "./phosphor-ledcontroller",
        "--path",
        "/sys/class/leds/sysfaultled"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    led_sysfs_main(argc, argv);
    return NULL;
}

int led_sysfs_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, led_sysfs_thread, LED_SYSFS_THREAD_STACK_SIZE);

    return 0;
}
#else
int led_sysfs_init()
{
    printk_thread("led_sysfs not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_LED_MANAGER) && defined(ENABLE_OPENBMC_PHOSPHOR_LED_MANAGER)
// phosphor-led-manager
MODULE_DEFINE_CHAN_OBSERVER(led_manager);
extern int led_manager_main(int argc, char *argv[]);
K_SEM_DEFINE(led_manager_ready_sem, 0, 1);

void* led_manager_thread_handler(void *arg)
{
    led_manager_main(0, NULL);

    // char* argv[] = {
    //     "./phosphor-led-manager",
    //     "-c",
    //     "/path/to/config.json"
    // };
    // int argc = sizeof(argv) / sizeof(argv[0]);

    // led_manager_main(argc, argv);
    return NULL;
}

int led_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, led_manager_thread, LED_MANAGER_THREAD_STACK_SIZE);

    return 0;
}
#else
int led_manager_init()
{
    printk_thread("led_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_WATCHDOG) && defined(ENABLE_OPENBMC_PHOSPHOR_WATCHDOG)
// phosphor-watchdog
MODULE_DEFINE_CHAN_OBSERVER(phosphor_watchdog);
int watchdog_main();
K_SEM_DEFINE(watchdog_ready_sem, 0, 1);

void* phosphor_watchdog_thread_handler(void *arg)
{
    watchdog_main();
    return NULL;
}

int watchdog_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, phosphor_watchdog_thread, WATCHDOG_THREAD_STACK_SIZE);

    return 0;
}
#else
int watchdog_init()
{
    printk_thread("watchdog not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_SETTINGS_MANAGER) && defined(ENABLE_OPENBMC_PHOSPHOR_SETTINGS_MANAGER)
// phosphor-settings
MODULE_DEFINE_CHAN_OBSERVER(setting_manager);
extern int setting_manager_main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]);
K_SEM_DEFINE(setting_manager_ready_sem, 0, 1);

void* setting_manager_thread_handler(void *arg)
{
    setting_manager_main(0, NULL);
    return NULL;
}

int setting_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, setting_manager_thread, SETTING_MANAGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int setting_manager_init()
{
    printk_thread("setting_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_NETWORK) && defined(ENABLE_OPENBMC_PHOSPHOR_NETWORK)
// phosphor-network
MODULE_DEFINE_CHAN_OBSERVER(network);
int network_manager_main();
K_SEM_DEFINE(network_manager_ready_sem, 0, 1);

void* network_manager_thread_handler(void *arg)
{
    network_manager_main();
    return NULL;
}

int network_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, network_manager_thread, NETWORK_MANAGER_THREAD_STACK_SIZE);

    return 0;
}
#else
int network_manager_init()
{
    printk_thread("network_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_IPMBBRIDGE) && defined(ENABLE_OPENBMC_IPMBBRIDGE)
// ipmbbridged
MODULE_DEFINE_CHAN_OBSERVER(ipmbbridge);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(ipmbbridge);
extern int ipmbbridge_main();
K_SEM_DEFINE(ipmbbridge_ready_sem, 0, 1);

void* ipmbbridge_thread_handler(void *arg)
{
    ipmbbridge_main();
    return NULL;
}
int ipmbbridge_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, ipmbbridge_thread, IPMBBRIDGE_THREAD_STACK_SIZE);
    return 0;
}
#else
int ipmbbridge_init()
{
    printk_thread("ipmbbridge not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_OBMC_CONSOLE) && defined(ENABLE_OPENBMC_OBMC_CONSOLE)
// obmc_console
MODULE_DEFINE_CHAN_OBSERVER(obmc_console);
int console_server_main(int argc, char **argv);
K_SEM_DEFINE(obmc_console_ready_sem, 0, 1);

void* obmc_console_thread_handler(void *arg)
{
    //obmc-console-server  --config /etc/obmc-console/server.%i.conf %i
    //obmc-console-server  --config /etc/obmc-console/server.ttyS0.conf ttyS0

    char* argv[] = {
        "./obmc-console-server",
        "--config",
        CONFIG_FS_ROOT_OVERLAY"/etc/obmc-console.conf",
        "vttyS2"
    };

    int argc = sizeof(argv) / sizeof(argv[0]);
    console_server_main(argc,argv);
    return NULL;
}
int obmc_console_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, obmc_console_thread, OBMC_CONSOLE_THREAD_STACK_SIZE);
    return 0;
}
#else
int obmc_console_init()
{
    printk_thread("obmc_console not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_TIME_MANAGER) && defined(ENABLE_OPENBMC_PHOSPHOR_TIME_MANAGER)
// timedated
MODULE_DEFINE_CHAN_OBSERVER(timedated);
int timedated_main();
K_SEM_DEFINE(timedated_ready_sem, 0, 1);

void* timedated_thread_handler(void *arg)
{
    timedated_main();
    return NULL;
}
int timedated_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, timedated_thread, TIME_DATED_THREAD_STACK_SIZE);
    return 0;
}

// phosphor-time-manager
MODULE_DEFINE_CHAN_OBSERVER(time_manager);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(time_manager);
int time_manager_main();
K_SEM_DEFINE(time_manager_ready_sem, 0, 1);

void* time_manager_thread_handler(void *arg)
{
    time_manager_main();
    return NULL;
}
int time_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, time_manager_thread, TIME_MANAGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int timedated_init()
{
    printk_thread("timedated not support...");
    return 0;
}
int time_manager_init()
{
    printk_thread("time_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_BIOS_SETTINGS_MGR) && defined(ENABLE_OPENBMC_BIOS_SETTINGS_MGR)
// bios_setting_mgr
MODULE_DEFINE_CHAN_OBSERVER(bios_settings_mgr);
int bios_settings_mgr_main();
K_SEM_DEFINE(bios_settings_ready_sem, 0, 1);

void* bios_settings_mgr_thread_handler(void *arg)
{
    bios_settings_mgr_main();
    return NULL;
}
int bios_settings_mgr_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, bios_settings_mgr_thread, BIOS_SETTINGS_MGR_THREAD_STACK_SIZE);
    return 0;
}
#else
int bios_settings_mgr_init()
{
    printk_thread("bios_settings_mgr not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_IPMITOOL) && defined(ENABLE_OPENBMC_IPMITOOL)
//ipmitool
MODULE_DEFINE_CHAN_OBSERVER(ipmitool);
int ipmitool_main();
K_SEM_DEFINE(ipmitool_ready_sem, 0, 1);

void* ipmitool_thread_handler(void *arg)
{
    ipmitool_main();
    return NULL;
}
int ipmitool_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, ipmitool_thread, IPMITOOL_THREAD_STACK_SIZE);
    return 0;
}
#else
int ipmitool_init()
{
    printk_thread("ipmitool not support\n");
    return 0;
}
#endif

#if defined(CONFIG_PHOSPHOR_USER_MANAGER) && defined(ENABLE_PHOSPHOR_USER_MANAGER)
//user_manager
MODULE_DEFINE_CHAN_OBSERVER(user_manager);
int user_manager_main(int /*argc*/, char** /*argv*/);
K_SEM_DEFINE(user_manager_ready_sem, 0, 1);

void* user_manager_thread_handler(void *arg)
{
    user_manager_main(0, NULL);
    return NULL;
}
int user_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, user_manager_thread, USER_MANAGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int user_manager_init()
{
    printk_thread("user_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_BMCWEB) && defined(ENABLE_OPENBMC_BMCWEB)
//bmcweb
MODULE_DEFINE_CHAN_OBSERVER(bmcweb);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(bmcweb);
int bmcweb_main(int /*argc*/, char** /*argv*/);
K_SEM_DEFINE(bmcweb_ready_sem, 0, 1);

void* bmcweb_thread_handler(void *arg)
{
    bmcweb_main(0, NULL);
    return NULL;
}
int bmcweb_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, bmcweb_thread, BMCWEB_THREAD_STACK_SIZE);
    return 0;
}
#else
int bmcweb_init()
{
    printk_thread("bmcweb not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_OBJMGR) && defined(ENABLE_OPENBMC_PHOSPHOR_OBJMGR)
// phosphor-objmgr
// MODULE_DEFINE_CHAN_OBSERVER(phosphor_objmgr);
// MODULE_DEFINE_CHAN_OBSERVER_MATCH(phosphor_objmgr);
K_SEM_DEFINE(objmgr_ready_sem, 0, 1);

int objmgr_main();
void* objmgr_thread_handler(void *arg)
{
    objmgr_main();
    return NULL;
}
int objmgr_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, objmgr_thread, OBJMGR_THREAD_STACK_SIZE);
    return 0;
}
#else
int objmgr_init()
{
    printk_thread("phosphor-objmgr not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_BMC_CODE_MGT) && defined(ENABLE_OPENBMC_PHOSPHOR_BMC_CODE_MGT)
// phosphor-bmc-code-mgt
MODULE_DEFINE_CHAN_OBSERVER(phosphor_software_updater);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(phosphor_software_updater);
int software_updater_main();
K_SEM_DEFINE(software_updater_ready_sem, 0, 1);

void* software_updater_thread_handler(void *arg)
{
    software_updater_main();
    return NULL;
}
int software_updater_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, software_updater_thread, UPDATER_THREAD_STACK_SIZE);
    return 0;
}

int image_manager_main();
MODULE_DEFINE_CHAN_OBSERVER(phosphor_image_manager);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(phosphor_image_manager);
K_SEM_DEFINE(image_manager_ready_sem, 0, 1);

void* image_manager_thread_handler(void *arg)
{
    image_manager_main();
    return NULL;
}
int image_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, image_manager_thread, IMAGE_THREAD_STACK_SIZE);
    return 0;
}
#else
int software_updater_init()
{
    printk_thread("image update not support");
    return 0;
}

int image_manager_init()
{
    printk_thread("image manager not support");
    return 0;
}
#endif

#ifdef CONFIG_OPENBMC_DBUS_SENSORS
#if defined(ENABLE_OPENBMC_DBUS_SENSORS_ADC)
// dbus-sensors
//MODULE_DEFINE_CHAN_OBSERVER(adc_sensor);
int adc_sensor_main();
K_SEM_DEFINE(adc_sensor_ready_sem, 0, 1);

void* adc_sensor_thread_handler(void *arg)
{
    adc_sensor_main();
    return NULL;
}
int adc_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, adc_sensor_thread, ADC_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int adc_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif

#if defined(ENABLE_OPENBMC_DBUS_SENSORS_EXTERNAL)
//MODULE_DEFINE_CHAN_OBSERVER(external_sensor);
int external_sensor_main();
K_SEM_DEFINE(external_sensor_ready_sem, 0, 1);

void* external_sensor_thread_handler(void *arg)
{
    external_sensor_main();
    return NULL;
}
int external_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, external_sensor_thread, EXTERNAL_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int external_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif

#if defined(ENABLE_OPENBMC_DBUS_SENSORS_FAN)
MODULE_DEFINE_CHAN_OBSERVER(fan_sensor);
int fan_sensor_main();
struct k_thread FAN_SENSOR_thread;
Z_KERNEL_STACK_DEFINE_IN(FAN_SENSOR_thread_stack, FAN_SENSOR_THREAD_STACK_SIZE, THREAD_STACK_SECTION);
K_SEM_DEFINE(fan_sensor_ready_sem, 0, 1);

void* fan_sensor_thread_handler(void *arg)
{
    fan_sensor_main();
    return NULL;
}
int fan_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, fan_sensor_thread, FAN_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int fan_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif

#if defined(ENABLE_OPENBMC_DBUS_SENSORS_HWMON_TEMP)
// MODULE_DEFINE_CHAN_OBSERVER(hwmontemp_sensor);
int hwmon_temp_sensor_main();
K_SEM_DEFINE(hwmon_temp_sensor_ready_sem, 0, 1);

void* hwmon_temp_sensor_thread_handler(void *arg)
{
    hwmon_temp_sensor_main();
    return NULL;
}
int hwmon_temp_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, hwmon_temp_sensor_thread, HWMON_TEMP_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int hwmon_temp_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif


#if defined(ENABLE_OPENBMC_DBUS_SENSORS_INTELCPU)
MODULE_DEFINE_CHAN_OBSERVER(intelcpu_sensor);
int intel_cpu_sensor_main();
K_SEM_DEFINE(intel_cpu_sensor_ready_sem, 0, 1);

void* intel_cpu_sensor_thread_handler(void *arg)
{
    intel_cpu_sensor_main();
    return NULL;
}
int intel_cpu_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, intel_cpu_sensor_thread, INTEL_CPU_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int intel_cpu_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif


#if defined(ENABLE_OPENBMC_DBUS_SENSORS_INTRUSION)
MODULE_DEFINE_CHAN_OBSERVER(intrusion_sensor);
int intrusion_sensor_main();
K_SEM_DEFINE(intrusion_sensor_ready_sem, 0, 1);

void* intrusion_sensor_thread_handler(void *arg)
{
    intrusion_sensor_main();
    return NULL;
}
int intrusion_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, intrusion_sensor_thread, INTRUSION_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int intrusion_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif

#if defined(ENABLE_OPENBMC_DBUS_SENSORS_PSU)
// MODULE_DEFINE_CHAN_OBSERVER(psu_sensor);
int psu_sensor_main();
K_SEM_DEFINE(psu_sensor_ready_sem, 0, 1);

void* psu_sensor_thread_handler(void *arg)
{
    psu_sensor_main();
    return NULL;
}
int psu_sensor_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, psu_sensor_thread, PSU_SENSOR_THREAD_STACK_SIZE);
    return 0;
}
#else
int psu_sensor_init()
{
    printk_thread("dus-sensors not support\n");
    return 0;
}
#endif

#endif


#ifdef CONFIG_ZEPHYR_OSAL
//i2ctest
extern "C" int i2c_main_test();
int i2ctest_init()
{
    printk_thread("\n Start i2c_main_test...\n");
    int ret = i2c_main_test();
    if (ret < 0) {
        printk_thread("\n i2c_main_test failed...\n");
        return -1;
    }
    printk_thread("\n i2c_main_test successful...\n");
    return 0;
}

extern "C" int led_main();
extern "C" int led_main_test();
int ledtest_init()
{
    int ret = 0;
    // printk_thread("\n Start led_main...\n");
    // ret = led_main();
    // if (ret < 0) {
    // 	printk_thread("\n led_main failed...\n");
    // 	return -1;
    // }
    // printk_thread("\n led_main successful...\n");

    printk_thread("\n Start led_main_test...\n");
    ret = led_main_test();
    if (ret < 0) {
        printk_thread("\n led_main_test failed...\n");
        return -1;
    }
    printk_thread("\n led_main_test successful...\n");
    return 0;
}

extern "C" int gpio_main_test();
int gpiotest_init()
{
    int ret = 0;

    printk_thread("\n Start gpio_main_test...\n");
    ret = gpio_main_test();
    if (ret < 0) {
        printk_thread("\n gpio_main_test failed...\n");
        return -1;
    }
    printk_thread("\n gpio_main_test successful...\n");
    return 0;
}

extern "C" int uart_main_test();
int uarttest_init()
{
    int ret = 0;

    printk_thread("\n Start uart_main_test...\n");
    ret = uart_main_test();
    if (ret < 0) {
        printk_thread("\n uart_main_test failed...\n");
        return -1;
    }
    printk_thread("\n uart_main_test successful...\n");
    return 0;
}

extern "C" int kcs_main_test();
int kcstest_init()
{
    printk_thread("\n Start kcs_main_test...\n");
    int ret = kcs_main_test();
    if (ret < 0) {
            printk_thread("\n kcs_main_test failed...\n");
            return -1;
    }
    printk_thread("\n kcs_main_test successful...\n");
    return 0;
}

extern "C" int hwmon_main_test();
int hwmontest_init()
{
    printk_thread("\n Start hwmon_main_test...\n");
    int ret = hwmon_main_test();
    if (ret < 0) {
            printk_thread("\n hwmon_main_test failed...\n");
            return -1;
    }
    printk_thread("\n hwmon_main_test successful...\n");
    return 0;
}

#ifdef CONFIG_EEPROM
extern "C" int eeprom_main_test();
int eepromtest_init()
{
    printk_thread("\n Start eeprom_main_test...\n");
    int ret = eeprom_main_test();
    if (ret < 0) {
            printk_thread("\n eeprom_main_test failed...\n");
            return -1;
    }
    printk_thread("\n eeprom_main_test successful...\n");
    return 0;
}
#endif
#endif



K_SEM_DEFINE(net_stack_cfg_ready_sem, 0, 1);

extern "C" {
    extern int net_stack_cfg_do_once(void);
}

void* net_stack_cfg_thread_handler(void *arg)
{
    printk("net_stack_cfg_thread: Starting network configuration\n");

    int ret = net_stack_cfg_do_once();
    if (ret == 0) {
        printk("net_stack_cfg_thread: Network configuration completed successfully\n");
    } else {
        printk("net_stack_cfg_thread: Network configuration failed (ret=%d)\n", ret);
    }

    k_sem_give(&net_stack_cfg_ready_sem);

    return NULL;
}

int net_stack_cfg_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, net_stack_cfg_thread, NET_STACK_CFG_THREAD_STACK_SIZE);

    return 0;
}


#if defined(CONFIG_OPENBMC_PHOSPHOR_NET_IPMID) && defined(ENABLE_OPENBMC_PHOSPHOR_NET_IPMID)
/// MODULE_DEFINE_CHAN_OBSERVER(ipmi_net);
// MODULE_DEFINE_CHAN_OBSERVER_MATCH(ipmi_net);
extern int net_ipmid_main(int argc, char* argv[]);
K_SEM_DEFINE(net_ipmid_ready_sem, 0, 1);


static char *net_ipmid_argv[] = {
    "net-ipmid",
    "--channel=eth0",
    NULL
};

void* net_ipmid_thread_handler(void *arg)
{
    printk("net_ipmid_thread: Started\n");

    int argc = sizeof(net_ipmid_argv) / sizeof(net_ipmid_argv[0]) - 1;

    int ret = net_ipmid_main(argc, net_ipmid_argv);
    printk("net_ipmid_thread: Main function returned (ret=%d)\n", ret);

    return NULL;
}

int net_ipmid_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, net_ipmid_thread, NET_IPMID_THREAD_STACK_SIZE);
    return 0;
}
#else
int net_ipmid_init()
{
    printk_thread("net_ipmid not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_LOGGING) && defined(ENABLE_OPENBMC_PHOSPHOR_LOGGING)
extern "C" int logging_main(void);
K_SEM_DEFINE(logging_ready_sem, 0, 1);

void* logging_thread_handler(void *arg)
{
    logging_main();
    return NULL;
}
int logging_init()
{
    pthread_t thread;
    CREATE_TASK_WITH_PTHREAD(&thread, logging_thread, PHOSPHOR_LOGGING_THREAD_STACK_SIZE);
    return 0;
}
#else
int logging_init()
{
    printk_thread("phosphor-logging not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_SEL_LOGGER) && defined(ENABLE_OPENBMC_PHOSPHOR_SEL_LOGGER)
//phosphor-sel-logger
MODULE_DEFINE_CHAN_OBSERVER(sel_logger);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(sel_logger);
int sel_logger_main(int, char*[]);
K_SEM_DEFINE(sel_logger_ready_sem, 0, 1);

void* sel_logger_thread_handler(void *arg)
{
    sel_logger_main(0, NULL);
    return NULL;
}
int sel_logger_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, sel_logger_thread, SEL_LOGGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int sel_logger_init()
{
       printk_thread("phosphor-sel-logger not support\n");
       return 0;
}
#endif


#if defined(CONFIG_OPENBMC_PHOSPHOR_STATE_MANAGER) && defined(ENABLE_OPENBMC_PHOSPHOR_BMC_STATE_MANAGER)
//phosphor-bmc-state-manager
MODULE_DEFINE_CHAN_OBSERVER(bmc_state_manager);
int bmc_state_main();
K_SEM_DEFINE(bmc_state_ready_sem, 0, 1);

void* bmc_state_thread_handler(void *arg)
{
    bmc_state_main();
    return NULL;
}
int bmc_state_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, bmc_state_thread, BMC_STATE_THREAD_STACK_SIZE);
    return 0;
}
#else
int bmc_state_init()
{
    printk_thread("bmc-state-manager not support\n");
    return 0;
}
#endif


#if defined(CONFIG_OPENBMC_PHOSPHOR_DEBUG_COLLECTOR) && defined(ENABLE_OPENBMC_PHOSPHOR_DEBUG_COLLECTOR)
//PHOSPHOR_DEBUG_COLLECTOR
int dump_manager_main();
K_SEM_DEFINE(dump_manager_ready_sem, 0, 1);

void* dump_manager_thread_handler(void *arg)
{
    dump_manager_main();
    return NULL;
}
int dump_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, dump_manager_thread, DUMP_MANAGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int dump_manager_init()
{
    printk_thread("phosphor-debug-collector not support\n");
    return 0;
}
#endif


#ifdef ENABLE_SYSTEMD_NETWORKD
//systemd-networkd
MODULE_DEFINE_CHAN_OBSERVER(systemd_network);
extern int networkd_main();
K_SEM_DEFINE(systemd_networkd_ready_sem, 0, 1);

void* systemd_networkd_thread_handler(void *arg)
{
    networkd_main();
    return NULL;
}
int systemd_networkd_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, systemd_networkd_thread, SYSTEMD_NETWORKD_THREAD_STACK_SIZE);

    return 0;
}
#else
int systemd_networkd_init()
{
    printk_thread("systemd-networkd not support\n");
    return 0;
}
#endif


#if defined(CONFIG_OPENBMC_X86_POWER_CONTROL) && defined(ENABLE_OPENBMC_X86_POWER_CONTROL)
//x86_power_control
MODULE_DEFINE_CHAN_OBSERVER(power_control);
int power_control_main(int argc, char* argv[]);
K_SEM_DEFINE(power_control_ready_sem, 0, 1);

void* power_control_thread_handler(void *arg)
{
    power_control_main(0, NULL);
    return NULL;
}
int power_control_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, power_control_thread, POWER_CONTROL_THREAD_STACK_SIZE);
    return 0;
}
#else
int power_control_init()
{
    printk_thread("power_control not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_ENTITY_MANAGER) && defined(ENABLE_OPENBMC_ENTITY_MANAGER)
// entity-manager
// MODULE_DEFINE_CHAN_OBSERVER(entity_manager);
// MODULE_DEFINE_CHAN_OBSERVER_MATCH(entity_manager);
extern int entity_main();
K_SEM_DEFINE(entity_manager_ready_sem, 0, 1);

void* entity_manager_thread_handler(void *arg)
{
    entity_main();
    return NULL;
}
int entity_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, entity_manager_thread, ENTITY_MANAGER_THREAD_STACK_SIZE);

    return 0;
}
#else
int entity_manager_init()
{
    printk_thread("entity_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_ENTITY_MANAGER) && defined(ENABLE_OPENBMC_FRU_DEVICE)
// phosphor-logging
// MODULE_DEFINE_CHAN_OBSERVER(fru_device);
extern int fru_main();
K_SEM_DEFINE(fru_device_ready_sem, 0, 1);

void* fru_device_thread_handler(void *arg)
{
    fru_main();
    return NULL;
}
int fru_device_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, fru_device_thread, FRU_DEVICE_THREAD_STACK_SIZE);

    return 0;
}
#else
int fru_device_init()
{
    printk_thread("fru_device not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_CALLBACK_MANAGER) && defined(ENABLE_OPENBMC_CALLBACK_MANAGER)
// callback-manager
MODULE_DEFINE_CHAN_OBSERVER(callback_manager);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(callback_manager);
extern int callback_manager_main(int argc, char** argv);
K_SEM_DEFINE(callback_manager_ready_sem, 0, 1);

void* callback_manager_thread_handler(void *arg)
{
    callback_manager_main(0, NULL);
    return NULL;
}
int callback_manager_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, callback_manager_thread, CALLBACK_MANAGER_THREAD_STACK_SIZE);

    return 0;
}
#else
int callback_manager_init()
{
    printk_thread("callback_manager not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_HOSTLOGGER) && defined(ENABLE_OPENBMC_PHOSPHOR_HOSTLOGGER)
// phosphor-hostlogger
MODULE_DEFINE_CHAN_OBSERVER(phosphor_hostlogger);
MODULE_DEFINE_CHAN_OBSERVER_MATCH(phosphor_hostlogger);
extern int phosphor_hostlogger_main(int argc, char** argv);
K_SEM_DEFINE(phosphor_hostlogger_ready_sem, 0, 1);

void* phosphor_hostlogger_thread_handler(void *arg)
{
    phosphor_hostlogger_main(0, NULL);
    return NULL;
}
int phosphor_hostlogger_init()
{
    pthread_t thread;

    CREATE_TASK_WITH_PTHREAD(&thread, phosphor_hostlogger_thread, PHOSPHOR_HOSTLOGGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int phosphor_hostlogger_init()
{
    printk_thread("phosphor-hostlogger not support\n");
    return 0;
}
#endif


#if defined(CONFIG_OPENBMC_KCSBRIDGE) && defined(ENABLE_OPENBMC_KCSBRIDGE)
// kcs bridge
// MODULE_DEFINE_CHAN_OBSERVER(kcs_bridge);
extern "C" int kcsbridge_main();
K_SEM_DEFINE(kcs_bridge_ready_sem, 0, 1);

void* kcs_bridge_thread_handler(void *arg)
{
    // char* argv[] = {
    //     "kcsbridged",
    //     "--channel=ipmi-kcs"
    // };
    // int argc = sizeof(argv) / sizeof(argv[0]);
    // kcs_main(argc, argv);
    kcsbridge_main();
    return NULL;
}
int kcs_bridge_init()
{
    pthread_t thread;
    CREATE_TASK_WITH_PTHREAD(&thread, kcs_bridge_thread, KCS_BRIDGE_THREAD_STACK_SIZE);
    return 0;
}
#else
int kcs_bridge_init()
{
    printk_thread("kcs bridge not support\n");
    return 0;
}
#endif

#if defined(CONFIG_OPENBMC_PHOSPHOR_CERTIFICATE_MANAGER) && defined(ENABLE_OPENBMC_CERTIFICATE_MANAGER)
MODULE_DEFINE_CHAN_OBSERVER(certificate_manager);
extern int certificate_manager_main(int argc, char* argv[]);
K_SEM_DEFINE(certificate_manager_ready_sem, 0, 1);

static char *cert_manager_argv[] = {
    "certificate-manager",
    "--type",      "server",
    "--endpoint",  "https",
    "--path",      "/overlay/etc/ssl/certs/https/server.pem",
    NULL
};
void* certificate_manager_thread_handler(void *arg)
{
    printk("certificate_manager_thread: Started\n");
    int argc = sizeof(cert_manager_argv) / sizeof(cert_manager_argv[0]) - 1;

    int ret = certificate_manager_main(argc, cert_manager_argv);
    printk("certificate_manager_thread: Main function returned (ret=%d)\n", ret);

    return NULL;
}
int certificate_manager_init()
{
    pthread_t thread;
    CREATE_TASK_WITH_PTHREAD(&thread, certificate_manager_thread, CERTIFICATE_MANAGER_THREAD_STACK_SIZE);
    return 0;
}
#else
int certificate_manager_init()
{
    printk_thread("certificate manager not support\n");
    return 0;
}
#endif

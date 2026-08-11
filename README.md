# **LS1020 OpenBMC 20260811 版本发布说明**

## 概述

#### OpenBMC Zephyr SDK 是基于 Zephyr RTOS 的 OpenBMC 软件运行环境，在资源受限的 BMC SoC 上实现 OpenBMC 核心功能栈：D-Bus 消息总线、sdbusplus/basu 通信库、phosphor-logging 日志框架、entity-manager 配置管理、dbus-sensors 传感器采集，以及 host-ipmid / net-ipmid / KCS 桥接等 BMC 管控通道。

#### 本次版本发布，在凌思微基板管理控制芯片（Lite-BMC） **LS1020** 的 2026-04-24 软件基线之上，新增了**文件系统**、**传感器采集**、**配置管理**、**KCS 桥接**以及若干支撑库，还加入了在 BMC shell 上可执行的 **busctl** 和 **ipmi** 等指令，形成可实际采集传感器、可经 IPMI/KCS 与主机及网络交互的较完整 BMC 软件栈。

#### 本次版本发布包含的核心 OpenBMC 模块如下：

> - phosphor-host-ipmid 
>
> - phosphor-net-ipmid
>
> - phosphor-objmgr
>
> - phosphor-dbus-interfaces
>
> - phosphor-entity-manager
>
> - phosphor-dbus-sensors
>
> - phosphor-logging
>

## 软件架构

#### OpenBMC软件架构由OpenBMC的上层业务应用层、核心中间件层、通用基础依赖层和安全运维层组成。各层包含的模块及其主要功能，以及各模块之间的依赖关系请参考如下面两张树形图：

##### OpenBMC 模块组成结构图
```ts
├─ 上层业务应用（用户功能层）
│  ├─ phosphor-host-ipmid       # 主机本地 IPMI（KCS/BT接口）
│  ├─ phosphor-net-ipmid        # 远程网络 IPMI（RMCP+）
│  ├─ kcsbridge                 # 主机 KCS 桥接（KCS 报文经 D-Bus 转发至 IPMI 服务）
│  ├─ dbus-sensors              # 传感器采集（HwmonTemp/PSUSensor/IntelCPU/ADC/Fan/Intrusion/External）
│  ├─ entity-manager            # FRU/传感器/机箱配置管理（含 fru-device）
│  ├─ phosphor-logging          # SEL/ELOG/Redfish 事件日志
│  └─ phosphor-objmgr           # 对象管理器（接口聚合、属性缓存）
├─ 核心中间件层（IPC+事件驱动）
│  ├─ phosphor-dbus-interfaces  # D-Bus 接口YAML标准定义
│  ├─ sdbusplus                 # C++ D-Bus 封装（生成工具+客户端/服务端）
│  ├─ D-Bus协议栈
│  │  └─ basu                   # 无 systemd 精简版 sd-bus（兼容API）
│  ├─ 事件循环核心
│  │  └─ sdeventplus            # 异步IO/定时器/信号/子进程管理
│  └─ 总线守护进程
│     └─ dbus-broker            # 高性能 D-Bus 路由（替代dbus-daemon）
├─ 通用基础依赖层（全模块共用）
│  ├─ C++通用工具库
│  │  ├─ boost                  # 异步IO/线程/容器/序列化
│  │  ├─ stdplus                # C++ 标准库增强（嵌入式适配）
│  │  ├─ function2              # 高效函数绑定/回调
│  │  ├─ fmt                    # 日志/字符串格式化
│  │  ├─ nlohmann_json          # JSON 解析/序列化
│  │  └─ tinyxml2               # 轻量 XML 解析
│  ├─ RTOS&系统抽象
│  │  ├─ zephyr                 # Zephyr RTOS 内核
│  │  ├─ zephyr-osal            # 跨OS抽象层（Linux/Zephyr适配）
│  │  ├─ zephyr-devfs           # Zephyr设备文件系统抽象（hwmon_xxx/gpio/i2c等）
│  │  └─ libgpiod / i2c-tools   # GPIO/i2c 工具库（传感器外设访问）
│  └─ 芯片平台BSP
│     ├─ linkedsemi             # 凌思微 BMC SoC 驱动 BSP
│     └─ ls_sdk                 # LS 系列芯片底层 SDK
└─ 安全&运维层（底层支撑）
   ├─ 加密通信
   │  ├─ mbedtls                # 轻量嵌入式 TLS（HTTPS/IPMI加密）
   │  └─ wolfssl                # 高性能 TLS（国密/FIPS/量子安全）
   ├─ 安全远程运维
   │  └─ wolfssh                # 嵌入式 SSH/SOL/SFTP
   ├─ 代码安全
   │  └─ safeclib               # C标准库安全加固（防缓冲区溢出）
   ├─ 硬件调试
   │  └─ jtag_asd               # BMC远程JTAG调试（CPU/BIOS 排障）
   └─ 文件系统工具
      ├─ mk-rofs                # 只读文件系统镜像制作
      └─ valijson               # JSON Schema 校验（entity-manager配置）
```
##### OpenBMC 核心业务模块依赖关系图
```ts
├─ phosphor-host-ipmid  # 顶层业务：主机本地 IPMI
│  ├─ sdbusplus          # D-Bus C++ 封装（核心通信）
│  │  ├─ nlohmann_json   # JSON 解析（数据序列化）
│  │  └─ basu            # 精简 sd-bus 协议栈（底层通信）
│  ├─ sdeventplus        # 事件循环封装（异步IO / 定时器）
│  │  ├─ sdbusplus       # 依赖 D-Bus 通信
│  │  │  ├─ nlohmann_json
│  │  │  └─ basu
│  │  └─ stdplus         # C++ 标准库增强
│  │     ├─ function2    # 函数绑定/回调
│  │     └─ fmt          # 字符串/日志格式化
│  ├─ phosphor-dbus-interfaces  # D-Bus 接口标准定义
│  │  └─ sdbusplus       # 依赖 D-Bus 封装
│  │     ├─ nlohmann_json
│  │     └─ basu
│  ├─ phosphor-logging   # 故障日志上报
│  │  ├─ nlohmann_json
│  │  ├─ phosphor-dbus-interfaces
│  │  │  └─ sdbusplus
│  │  ├─ sdeventplus
│  │  │  ├─ sdbusplus
│  │  │  └─ stdplus
│  │  └─ sdbusplus
│  ├─ nlohmann_json      # 直接依赖：JSON 数据处理
│  └─ boost              # 直接依赖：通用工具库（序列化/容器）
├─ phosphor-net-ipmid   # 顶层业务：远程网络 IPMI
│  ├─ boost              # 通用工具支撑
│  ├─ phosphor-dbus-interfaces  # D-Bus 接口标准定义
│  │  └─ sdbusplus
│  │     ├─ nlohmann_json
│  │     └─ basu
│  ├─ phosphor-logging   # 故障日志上报
│  │  ├─ nlohmann_json
│  │  ├─ phosphor-dbus-interfaces
│  │  ├─ sdeventplus
│  │  └─ sdbusplus
│  ├─ phosphor-objmgr    # D-Bus 对象管理（专属依赖）
│  └─ sdbusplus          # D-Bus C++ 封装
│     ├─ nlohmann_json
│     └─ basu
├─ kcsbridge            # 顶层业务：主机 KCS 桥接
│  ├─ sdbusplus          # D-Bus C++ 封装（报文经 D-Bus 转发）
│  │  ├─ nlohmann_json
│  │  └─ basu
│  ├─ phosphor-host-ipmid # 依赖主机 IPMI 服务处理命令
│  │  └─ sdbusplus
│  ├─ phosphor-logging   # 日志上报
│  │  └─ sdbusplus
│  └─ sdeventplus        # 事件循环
│     └─ stdplus
├─ entity-manager       # 顶层业务：配置管理（含 fru-device）
│  ├─ sdbusplus          # D-Bus C++ 封装
│  │  ├─ nlohmann_json
│  │  └─ basu
│  ├─ phosphor-objmgr    # 对象管理（配置注册）
│  ├─ phosphor-logging   # 日志上报
│  ├─ valijson           # JSON Schema 校验
│  └─ sdeventplus        # 事件循环
│     └─ stdplus
└─ dbus-sensors         # 顶层业务：传感器采集
   ├─ sdbusplus          # D-Bus C++ 封装（暴露传感器对象）
   │  ├─ nlohmann_json
   │  └─ basu
   ├─ sdeventplus        # 事件循环（io_context 驱动）
   │  └─ stdplus
   ├─ zephyr-devfs       # hwmon_general/hwmon_adc/hwmon_pwm_tach 等 sysfs 驱动（读取硬件）
   ├─ libgpiod / i2c-tools  # GPIO/i2c外设访问
   ├─ entity-manager     # 依赖配置（传感器/FRU 定义）
   │  └─ sdbusplus
   └─ phosphor-logging   # 日志上报
```
#### 各模块代码仓库拉取的地址，详见项目启动仓库中的 west.yml

## 环境搭建

官方文档请参考：[https://docs.zephyrproject.org/latest/develop/getting_started/index.html]

#### 一、需要下载交叉编译工具链
[https://github.com/linkedsemi/xuantie-900-gcc-elf-newlib-x86_64-v3.0.1/tree/dbus-ipmi-v1]

```jsx
指定交叉编译器
export PATH=$PATH:<你的path>Xuantie-900-gcc-elf-newlib-x86_64-V3.0.1/bin
export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
export CROSS_COMPILE=<你的path>/Xuantie-900-gcc-elf-newlib-x86_64-V3.0.1/bin/riscv64-unknown-elf-
```

#### 二、基于 linux 环境的安装、配置步骤：

1. 安装工具
```jsx
   sudo apt update
   sudo apt upgrade
   sudo apt install --no-install-recommends git cmake ninja-build gperf \
      ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
      xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

   验证工具版本
   dtc --version
```

2. 创建工作区目录,拉取项目启动仓库
```jsx
   mkdir <你的path>/[工作区目录]
   cd [工作区目录]
   git clone -b dbus-ipmi-v1 git@github.com:linkedsemi/openbmc_zephyr_app.git 或
   git clone -b dbus-ipmi-v1 https://github.com/linkedsemi/openbmc_zephyr_app.git
```

3. 初始化仓库
```jsx
   cd [工作区目录]
   python3 -m venv openbmc_zephyr_app/.venv
   source openbmc_zephyr_app/.venv/bin/activate
   pip install west
   west init -l openbmc_zephyr_app

   cd openbmc_zephyr_app
   west update
   west zephyr-export
   pip install -r ../zephyr/scripts/requirements.txt
   pip install inflection mako jsonschema
```

4. 检查环境变量
```jsxx
   > cd [工作区目录]
   > 使用 `env |grep zephyr` 查看环境变量：
      ZEPHYR_BASE=[工作区目录]/zephyr - 如没有，可暂时忽略或可以在.bashrc中添加
      PWD=[工作区目录]
      VIRTUAL_ENV=[工作区目录]/openbmc_zephyr_app/.venv
   > 使用 `env |grep com` 和 `env |grep COM` 查看交叉编译器的环境变量：
      ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      CROSS_COMPILE=<你的path>/Xuantie-900-gcc-elf-newlib-x86_64-V3.0.1/bin/riscv64-unknown-elf-
```

#### 三、编译和烧录
```jsx
   > 编译openbmc：
      - cd [工作区目录]
      - west build -p auto -b lsqsh_evb@1os_xip/lsqsh/cpu1 openbmc/openbmc_zephyr_app/app

   > 烧录openbmc：
      - 新版本需要烧录二进制程序和配置文件，使用 cklink 烧录时间较长。所以新版本使用 tftp 应用烧录程序和 json 配置文件
      - 搭建好 tfpt server，或者下载 tftp64 工具，把要烧录的文件拷贝放进 tftp server 的 Current Directory 里面
         - 配置文件：[工作区目录]/build/rofs_bin/flash_rofs.bin
         - 二进制程序：[工作区目录]/build/zephyr/zephyr_flash_bram_xip_lsqsh_evb_1os_xip_lsqsh_cpu1.bin
      - 使用传统方式（FlashProgrammer或flash命令）烧录 zephyr_flash_lsqsh_evb_1os_lsqsh_cpu1.bin，让 zephyr 能够启用 tftpc 去 server下载文件：
         - [工作区目录]/openbmc_zephyr_app/zephyr_flash_lsqsh_evb_1os_lsqsh_cpu1.bin
      - 烧录之后复位，串口打印看到 "success"
      - 使用网线连接电脑网口与BMC网口，配置电脑以太网的IPv4地址在 192.0.2.x 局域网段，比如：192.0.2.3
      - 在串口执行下面的命令：
         tftpc init 192.0.2.3 69;tftpc get_flashcp zephyr_flash_bram_xip_lsqsh_evb_1os_xip_lsqsh_cpu1.bin qspi@40000000 0x0;tftpc get_flashcp flash_rofs.bin qspi@40000000 0xD00000;
      - 看到两个文件的 "verify pass" 后复位 （kernel reboot 或者 power cycle），程序烧录、启动成功
```

## 应用使用说明

#### 一、应用配置
```jsx
1. 启动配置

   编译宏：[具体参考 openbmc_zephyr_app/app/prj.conf]
      CONFIG_OPENBMC_PHOSPHOR_HOST_IPMID=y
      CONFIG_OPENBMC_PHOSPHOR_NET_IPMID=y
      CONFIG_OPENBMC_PHOSPHOR_OBJMGR=y
      CONFIG_OPENBMC_PHOSPHOR_LOGGING=y
      CONFIG_OPENBMC_PHOSPHOR_DBUS_INTERFACES=y
      CONFIG_OPENBMC_ENTITY_MANAGER=y
      CONFIG_OPENBMC_DBUS_SENSORS=y
      CONFIG_OPENBMC_IPMITOOL=y
      CONFIG_OPENBMC_KCSBRIDGE=y
      CONFIG_BASU=y
      CONFIG_DBUS_BROKER=y
      ......

   启动线程控制宏：[具体参考 openbmc_zephyr_app/app/src/task_enable.hpp]
      #define ENABLE_DBUS_BROKER
      #define ENABLE_OPENBMC_PHOSPHOR_OBJMGR
      #define ENABLE_OPENBMC_PHOSPHOR_HOST_IPMID
      #define ENABLE_OPENBMC_PHOSPHOR_NET_IPMID
      #define ENABLE_OPENBMC_KCSBRIDGE
      #define ENABLE_OPENBMC_PHOSPHOR_LOGGING
      #define ENABLE_OPENBMC_DBUS_SENSORS_ADC
      #define ENABLE_OPENBMC_DBUS_SENSORS_EXTERNAL
      #define ENABLE_OPENBMC_DBUS_SENSORS_FAN
      #define ENABLE_OPENBMC_DBUS_SENSORS_HWMON_TEMP
      #define ENABLE_OPENBMC_DBUS_SENSORS_INTELCPU
      #define ENABLE_OPENBMC_DBUS_SENSORS_INTRUSION
      #define ENABLE_OPENBMC_DBUS_SENSORS_PSU
      #define ENABLE_OPENBMC_ENTITY_MANAGER
      #define ENABLE_OPENBMC_FRU_DEVICE
      ......

   线程栈分配：[具体参考 openbmc_zephyr_app/app/src/task_def.hpp]
      大部分应用分配 32K，少数应用分配 16K/20K/64K

   线程优先级：
      dbus-broker优先级： [0]
      其他应用线程优先级： [4]

   依赖关系：
      dbus-broker 需要先于其他应用线程完成启动
      其他依赖关系参考：[openbmc_zephyr_app/app/src/main.cpp]

2. 应用入口函数：

   dbus_broker_main()         [openbmc_zephyr_app/app/src/dbroker.c]
   ipmid_main()               [phosphor-host-ipmid/ipmid-new.cpp]
   net_ipmid_main()           [phosphor-net-ipmid/net_ipmi_main.cpp]
   logging_main()             [phosphor-logging/log_manager_main.cpp]
   objmgr_main()              [phosphor-objmgr/src/main.cpp]
   entity_main()              [entity-manager/src/entity_manager.cpp]
   fru_main()                 [entity-manager/src/fru_device.cpp]
   adc_sensor_main()          [dbus-sensors/src/ADCSensorMain.cpp]
   external_sensor_main()     [dbus-sensors/src/ExternalSensorMain.cpp]
   fan_sensor_main()          [dbus-sensors/src/FanMain.cpp]
   hwmon_temp_sensor_main()   [dbus-sensors/src/HwmonTempMain.cpp]
   intel_cpu_sensor_main()    [dbus-sensors/src/IntelCPUSensorMain.cpp]
   intrusion_sensor_main()    [dbus-sensors/src/IntrusionSensorMain.cpp]
   psu_sensor_main()          [dbus-sensors/src/PSUSensorMain.cpp]
   kcsbridge_main()           [kcsbridge/src/main.cpp]
   busctl_main()              [modules/lib/basu/src/busctl/busctl.c]
   ipmi_main()                [ipmitool/lib/ipmi_main.c]
   ......
```

#### 二、应用目前可用的控制指令
```jsx
1. BMC shell
   除 kernel 相关指令外，BMC shell 还可以支持 busctl 指令和 ipmitool 指令， 例如：
   1) busctl 指令：
   busctl list
   busctl status
   busctl help
   busctl tree
   busctl monitor
   busctl tree org.freedesktop.DBus
   busctl introspect org.freedesktop.DBus /org/freedesktop/DBus
   busctl call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ListNames

   busctl tree xyz.openbmc_project.Ipmi.Channel.eth0
   busctl tree xyz.openbmc_project.Ipmi.Host
   busctl introspect xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi
   busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 6 0 4 0 0
   busctl call xyz.openbmc_project.Ipmi.Host /xyz/openbmc_project/Ipmi xyz.openbmc_project.Ipmi.Server execute yyyaya{sv} 6 0 1 0 0
   
   busctl tree xyz.openbmc_project.Ipmi.Channel.ipmi_kcs3
   busctl introspect xyz.openbmc_project.Ipmi.Channel.ipmi_kcs3 /xyz/openbmc_project/Ipmi/Channel/ipmi_kcs3
   busctl call xyz.openbmc_project.Ipmi.Channel.ipmi_kcs3 /xyz/openbmc_project/Ipmi/Channel/ipmi_kcs3 xyz.openbmc_project.Ipmi.Channel.SMS clearAttention
   busctl call xyz.openbmc_project.Ipmi.Channel.ipmi_kcs3 /xyz/openbmc_project/Ipmi/Channel/ipmi_kcs3 xyz.openbmc_project.Ipmi.Channel.SMS setAttention
   busctl call xyz.openbmc_project.Ipmi.Channel.ipmi_kcs3 /xyz/openbmc_project/Ipmi/Channel/ipmi_kcs3 xyz.openbmc_project.Ipmi.Channel.SMS forceAbort
   
   busctl tree xyz.openbmc_project.HwmonTempSensor
   busctl get-property xyz.openbmc_project.HwmonTempSensor /xyz/openbmc_project/sensors/temperature/fake_temp xyz.openbmc_project.Sensor.Value Value
   
   busctl tree xyz.openbmc_project.ExternalSensor
   busctl introspect xyz.openbmc_project.ExternalSensor /xyz/openbmc_project/sensors/temperature/TestExternalSensor xyz.openbmc_project.Sensor.Value
   busctl set-property xyz.openbmc_project.ExternalSensor /xyz/openbmc_project/sensors/temperature/TestExternalSensor xyz.openbmc_project.Sensor.Value Value d 25.6
   busctl get-property xyz.openbmc_project.ExternalSensor /xyz/openbmc_project/sensors/temperature/TestExternalSensor xyz.openbmc_project.Sensor.Value Value
   
   busctl tree xyz.openbmc_project.PSUSensor
   busctl introspect xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/voltage/fake_psu_Input_Voltage
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/voltage/Input_Voltage_1  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/voltage/fake_psu_Input_Voltage  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/voltage/fake_psu_Output_Voltage  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/power/fake_psu_Input_Power  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/power/fake_psu_Output_Power  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/current/fake_psu_Output_Current  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/fan_tach/fake_psu_Fan_Speed_1  xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.PSUSensor /xyz/openbmc_project/sensors/temperature/fake_psu_Temperature  xyz.openbmc_project.Sensor.Value Value
   
   busctl tree xyz.openbmc_project.IntrusionSensor
   busctl get-property xyz.openbmc_project.IntrusionSensor /xyz/openbmc_project/Chassis/Intrusion xyz.openbmc_project.Chassis.Intrusion Status
   busctl set-property xyz.openbmc_project.IntrusionSensor /xyz/openbmc_project/Chassis/Intrusion xyz.openbmc_project.Chassis.Intrusion Status s "Normal"
   
   busctl tree xyz.openbmc_project.IntelCPUSensor
   busctl introspect xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/Margin_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DIMM_A1_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DIMM_B1_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/power/Cpu_Power_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/power/Dimm_Power_CPU0 xyz.openbmc_project.Sensor.Value Value
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0 xyz.openbmc_project.Sensor.Threshold.Critical CriticalHigh
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0 xyz.openbmc_project.Sensor.Threshold.Critical CriticalAlarmHigh
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0 xyz.openbmc_project.State.Decorator.OperationalStatus Functional
   busctl get-property xyz.openbmc_project.IntelCPUSensor /xyz/openbmc_project/sensors/temperature/DTS_CPU0 xyz.openbmc_project.State.Decorator.Availability Available

   busctl tree xyz.openbmc_project.FanSensor
   busctl introspect xyz.openbmc_project.FanSensor  /xyz/openbmc_project/control
   busctl introspect xyz.openbmc_project.FanSensor  /xyz/openbmc_project/inventory
   busctl introspect xyz.openbmc_project.FanSensor /xyz/openbmc_project/sensors

   busctl tree xyz.openbmc_project.ADCSensor
   busctl introspect xyz.openbmc_project.ADCSensor /xyz/openbmc_project/sensors/voltage/SYS_P1V8_AUX
   busctl introspect xyz.openbmc_project.ADCSensor /xyz/openbmc_project/sensors/voltage/SYS_PVTT
   busctl introspect   xyz.openbmc_project.ADCSensor /xyz/openbmc_project/sensors/voltage/SYS_PVDDQ
   
   2）ipmitool 指令：
   ipmitool mc info
   ipmitool sel info
   ipmitool mc selftest
   ipmitool fru print
   ipmitool sensor list
   ipmitool sdr list
   ipmitool raw 6 1
   ipmitool raw 6 4
   ......
   此外，BMC shell 还支持 fs ls，fs cat 等文件系统指令。还有 kernel heap，kernel thread stacks 等内核查询指令。具体可在 BMC shell 中 "help"

2. 带外测试指令：
   带外安装 ipmitool 工具 [可以使用Linux或者windows版本] 使用带外指令进行相关的验证。

   连接网线，ping 通 ip (目前 IP 地址默认是 192.0.2.13，也可以使用 zephyr shell 指令调整 IP 地址 )

   参考指令 （目前支持的带外指令）： 
      
      查询 BMC 硬件信息
         ipmitool -I lanplus -H 192.0.2.13 -U admin -P bypass -C 17 mc info

      获取设备id
         ipmitool -I lanplus -H 192.0.2.13 -U admin -P bypass -C 17 raw 0x06 0x01

      获取自测结果
         ipmitool -I lanplus -H 192.0.2.13 -U admin -P bypass -C 17 raw 0x06 0x04

      其他
         ipmitool -I lanplus -H 192.0.2.13 -U admin -P bypass -C 17 raw 0x06 0x36

3. 使用 shell 查看 sensor 的 sysfs 文件
   > 查看 fan-sensor 的 pwm 和 fan 文件
      fs cat /sys/class/hwmon/hwmon1/fan1_input
      fs cat /sys/class/hwmon/hwmon2/fan2_input
      fs cat /sys/class/hwmon/hwmon1/pwm1
      fs cat /sys/class/hwmon/hwmon2/pwm2
   
   > 查询adc-sensor文件
      fs cat /sys/class/hwmon/hwmon0/in1_input

   > 查询其他 sensor 的文件方法相同
      查看目录：fs ls /sys/class/hwmon
      查看文件内容：fs cat /sys/class/hwmon/......
```
#### 三、应用已知问题和局限性
```jsx
1. host ipmid
   - 未启用PAM账号密码管理
   - chassishandler.cpp 中的全局变量 dbus，在需要时再去获取
   - stdplus：fd 部分没有参与编译
   - sd-journal 仅实现简单打印  
   - sd_id128_get_machine 的 ID 是设置固定的

2. net ipmid
   - 用户管理使用的应该是phosphor-user-manager，但目前尚未集成该模块，因此目前写死只要使用bypass密码，即可认为认证成功

3. entity manager
   - fru-device里面使用的是默认的 baseboard.fru.bin，探测 fru 跳过 /sys/bus/i2c/devices/i2c- 的链接文件采取 i2c 探测 fru 的方式
   - entity manager 里面 CurrentHostState() 暂时忽略，isPowerOnEntity() 返回 powerStatusOn

4. dbus sensors
   - dbus-sensors 目前未支持 4 类 sensor：机箱出风口（ExitAirTempSensor.cpp）、IPMB总线（IpmbSDRSensor.cpp、IpmbSensor.cpp）、BMC 自身 MCU/板载温度（MCUTempSensor.cpp）、NVMe 盘温度（NVMeBasicContext.cpp、NVMeSensor.cpp、NVMeSensorMain.cpp）
   - fan-sensor 没有接入物理风扇，所以用 pa08 和 pg02 代替 tach，并且要将 pn12 和 pn13 与其连接，如果要接入风扇需要修改 lsqsh_evb_cpu1.overlay 里面的 cap 和 hwmon1

5. kcs bridge
   - kcs bridge 目前实现了在 BMC 侧的功能，自测函数可以从 epsi/lpc 的共享内存中自注入测试指令，触发回调函数。后续使用 kcs 方式进行带内通信/管理的时候，还需要正确配置主机（host）侧的设备树（如epsi/lpc等），还要确保主机侧加载正确的驱动，并能根据 kcs 状态机触发回调函数等

6. HWMON
   - ADC sensor 会使用 hwmon_adc - class 2
   - Fan sensor 会使用 hwmon_pwm_tach - class 0
   - 其他 sensor (ExternalSensor, HwmonTempSensor, PSUSensor, IntrusionSensor, IntelCPUSensor) 会使用 hwmon_general - class 5
   - Hwmon 在设备树中的定义方法，请参考：zephyr/dts/bindings/hwmon/linkedsemi,hwmon.yaml
```

## dbus-broker

#### dbus-broker 是所有应用的核心依赖模块，其软件基本架构如下：
```jsx
D-BUS BROKER ARCHITECTURE (Zephyr Implementation)
│
├── 📦 APPLICATION LAYER (Client Interface)
│   ├── connect_to_dbroker() - Establish connection to broker
│   ├── disconnect_from_dbroker() - Clean disconnection
│   └── sd_bus API (from basu library)
│       ├── sd_bus_new() - Create bus object
│       ├── sd_bus_set_fd() - Set file descriptors
│       ├── sd_bus_start() - Initialize bus (Hello message)
│       ├── sd_bus_process() - Process messages
│       └── sd_bus_call()/sd_bus_call_async()- Send messages
│
├── 🔌 CONNECTION MANAGEMENT LAYER
│   │
│   └── 🔄 AF_UNIX Mode (CONFIG_NET_SOCKETS_AF_UNIX)
│       ├── socketpair(AF_UNIX, SOCK_STREAM, 0, g_controller_fds) - create socketpair for dbus controller only
│       ├── create_listener_socket() - socket() + bind() + listen() - create fd for listener socket, bind fd to address path, listen on fd
│       └── add_listener_to_broker() - allocate listener rbtree node, bind listener fd to listener_dispatch() function
│           └── listener_dispatch() - accept() client connection, peer_new_with_fd()
│               ├── accept() - accept client connection with a new connection fd
│               ├── peer_new_with_fd() - bind the connection fd with peer_dispatch() function
│               └── peer_spawn() - open peer connection, selects the events on dispatch file
│
├── 🏗️ DBROKER SUBSYSTEM (/zephyr/subsys/dbroker)
│   │
│   ├── dbroker.c - Main broker initialization
│   │   ├── SYS_INIT(dbus_broker_init) - Auto-initialization
│   │   ├── standard_broker_deployment() - Setup broker
│   │   ├── broker_thread_entry() - Dedicated broker thread
│   │   └── g_controller_fds[2] - Controller socketpair
│   │
│   └── include/dbus_broker.h - Public API declarations
│
├── 🎯 CORE BROKER ENGINE (/modules/lib/dbus-broker/src/broker)
│   │
│   ├── Broker (Main Structure)
│   │   ├── Log *log - Logging subsystem
│   │   ├── Bus bus - Message bus core
│   │   ├── DispatchContext dispatcher - Event dispatcher
│   │   ├── int signals_fd - Signal handling
│   │   └── Controller controller - Control interface
│   │
│   ├── broker_new() - Create broker instance
│   ├── broker_run() - Main event loop
│   ├── broker_free() - Cleanup
│   └── g_broker - Global broker pointer
│
├── 🎮 CONTROLLER LAYER (/modules/lib/dbus-broker/src/broker)
│   │
│   ├── Controller (Management Interface)
│   │   ├── Connection connection - Control socket
│   │   ├── CRBTree listeners - Listener registry
│   │   ├── CRBTree names - Name registry
│   │   ├── CRBTree metrics - Metrics collectors
│   │   └── Activation activation - Service activation
│   │
│   ├── controller.c - Controller implementation
│   │   ├── controller_add_listener() - Add TCP/socket listener
│   │   ├── controller_add_name() - Register service name
│   │   └── controller_reload_config() - Runtime config update
│   │
│   └── controller-dbus.c - D-Bus control interface
│       ├── org.bus1.DBus.Controller API
│       ├── AddListener() method
│       ├── RemoveListener() method
│       └── GetNameOwner() method
│
├── 🚌 BUS CORE (/modules/lib/dbus-broker/src/bus)
│   │
│   ├── Bus (Message Routing Core)
│   │   ├── PeerRegistry peers - Connected clients
│   │   ├── NameRegistry names - Service name database
│   │   ├── MatchRegistry matches - Signal subscriptions
│   │   └── ReplyRegistry replies - Method call tracking
│   │
│   ├── Peer Management (peer.c/h)
│   │   ├── struct Peer - Client representation
│   │   │   ├── Connection connection - Socket connection
│   │   │   ├── uint64_t id - Unique peer ID
│   │   │   ├── PolicySnapshot *policy - Access control
│   │   │   ├── NameOwner owned_names - Owned service names
│   │   │   ├── MatchRegistry sender_matches - Signal filters
│   │   │   └── ReplyRegistry replies - Pending replies
│   │   ├── peer_new_with_fd() - Create peer from socket
│   │   ├── peer_spawn() - Activate peer
│   │   ├── peer_register() - Register with bus
│   │   └── peer_unregister() - Remove peer
│   │
│   ├── Driver (driver.c/h) - Internal "org.freedesktop.DBus"
│   │   ├── Hello() - Initial handshake
│   │   ├── RequestName() - Claim service name
│   │   ├── AddMatch() - Subscribe to signals
│   │   ├── SendMessage() - Route messages
│   │   └── NameAcquired/NameLost signals
│   │
│   ├── Listener (listener.c/h) - Connection acceptor
│   │   ├── listener_new() - Create listener
│   │   └── listener_accept() - Accept new connections
│   │
│   ├── Name Registry (name.c/h) - Service name management
│   │   ├── NameOwner - Name ownership tracking
│   │   ├── name_acquire() - Claim name
│   │   ├── name_release() - Release name
│   │   └── name_get_owner() - Query owner
│   │
│   ├── Match Registry (match.c/h) - Signal subscription
│   │   ├── MatchRule - Filter rules
│   │   ├── match_add() - Add subscription
│   │   ├── match_remove() - Remove subscription
│   │   └── match_dispatch() - Deliver matching signals
│   │
│   ├── Policy Engine (policy.c/h) - Access control
│   │   ├── PolicySnapshot - Permission snapshot
│   │   ├── policy_check_send() - Verify send permission
│   │   └── policy_check_receive() - Verify receive permission
│   │
│   ├── Reply Tracking (reply.c/h) - Method call correlation
│   │   ├── ReplyEntry - Pending reply record
│   │   ├── reply_track() - Track outgoing call
│   │   └── reply_complete() - Match reply to call
│   │
│   └── Activation (activation.c/h) - Service auto-start
│       ├── Activation entry - Service definition
│       └── activation_trigger() - Start service on demand
│
├── 🔌 CONNECTION LAYER (/modules/lib/dbus-broker/src/dbus)
│   │
│   ├── Connection (connection.c/h) - Socket abstraction
│   │   ├── int fd - File descriptor
│   │   ├── Buffer read_buffer - Incoming data
│   │   ├── Buffer write_buffer - Outgoing data
│   │   ├── connection_read() - Receive messages
│   │   ├── connection_write() - Send messages
│   │   └── connection_dispatch() - I/O event handler
│   │
│   ├── Message (message.c/h) - D-Bus message format
│   │   ├── Header fields (type, serial, destination, etc.)
│   │   ├── Body payload
│   │   ├── message_seal() - Finalize message
│   │   └── message_parse() - Parse incoming message
│   │
│   └── Protocol handling
│       ├── Authentication (AUTH EXTERNAL)
│       ├── Capability negotiation
│       └── Error handling
│
├── 📚 BASU LIBRARY (/modules/lib/basu) - sd-bus Implementation
│   │
│   ├── src/libsystemd/sd-bus/ - Systemd D-Bus client library
│   │   ├── sd-bus.c - Main bus API
│   │   ├── sd-bus-message.c - Message construction/parsing
│   │   ├── sd-bus-slot.c - Resource management
│   │   └── sd-bus-track.c - Object tracking
│   │
│   ├── include/systemd/sd-bus.h - Public API headers
│   └── zephyr/ - Zephyr integration layer
│
├── 🛠️ UTILITY LAYER (/modules/lib/dbus-broker/src/util)
│   │
│   ├── dispatch.c/h - Event dispatch framework
│   │   ├── DispatchContext - Event loop context
│   │   ├── DispatchFile - File descriptor monitoring
│   │   └── dispatch_run() - Run event loop
│   │
│   ├── log.c/h - Logging subsystem
│   ├── user.c/h - User/group management
│   ├── string.c/h - String utilities
│   └── Various helpers (54 utility modules)
│
└── ⚙️ CONFIGURATION (Kconfig)
    ├── CONFIG_DBROKER - Enable D-Bus broker
    ├── CONFIG_DBUS_BROKER - Select dbus-broker library
    ├── DBUS_BROKER_THREAD_STACK_SIZE - dbus-broker thread stack size: 32KB
    ├── CONFIG_NET_SOCKETS_AF_UNIX - Enable AF_UNIX socket
    ├── CONFIG_NET_UNIX_BUFFER_SIZE - AF_UNIX buffer size: 2KB
    └── CONFIG_NET_UNIX_MAX_SOCKETS - AF_UNIX connection limit: 64
```
#### dbus-broker 用法
```jsx
应用应首先调用 connect_to_dbroker() API，完成与 dbus-broker 的连接，并拿到sd-bus句柄。应用退出前调用 disconnect_from_dbroker() API，断开与 dbus-broker 的连接，完成 socketpair 和 sd-bus 的资源释放。

example:
   应用入口：
      sd_bus* bus;
      connect_to_dbroker(&bus);

   应用退出前：
      disconnect_from_dbroker(bus);
```
#### 已知问题和局限性
```jsx
1. 本版本在 Zephyr 上增加了对 AF_UNIX 套接字通信的支持，包括 SOCK_STREAM 模式和 SOCK_DGRAM 模式。目前支持 64 个连接，每个连接的管道缓冲区大小是 2KB，采用静态数组的分配方式以避免内存碎片。dbus-broker与应用线程之间的通信已经切换为 AF_UNIX 连接，可以通过修改 CONFIG_NET_UNIX_MAX_SOCKETS 来调整连接数量，还可以通过修改 CONFIG_NET_UNIX_BUFFER_SIZE 改变缓冲区大小

2. 由于目前 dbus-broker 与应用线程并不需要对套接字连接路径做深层搜索和查询，所以套接字路径并未采用虚拟文件系统方式，而是直接采用一个路径字符串作为通信地址条目。类似的，也可以采用抽象命名空间的方式

3. 目前不支持 policy、signal fd、SO_PEERSEC、 SO_PEERGROUPS、 SELinux，AppArmor，and credential
```

## sd-event

#### basu库里没有sd-event，所以我们嵌入了sd-event的实现：
```jsx
sd-event (Zephyr Implementation)
│
├── 1. Core Event Loop Management
│   ├── sd_event_new() / sd_event_unref() - Lifecycle management
│   ├── sd_event_default() - Global default event loop
│   ├── sd_event_ref() / sd_event_unref() - Reference counting
│   ├── sd_event_get_fd() - Get monitoring FD (terminate_pipe)
│   ├── sd_event_get_state() - Query current state machine state
│   └── sd_event_set_dispatch_context() - Integrate with external dispatch (e.g., dbus-broker)
│
├── 2. State Machine & Iteration
│   ├── States: INITIAL → PREPARING → ARMED → PENDING → DISPATCHING
│   ├── sd_event_prepare() 
│   │   ├── Call prepare callbacks
│   │   ├── Process timer sources (check expiry)
│   │   └── Process deferred sources (mark as pending)
│   ├── sd_event_wait()
│   │   ├── Calculate timeout (based on timers or user input)
│   │   ├── Check for pre-existing pending sources
│   │   └── Block via dispatch_context_poll()
│   ├── sd_event_dispatch()
│   │   ├── Handle deferred sources (priority-sorted)
│   │   ├── Handle expired timers
│   │   └── Dispatch IO/Signal events via dispatch_context
│   ├── sd_event_run() - One-shot iteration (prepare + wait + dispatch)
│   └── sd_event_loop() - Infinite loop until exit requested
│
├── 3. Event Sources (Types)
│   ├── IO Sources (SOURCE_IO)
│   │   ├── sd_event_add_io() - Monitor file descriptors
│   │   ├── Integrated with DispatchFile (dbus-broker wrapper)
│   │   └── Supports EPOLLIN/EPOLLOUT etc.
│   ├── Timer Sources (SOURCE_TIME)
│   │   ├── sd_event_add_time() - Absolute/Relative timers
│   │   ├── Uses k_work_delayable for expiration logic
│   │   ├── Notification pipe to wake up poll loop
│   │   └── Supports accuracy and clock IDs (MONOTONIC, BOOTTIME, etc.)
│   ├── Signal Sources (SOURCE_SIGNAL) - Stubbed (Zephyr limitation)
│   ├── Child Process Sources (SOURCE_CHILD) - Stubbed (Zephyr limitation)
│   ├── Deferred Sources (SOURCE_DEFER)
│   │   ├── sd_event_add_defer() - Run once in next dispatch phase
│   │   └── Priority-sorted execution
│   ├── Post Sources (SOURCE_POST) - Run after dispatch
│   └── Exit Sources (SOURCE_EXIT) - Run before loop termination
│
├── 4. Source Configuration & Control
│   ├── sd_event_source_set_enabled() - ON / OFF / ONESHOT
│   ├── sd_event_source_set_priority() - Dynamic priority adjustment
│   ├── sd_event_source_set_userdata() - Context data passing
│   ├── sd_event_source_set_description() - Debugging labels
│   ├── sd_event_source_set_prepare() - Per-source prepare hook
│   └── sd_event_source_set_destroy_callback() - Cleanup hooks
│
├── 5. Termination & Watchdog
│   ├── sd_event_exit() - Request loop termination with exit code
│   ├── sd_event_get_exit_code() - Retrieve exit status
│   └── sd_event_set_watchdog() - Stubbed watchdog support
│
└── 6. Internal Architecture (Zephyr Mappings)
    ├── Polling: dispatch_context_poll() -> zsock_poll()
    ├── Timers: k_timer + k_work_delayable + socketpair notification
    ├── Locking: k_mutex for thread safety
    ├── Lists: CList for source management (sorted by priority)
    └── Memory: calloc/free with reference counting
```
#### Signal Sources、Child Process Sources在Zephyr中被Stubbed
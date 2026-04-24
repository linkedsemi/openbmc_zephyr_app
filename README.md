# **LS1020 OpenBMC 20260424 版本发布说明**

## 概述

#### 本次版本发布，在凌思微基板管理控制芯片（Lite-BMC） **LS1020** 上提供了基础的 OpenBMC 功能。用户可以使用ipmid工具等方式给 BMC 发送指令，执行指定操作或进行状态查询等任务。  

## 软件架构

#### OpenBMC软件架构由OpenBMC的上层业务应用层、核心中间件层、通用基础依赖层和安全运维层组成。各层包含的模块及其主要功能，以及各模块之间的依赖关系请参考如下面两张树形图：

##### OpenBMC 模块组成结构图
```ts
├─ 上层业务应用（用户功能层）
│  ├─ phosphor-host-ipmid       # 主机本地IPMI（KCS/BT接口）
│  ├─ phosphor-net-ipmid        # 远程网络IPMI（RMCP+）
│  ├─ phosphor-logging          # SEL/ELOG/Redfish事件日志
│  └─ phosphor-objmgr           # Phosphor Object Manager
├─ 核心中间件层（IPC+事件驱动）
│  ├─ phosphor-dbus-interfaces  # D-Bus接口YAML标准定义
│  ├─ sdbusplus                 # C++ D-Bus封装（生成工具+客户端/服务端）
│  ├─ D-Bus协议栈
│  │  └─ basu                   # 无systemd精简版sd-bus（兼容API）
│  ├─ 事件循环核心
│  │  └─ sd-eventplus           # 异步IO/定时器/信号/子进程管理
│  └─ 总线守护进程
│     └─ dbus-broker            # 高性能D-Bus路由（替代dbus-daemon）
├─ 通用基础依赖层（全模块共用）
│  ├─ C++通用工具库
│  │  ├─ boost                  # 异步IO/线程/容器/序列化
│  │  ├─ stdplus                # C++标准库增强（嵌入式适配）
│  │  ├─ function2              # 高效函数绑定/回调
│  │  ├─ fmt                    # 日志/字符串格式化
│  │  ├─ nlohmann_json          # JSON解析/序列化
│  │  └─ tinyxml2               # 轻量XML解析
│  ├─ RTOS&系统抽象
│  │  ├─ zephyr                 # Zephyr RTOS内核
│  │  ├─ zephyr-osal            # 跨OS抽象层（Linux/Zephyr适配）
│  │  └─ zephyr-devfs           # Zephyr设备文件系统抽象
│  └─ 芯片平台BSP
│     ├─ linkedsemi             # 凌思微BMC SoC驱动BSP
│     └─ ls_sdk                 # LS系列芯片底层 SDK
└─ 安全&运维层（底层支撑）
   ├─ 加密通信
   │  ├─ mbedtls                # 轻量嵌入式TLS（HTTPS/IPMI加密）
   │  └─ wolfssl                # 高性能TLS（国密/FIPS/量子安全）
   ├─ 安全远程运维
   │  └─ wolfssh                # 嵌入式SSH/SOL/SFTP
   ├─ 代码安全
   │  └─ safeclib               # C标准库安全加固（防缓冲区溢出）
   └─ 硬件调试
      └─ jtag_asd               # BMC远程JTAG调试（CPU/BIOS排障）
```
##### OpenBMC 核心业务模块依赖关系图
```ts
├─ phosphor-host-ipmid  # 顶层业务：主机本地IPMI
│  ├─ sdbusplus          # D-Bus C++封装（核心通信）
│  │  ├─ nlohmann_json   # JSON解析（数据序列化）
│  │  └─ basu            # 精简sd-bus协议栈（底层通信）
│  ├─ sdeventplus        # 事件循环封装（异步IO/定时器）
│  │  ├─ sdbusplus       # 依赖D-Bus通信
│  │  │  ├─ nlohmann_json
│  │  │  └─ basu
│  │  └─ stdplus         # C++标准库增强
│  │     ├─ function2    # 函数绑定/回调
│  │     └─ fmt          # 字符串/日志格式化
│  ├─ phosphor-dbus-interfaces  # D-Bus接口标准定义
│  │  └─ sdbusplus       # 依赖D-Bus封装
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
│  ├─ nlohmann_json      # 直接依赖：JSON数据处理
│  └─ boost              # 直接依赖：通用工具库（序列化/容器）
└─ phosphor-net-ipmid   # 顶层业务：远程网络IPMI
   ├─ boost              # 通用工具支撑
   ├─ phosphor-dbus-interfaces  # D-Bus接口标准定义
   │  └─ sdbusplus
   │     ├─ nlohmann_json
   │     └─ basu
   ├─ phosphor-logging   # 故障日志上报
   │  ├─ nlohmann_json
   │  ├─ phosphor-dbus-interfaces
   │  ├─ sdeventplus
   │  └─ sdbusplus
   ├─ phosphor-objmgr    # D-Bus对象管理（专属依赖）
   └─ sdbusplus          # D-Bus C++封装
      ├─ nlohmann_json
      └─ basu
```
#### 各模块代码仓库拉取的地址，详见项目启动仓库中的 west.yml

## 环境搭建

官方文档请参考：[https://docs.zephyrproject.org/latest/develop/getting_started/index.html]

#### 一、需要下载交叉编译工具链
[https://github.com/linkedsemi/xuantie-900-gcc-elf-newlib-x86_64-v3.0.1/tree/zephyr]

```jsx
指定交叉编译器
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
   mkdir  <你的path>/zephyr_work [工作区目录]
   cd     <你的path>/zephyr_work [工作区目录]
   git clone -b dbus-ipmi-v1 git@github.com:linkedsemi/openbmc_zephyr_app.git 或
   git clone -b dbus-ipmi-v1 https://github.com/linkedsemi/openbmc_zephyr_app.git
```

3. 初始化仓库
```jsx
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
   > 使用 `env |grep zephyr` 查看环境变量：
      ZEPHYR_BASE=[工作区目录]/zephyr
      BOARD_ROOT=[工作区目录]/zephyr
      PWD=[工作区目录]/linkedsemi_zephyr_project
      VIRTUAL_ENV=[工作区目录]/openbmc_zephyr_app/.venv

   > 使用 `env |grep com` 和 `env |grep COM` 查看交叉编译器的环境变量：
      ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      CROSS_COMPILE=<你的path>/Xuantie-900-gcc-elf-newlib-x86_64-V3.0.1/bin/riscv64-unknown-elf-
```

#### 三、编译和烧录
```jsx
   > 编译openbmc：
      cd [工作区目录]
      west build -p auto -b lsqsh_evb@2os_cpu1_xip/lsqsh/cpu1 openbmc_zephyr_app/app

   > 烧录openbmc：
      使用FlashProgrammer或flash命令，烧录[工作区目录]/build/zephyr/下编译生成的zephyr_flash_bram_xip_lsqsh_evb_2os_cpu1_xip_lsqsh_cpu1.bin
```

## ipmid应用使用说明

#### 一、host ipmid
```jsx
1. host启动配置

   编译宏：       [prj.conf]
      CONFIG_OPENBMC_PHOSPHOR_HOST_IPMID=y

   启动线程控制宏：[task_enable.hpp]
      #define ENABLE_OPENBMC_PHOSPHOR_HOST_IPMID

   线程栈分配：    [task_def.hpp]
      IPMI_THREAD_STACK_SIZE - 32K
      IPMID_TEST_STACK_SIZE  - 32K

   线程优先级：
      ipmid_main 线程优先级： 继承创建线程的优先级     [0]
      ipmid_test 线程优先级： CONFIG_THREAD_PRI_TEST [5]

   依赖关系：
      dbus-broker 需要先于 host ipmid 完成启动

2. 应用入口函数：

   ipmid_main()   [phosphor-host-ipmid/ipmid-new.cpp]

3. 测试方法：

   ipmid_test()   [phosphor-host-ipmid/zephyr/test.cpp]
      指令参数：
         uint8_t netFn = 6;
         uint8_t lun = 0;
         uint8_t cmd = 4;
      调用host server的 ipmiAppGetSelfTestResults()
   
   目前 host ipmid 应用启动后会自动执行20次 self test 测试，并返回测试结果

4. 已知问题：

   - 未启用PAM账号密码管理

   - 放行 execute 方法调用的权限通道检查

   - chassishandler.cpp 中的全局变量 dbus，在需要时再去获取

   - stdplus：fd 部分没有参与编译

   - sd-journal 未实现  

   - sd_id128_get_machine 的 ID 是设置固定的
```

#### 二、net ipmid
```jsx
1. net启动配置

   编译宏：       [prj.conf]
      CONFIG_OPENBMC_PHOSPHOR_NET_IPMID=y

   启动线程控制宏：[task_enable.hpp]
      #define ENABLE_OPENBMC_PHOSPHOR_NET_IPMID

   线程栈分配：    [task_def.hpp]
      NET_IPMID_THREAD_STACK_SIZE     - 64K
      NET_STACK_CFG_THREAD_STACK_SIZE - 16K

   线程优先级：
      net_ipmid_main 线程优先级： 继承创建线程的优先级     [0]
      net_stack_cfg  线程优先级： 继承创建线程的优先级     [0]

   依赖关系：
      dbus-broker、host ipmid 和 net_stack_cfg 需要先于 net ipmid 完成启动

2. 应用入口函数：

   net_ipmid_main()   [phosphor-net-ipmid/net_ipmi_main.cpp]

3. 测试方法：

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

4. 已知问题：

   - 由于目前缺少文件系统，net-ipmid 无法存储 cipher_list.json，目前看这会导致这些函数会根据会话协商的算法类型（如 HMAC_SHA1_96 或  AES_CBC_128）来创建对应的加解密对象。没有配置文件，系统无法确定该创建哪种对象，导致后续的业务报文（如 payloadType=0 的 IPMI 命令）因无法解密或校验失败而被丢弃。目前写死了支持SHA256即-C17使用的验证算法
   
   - 用户管理使用的应该是phosphor-user-manager，但目前尚未集成该模块，因此目前写死只要使用bypass密码，即可认为认证成功
   
   - 由于上面写死的一些更改以及目前lg2日志模块没有集成，导致目前时序可能受到影响，看到的现象是，如果调试时加入 printf 打印，则 rakp3 包能正常收到，不抛异常，整个 rcmp 的校验流程可以实现。但如果不加入，某些 ipmitool 工具在建立会话时，存在时序问题，ipmitool 在协商了非 NONE 的 integrity 后，会在 RAKP3 的 payloadType 上置 0x40 。BMC 在 unflatten 中需调用 getIntegrityAlgo() 校验，但完整性算法对象在 RAKP34 末尾的 applyIntegrityAlgo() 里晚于 RAKP3 入栈解析，导致 getIntegrityAlgo() 抛 Integrity Algorithm Empty（或等效失败），receive() 在到达 RAKP34 之前就返回。目前调整了 net-ipmid 的代码改了一下时序即这个对象的创建移动到了 rakp2 回包的时候（此时本地已具备校验的全部要素），而不是在rakp34的末端，暂时消除了这个问题，目前该问题还需要深入研究。
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
│   ├── 🔄 Socketpool Mode (CONFIG_DBUS_BROKER_SOCKETPOOL)
│   │   ├── socketpool_init() - Pre-allocate socketpairs as a pool
│   │   ├── socketpool_allocate() - Get client_fd & broker_fd from the pool
│   │   ├── socketpool_add_peer_to_broker() - Register peer
│   │   └── socketpool_free() - Return to pool
│   │
│   └── 🌐 INET Socket Mode (CONFIG_DBUS_BROKER_INET)    [obsolete]
│       ├── create_listener_socket_with_retry() - TCP listener
│       ├── bind(127.0.0.1:55555) - Listen on localhost
│       └── accept() - Accept incoming connections
│
├── 🏗️ DBROKER SUBSYSTEM (/zephyr/subsys/dbroker)
│   │
│   ├── dbroker.c - Main broker initialization
│   │   ├── SYS_INIT(dbus_broker_init) - Auto-initialization
│   │   ├── standard_broker_deployment() - Setup broker
│   │   ├── broker_thread_entry() - Dedicated broker thread
│   │   └── g_controller_fds[2] - Controller socketpair
│   │
│   ├── socketpool.c - Socketpair pool management
│   │   ├── struct socketpool_entry[] - Pool entries
│   │   ├── k_mutex lock - Thread-safe allocation
│   │   └── CONFIG_DBUS_BROKER_SOCKETPOOL_SIZE (default: 16)
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
    ├── CONFIG_DBUS_BROKER_SOCKETPOOL vs INET - Connection mode
    ├── CONFIG_DBUS_BROKER_SOCKETPOOL_SIZE - Pool size (default: 16)
    ├── CONFIG_DBUS_BROKER_SOCKETPOOL_BUFFER_SIZE - Buffer (default: 4096)
    ├── CONFIG_DBUS_BROKER_STACK_SIZE - Thread stack (default: 8192)
    └── CONFIG_DBUS_BROKER_PRIORITY - Thread priority (default: 7)
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
1. 由于 Zephyr 对 Named Unix Domain Socket 的支持还不够充分，而 INET socket 模式需要大量网络栈资源开销，我们最终选择了轻量级的无名套接字（ Unamed Unix Domain Socket ）模式。使用socketpair() 创建一对 socket pair，一端作为客户端，另一端作为服务器端，进行通信。

2. socketpair 目前预分配16个 socketpair，形成一个 socketpair pool，后续可以根据需要调整。每个应用连接 broker 时，会从 socketpair pool 中分配一个 socketpair。应用结束时应调用 disconnect_from_dbroker() 释放socketpair 资源。

3. 在对 socketpair 的某一端进行 pollout 或者 write 时，也会去拿远端的 semaphore，这可能会对同一时间在另一端的操作会造成阻塞，造成一定程度的性能损失。

4. 目前不支持 policy、signal fd、SO_PEERSEC、 SO_PEERGROUPS、 SELinux，AppArmor，and credential
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
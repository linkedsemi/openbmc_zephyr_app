# D-Bus Broker Test Framework

## 概述

这是一个用于在 Zephyr RTOS 上测试 dbus-broker 功能的框架。框架采用多线程架构，将 broker 运行与测试代码分离。

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Test Application                      │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────┐  ┌────────────────┐ │
│  │   Broker Thread (Priority 7) │  │ Controller    │ │
│  │   - broker_run()           │  │ Thread (Pri 6)│ │
│  │   - dispatch_context_dispatch│  │              │ │
│  │   - 事件循环               │  │ - 测试框架   │ │
│  └─────────────────────────────┘  │              │ │
│           │                      │ - run_all_tests() │ │
│           │ g_broker              └────────────────┘ │
│           ▼                                          │
│  ┌──────────────────────────────────────────────────────┐ │
│  │         dbus-broker Library                        │ │
│  │  ┌──────────────────────────────────────────────┐  │ │
│  │  │ Broker (g_broker)                       │  │ │
│  │  │  - Bus                                   │  │ │
│  │  │  - DispatchContext                        │  │ │
│  │  │  - Controller                            │  │ │
│  │  └──────────────────────────────────────────────┘  │ │
│  └──────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## 文件结构

```
test_broker/
├── src/
│   ├── main.c           # 主程序，线程创建
│   ├── test_dbus.h      # 测试框架头文件
│   └── test_dbus.c      # 测试实现
├── CMakeLists.txt
└── README.md
```

## 测试阶段

测试分为 5 个阶段，逐步验证 broker 功能：

### Phase 1: Broker Ready ✅ 已实现
- 验证全局 broker 指针已设置
- 验证 broker 线程正在运行

### Phase 2: Create Listener ⚠️ 待实现
- 通过 D-Bus controller 协议创建 listener socket
- 需要实现：
  - 连接到 broker 的 controller socket
  - SASL 认证 (EXTERNAL)
  - 发送 AddListener 方法调用

### Phase 3: Peer Connection ⚠️ 待实现
- 连接到 listener socket
- 需要：
  - 连接到 listener 地址
  - SASL 认证
  - 发送 Hello 消息

### Phase 4: D-Bus Messages ⚠️ 待实现
- 发送和接收 D-Bus 消息
- 需要：
  - 发送 method call 消息
  - 接收 method return 消息
  - 验证消息格式

### Phase 5: Protocol Validation ⚠️ 待实现
- 完整的 D-Bus 协议验证
- 需要：
  - 验证所有消息类型
  - 验证消息路由
  - 验证 name 注册

## 使用方法

### 编译

```bash
cd /home/jory/zephyrproject/jory_workspace
west build -b <board> samples/test_broker
```

### 运行

```bash
west flash
west attach
```

### 预期输出

```
[00:00:00.000] <inf> TEST_BROKER: ========================================
[00:00:00.000] <inf> TEST_BROKER:  D-Bus Broker Test Framework
[00:00:00.000] <inf> TEST_BROKER: ========================================
[00:00:00.000] <inf> TEST_BROKER: 
[00:00:00.000] <inf> TEST_BROKER: Threads created:
[00:00:00.000] <inf> TEST_BROKER:   - Broker thread: running dbus-broker main loop
[00:00:00.000] <inf> TEST_BROKER:   - Controller thread: running tests
[00:00:00.000] <inf> TEST_BROKER: 
[00:00:00.010] <inf> TEST_BROKER: [Broker Thread] Starting dbus-broker...
[00:00:00.100] <inf> TEST_BROKER: [Controller Thread] Broker ready, initializing tests...
[00:00:00.100] <inf> TEST_DBUS: ========== Starting D-Bus Broker Tests ==========
[00:00:00.100] <dbg> TEST_DBUS: Phase 1: Verifying broker is ready
[00:00:00.100] <dbg> TEST_DBUS: Broker is ready at 0x...
[00:00:00.100] <wrn> TEST_DBUS: Phase 2 not yet implemented - skipping
[00:00:00.100] <wrn> TEST_DBUS: Phase 3 not yet implemented - skipping
[00:00:00.100] <wrn> TEST_DBUS: Phase 4 not yet implemented - skipping
[00:00:00.100] <wrn> TEST_DBUS: Phase 5 not yet implemented - skipping
[00:00:00.100] <inf> TEST_BROKER: [Controller Thread] Tests completed
```

## 下一步

### 实现 Phase 2: Create Listener

1. **创建 socket 连接**
   ```c
   int fd = socket(AF_UNIX, SOCK_STREAM, 0);
   connect(fd, controller_addr);
   ```

2. **SASL 认证**
   - 发送 AUTH EXTERNAL 命令
   - 接收 OK 响应
   - 发送 BEGIN 命令

3. **发送 AddListener 方法调用**
   - 构造 D-Bus 消息
   - 通过 SCM_RIGHTS 传递 listener fd
   - 接收 Method Return

### 实现工具函数

建议创建以下辅助文件：
- `sasl_auth.c/h` - SASL 认证实现
- `dbus_message.c/h` - D-Bus 消息构造和解析
- `controller_client.c/h` - D-Bus controller 协议客户端

## 调试

启用详细日志：

``c
LOG_MODULE_REGISTER(TEST_BROKER, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(TEST_DBUS, LOG_LEVEL_DBG);
```

## 注意事项

1. **线程优先级**：Controller 线程 (6) 高于 Broker 线程 (7)，确保测试能及时响应
2. **栈大小**：8192 字节应该足够，如有栈溢出需要增加
3. **全局指针**：`g_broker` 在 broker 线程中设置，controller 线程需要等待其初始化

# Test Broker - D-Bus Broker Implementation Tests

## Overview

This sample tests the D-Bus broker implementation in Zephyr with concurrent stress testing.

## Features

- **D-Bus Broker Testing**: Verifies broker deployment, controller connections, and message routing
- **Concurrent SD-Event Tests**: Runs comprehensive sd-event test suite simultaneously
- **Stress Testing**: Shared dispatch_context is tested under concurrent load from both subsystems

## Thread Architecture

```
Main Thread (infinite loop)
├─ Broker Thread (priority 7) - Standard broker_run()
├─ Service Thread (priority 6) - Name service provider  
├─ Test Thread (priority 6) - D-Bus functionality tests
└─ SD-Event Test Thread (priority 5) - Event loop stress tests ⭐ NEW!
```

## Concurrent Stress Testing Design

The **sd-event test suite** runs concurrently with dbus-broker tests to provide:

1. **DispatchContext Pressure**: Both subsystems use the same underlying dispatch_context
2. **Resource Competition**: Tests FD management, memory allocation, and lock contention
3. **Real-world Simulation**: Actual applications run dbus and event loops together

## Running the Tests

Build and flash as usual:

```bash
west build -b <your_board> samples/test_broker
west flash
```

Watch for test output in two phases:

1. **SD-Event Tests** (concurrent):
   ```
   ========================================
   SD-Event Test Suite Starting
   Running concurrently with dbus-broker tests
   This provides stress testing for dispatch_context
   ========================================
   ✓ All SD-Event tests PASSED!
   ```

2. **D-Bus Broker Tests** (main test thread):
   ```
   [Test Thread] Broker is ready!
   [Test Thread] Deployment verification passed!
   ```

## Test Coverage

### SD-Event Tests (20 test cases):
1. Basic Create
2. IO Handler
3. Timer Handler
4. Deferred Handler
5. Exit Handler
6. Multiple Sources
7. Priority
8. Cleanup
9. Dynamic Priority Adjustment
10. Enable/Disable State Transition
11. Timer Accuracy
12. Concurrent IO Events
13. Nested Event Loop Operations
14. Resource Leak Detection
15. Boundary Conditions
16. Stable Sorting
17. Timer Multiple Expirations
18. State Transitions
19. IO Edge/Level Trigger
20. Stress Test (many sources)

### D-Bus Broker Tests:
- Controller connection handling
- Message routing
- Name service operations
- Client communication

## Configuration

Edit `prj.conf` to adjust:

- `CONFIG_LOG_DEFAULT_LEVEL`: Increase to DEBUG for verbose output
- `CONFIG_LOG_BUFFER_SIZE`: Increase if messages are dropped
- `CONFIG_SD_EVENT_STANDALONE`: (future) Run sd-event tests separately

## Debugging Tips

If tests fail:

1. Check log output for assertion failures
2. Verify dispatch_context initialization
3. Look for resource leaks (FDs, memory)
4. Monitor stack usage (increase if needed)

## Architecture Notes

The concurrent design intentionally stresses the shared dispatch_context:

- **dbus-broker** creates its own dispatcher instance
- **sd-event** creates separate event instances per test
- Both use the same underlying `DispatchContext` from dbus-broker
- Concurrent execution exposes race conditions and resource conflicts

This validates the robustness of the dispatch infrastructure under realistic conditions.

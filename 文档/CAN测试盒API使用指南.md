# CAN测试盒专业API使用指南

## 概述

本文档详细介绍了CAN测试盒专业API的使用方法，包括接口位置、使用示例和最佳实践。该API提供了完整的CAN报文收发功能，适用于专业的CAN总线测试和验证工作。

## 文件结构

### 核心文件位置

```
Core/
├── Inc/
│   └── can_testbox_api.h          # API接口头文件
└── Src/
    ├── can_testbox_api.c           # API接口实现文件
    └── can_testbox_example.c       # 使用示例文件
```

### 文件说明

- **can_testbox_api.h**: 包含所有API接口声明、数据结构定义和配置宏
- **can_testbox_api.c**: API接口的具体实现代码
- **can_testbox_example.c**: 详细的使用示例和最佳实践

## API接口分类

### 1. 单帧事件报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第133-155行)
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrame(const CAN_TestBox_Message_t *message);
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrameQuick(uint32_t id, uint8_t dlc, const uint8_t *data, bool is_extended);
```

#### 使用方法
```c
// 方法1: 使用完整结构体
CAN_TestBox_Message_t msg = {
    .id = 0x123,
    .dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
    .is_extended = false,
    .is_remote = false
};
CAN_TestBox_SendSingleFrame(&msg);

// 方法2: 快速接口
uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
CAN_TestBox_SendSingleFrameQuick(0x456, 4, data, false);
```

### 2. 单帧循环报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第157-200行)
CAN_TestBox_Status_t CAN_TestBox_StartPeriodicMessage(const CAN_TestBox_Message_t *message, uint32_t period_ms, uint8_t *handle_id);
CAN_TestBox_Status_t CAN_TestBox_StopPeriodicMessage(uint8_t handle_id);
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicPeriod(uint8_t handle_id, uint32_t new_period_ms);
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicData(uint8_t handle_id, const uint8_t *new_data, uint8_t dlc);
CAN_TestBox_Status_t CAN_TestBox_StopAllPeriodicMessages(void);
```

#### 使用方法
```c
// 启动周期性消息
uint8_t handle_id;
CAN_TestBox_Message_t periodic_msg = {
    .id = 0x100,
    .dlc = 8,
    .data = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80}
};
CAN_TestBox_StartPeriodicMessage(&periodic_msg, CAN_TESTBOX_PERIOD_100MS, &handle_id);

// 修改发送周期
CAN_TestBox_ModifyPeriodicPeriod(handle_id, CAN_TESTBOX_PERIOD_200MS);

// 修改数据内容
uint8_t new_data[] = {0x99, 0x88, 0x77, 0x66};
CAN_TestBox_ModifyPeriodicData(handle_id, new_data, 4);

// 停止周期性消息
CAN_TestBox_StopPeriodicMessage(handle_id);
```

#### 可用周期配置宏
```c
#define CAN_TESTBOX_PERIOD_1MS      1
#define CAN_TESTBOX_PERIOD_5MS      5
#define CAN_TESTBOX_PERIOD_10MS     10
#define CAN_TESTBOX_PERIOD_20MS     20
#define CAN_TESTBOX_PERIOD_50MS     50
#define CAN_TESTBOX_PERIOD_100MS    100
#define CAN_TESTBOX_PERIOD_200MS    200
#define CAN_TESTBOX_PERIOD_500MS    500
#define CAN_TESTBOX_PERIOD_1000MS   1000
#define CAN_TESTBOX_PERIOD_2000MS   2000
#define CAN_TESTBOX_PERIOD_5000MS   5000
```

### 3. 连续帧报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第202-230行)
CAN_TestBox_Status_t CAN_TestBox_SendBurstFrames(const CAN_TestBox_BurstMsg_t *burst_config);
CAN_TestBox_Status_t CAN_TestBox_SendBurstFramesQuick(uint32_t id, uint8_t dlc, const uint8_t *data, 
                                                      uint16_t burst_count, uint16_t interval_ms, bool auto_increment_id);
```

#### 使用方法
```c
// 方法1: 使用完整配置
CAN_TestBox_BurstMsg_t burst_config = {
    .message = {
        .id = 0x400,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    },
    .burst_count = 10,
    .interval_ms = CAN_TESTBOX_INTERVAL_5MS,
    .auto_increment_id = true,
    .auto_increment_data = false
};
CAN_TestBox_SendBurstFrames(&burst_config);

// 方法2: 快速接口
uint8_t burst_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
CAN_TestBox_SendBurstFramesQuick(0x500, 4, burst_data, 5, CAN_TESTBOX_INTERVAL_10MS, true);
```

#### 可用间隔配置宏
```c
#define CAN_TESTBOX_INTERVAL_0MS    0    // 无间隔
#define CAN_TESTBOX_INTERVAL_1MS    1
#define CAN_TESTBOX_INTERVAL_2MS    2
#define CAN_TESTBOX_INTERVAL_5MS    5
#define CAN_TESTBOX_INTERVAL_10MS   10
#define CAN_TESTBOX_INTERVAL_20MS   20
#define CAN_TESTBOX_INTERVAL_50MS   50
#define CAN_TESTBOX_INTERVAL_100MS  100
```

## 报文发送队列设计

### 队列配置
```c
// 文件: Core/Inc/can_testbox_api.h (第45-49行)
#define CAN_TESTBOX_SEND_QUEUE_SIZE     50    // 发送队列大小
#define CAN_TESTBOX_RECEIVE_QUEUE_SIZE  100   // 接收队列大小
#define CAN_TESTBOX_MAX_PERIODIC_MSGS   20    // 最大周期消息数量
```

### 队列管理特性
- **发送队列**: 支持50个待发送消息的缓存
- **接收队列**: 支持100个接收消息的缓存
- **周期性消息**: 最多支持20个并发的周期性消息
- **自动管理**: 队列满时自动丢弃最旧的消息

## 完整功能列表

### 已实现的核心功能

1. **消息发送功能**
   - 单帧事件报文发送
   - 周期性报文发送
   - 连续帧报文发送
   - 扩展帧和标准帧支持
   - 远程帧支持

2. **消息接收功能**
   - 中断驱动接收
   - 接收队列管理
   - 回调函数支持
   - 消息过滤

3. **配置管理功能**
   - 波特率配置
   - 工作模式配置
   - 过滤器管理
   - 启动/停止控制

4. **统计和诊断功能**
   - 发送/接收统计
   - 错误统计
   - 总线状态监控
   - 自检功能

### 建议补充的专业功能

#### 1. 高级测试功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_StartStressTest(uint32_t duration_ms, uint32_t load_percentage);
CAN_TestBox_Status_t CAN_TestBox_StartLatencyTest(uint32_t test_id, uint32_t timeout_ms);
CAN_TestBox_Status_t CAN_TestBox_StartThroughputTest(uint32_t duration_ms);
```

#### 2. 错误注入功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_InjectBusError(CAN_ErrorType_t error_type);
CAN_TestBox_Status_t CAN_TestBox_InjectFrameError(uint32_t frame_id, CAN_FrameErrorType_t error_type);
CAN_TestBox_Status_t CAN_TestBox_SimulateBusOff(uint32_t duration_ms);
```

#### 3. 数据记录功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_StartDataLogging(const char* filename);
CAN_TestBox_Status_t CAN_TestBox_StopDataLogging(void);
CAN_TestBox_Status_t CAN_TestBox_ExportStatistics(const char* filename);
```

#### 4. 网络层支持
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_SendISOTP(uint32_t id, const uint8_t* data, uint16_t length);
CAN_TestBox_Status_t CAN_TestBox_ReceiveISOTP(uint32_t id, uint8_t* data, uint16_t* length, uint32_t timeout_ms);
CAN_TestBox_Status_t CAN_TestBox_SendJ1939(uint32_t pgn, uint8_t sa, uint8_t da, const uint8_t* data, uint8_t length);
```

#### 5. 时间同步功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_EnableTimeSync(bool enable);
CAN_TestBox_Status_t CAN_TestBox_SetTimeReference(uint64_t reference_time_us);
CAN_TestBox_Status_t CAN_TestBox_GetPreciseTimestamp(uint64_t* timestamp_us);
```

## 使用示例

### 基本初始化
```c
#include "can_testbox_api.h"

void CAN_TestBox_Setup(void)
{
    // 1. 初始化CAN测试盒
    CAN_TestBox_Status_t status = CAN_TestBox_Init(&hcan1);
    if (status != CAN_TESTBOX_OK) {
        printf("CAN TestBox initialization failed: %d\r\n", status);
        return;
    }
    
    // 2. 设置接收回调
    CAN_TestBox_SetRxCallback(My_RxCallback);
    
    // 3. 启动测试盒
    CAN_TestBox_Enable(true);
    
    printf("CAN TestBox ready\r\n");
}
```

### 主循环集成
```c
void Main_Loop(void)
{
    while (1) {
        // 调用CAN测试盒主任务
        CAN_TestBox_Task();
        
        // 其他应用逻辑
        // ...
        
        // 延时
        HAL_Delay(1);
    }
}
```

### FreeRTOS任务集成
```c
void CAN_TestBox_Task_Thread(void *argument)
{
    // 初始化
    CAN_TestBox_Setup();
    
    for (;;) {
        // 调用CAN测试盒主任务
        CAN_TestBox_Task();
        
        // 任务延时
        osDelay(1);
    }
}
```

## 配置建议

### 1. 内存配置
- 建议堆栈大小: 至少2KB
- FreeRTOS任务优先级: osPriorityNormal或更高
- 队列内存: 约8KB (发送队列50 + 接收队列100) × 消息大小

### 2. 中断配置
- CAN接收中断优先级: 建议设置为较高优先级
- 确保CAN中断不被其他中断长时间阻塞

### 3. 时钟配置
- 确保CAN时钟配置正确
- 建议使用标准波特率: 125K, 250K, 500K, 1M

## 错误处理

### 常见错误码
```c
typedef enum {
    CAN_TESTBOX_OK = 0,             // 成功
    CAN_TESTBOX_ERROR,              // 一般错误
    CAN_TESTBOX_BUSY,               // 忙碌
    CAN_TESTBOX_TIMEOUT,            // 超时
    CAN_TESTBOX_INVALID_PARAM,      // 无效参数
    CAN_TESTBOX_QUEUE_FULL,         // 队列满
    CAN_TESTBOX_QUEUE_EMPTY,        // 队列空
    CAN_TESTBOX_NOT_INITIALIZED,    // 未初始化
    CAN_TESTBOX_ALREADY_EXISTS,     // 已存在
    CAN_TESTBOX_NOT_FOUND           // 未找到
} CAN_TestBox_Status_t;
```

### 错误处理建议
```c
CAN_TestBox_Status_t status = CAN_TestBox_SendSingleFrame(&msg);
switch (status) {
    case CAN_TESTBOX_OK:
        // 发送成功
        break;
    case CAN_TESTBOX_BUSY:
        // 总线忙，稍后重试
        HAL_Delay(1);
        break;
    case CAN_TESTBOX_INVALID_PARAM:
        // 参数错误，检查消息内容
        printf("Invalid message parameters\r\n");
        break;
    default:
        // 其他错误
        printf("Send failed: %d\r\n", status);
        break;
}
```

## 性能优化建议

1. **批量操作**: 使用连续帧接口而不是多次单帧发送
2. **队列管理**: 定期清理接收队列，避免内存泄漏
3. **中断优化**: 在接收回调中避免长时间操作
4. **统计监控**: 定期检查统计信息，及时发现问题

## PEPS专业测试接口

### 概述

PEPS（Passive Entry Passive Start，被动进入被动启动）专业测试接口是CAN测试盒API的高级功能模块，专门用于汽车PEPS系统的测试和验证。该接口集成了PEPS状态机功能，提供了完整的PEPS测试解决方案。

### 主要特性

- **完整的PEPS测试覆盖**：支持SCW1/SCW2唤醒、钥匙位置、BSI状态等测试
- **专业API接口**：标准化的函数接口，易于集成和使用
- **便捷测试工具**：提供快速测试、压力测试、循环测试等便捷功能
- **详细统计信息**：实时统计测试结果和性能数据
- **灵活配置**：支持自定义测试参数和数据

### 文件结构

```
Core/
├── Inc/
│   ├── can_testbox_api.h          # 主API头文件（包含PEPS接口）
│   └── can_testbox_peps_helper.h   # PEPS便捷接口头文件
└── Src/
    ├── can_testbox_api.c           # 主API实现（包含PEPS功能）
    └── can_testbox_peps_helper.c    # PEPS便捷接口实现
```

### PEPS核心API接口

#### 1. PEPS模块初始化

```c
/**
 * @brief 初始化PEPS测试模块
 * @retval CAN_TESTBOX_OK 初始化成功
 * @retval 其他 初始化失败
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_Init(void);
```

**使用示例：**
```c
// 初始化CAN测试盒
if (CAN_TestBox_Init() != CAN_TESTBOX_OK) {
    printf("CAN TestBox initialization failed\r\n");
    return;
}

// 初始化PEPS模块
if (CAN_TestBox_PEPS_Init() != CAN_TESTBOX_OK) {
    printf("PEPS module initialization failed\r\n");
    return;
}

printf("PEPS测试环境初始化完成\r\n");
```

#### 2. PEPS命令执行

```c
/**
 * @brief 执行PEPS命令
 * @param config PEPS测试配置
 * @param result 测试结果输出
 * @retval CAN_TESTBOX_OK 执行成功
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_ExecuteCommand(
    const CAN_TestBox_PEPS_Config_t *config, 
    CAN_TestBox_PEPS_Result_t *result);
```

**使用示例：**
```c
CAN_TestBox_PEPS_Config_t config = {
    .command = PEPS_CMD_SCW1_WAKEUP_START,
    .data = {0x10, 0x03, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00}
};

CAN_TestBox_PEPS_Result_t result;
CAN_TestBox_Status_t status = CAN_TestBox_PEPS_ExecuteCommand(&config, &result);

if (status == CAN_TESTBOX_OK) {
    printf("PEPS命令执行成功，发送帧数: %lu\r\n", result.frames_sent);
    printf("执行时间: %lu ms\r\n", result.execution_time_ms);
    printf("最终状态: %d\r\n", result.final_state);
}
```

#### 3. SCW1唤醒测试

```c
/**
 * @brief 启动SCW1 PEPS唤醒测试
 * @param data 自定义数据（NULL使用默认数据）
 * @param period_ms 发送周期（毫秒）
 * @retval CAN_TESTBOX_OK 启动成功
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartSCW1Wakeup(const uint8_t *data, uint32_t period_ms);

/**
 * @brief 停止SCW1 PEPS唤醒测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopSCW1Wakeup(void);
```

**使用示例：**
```c
// 使用默认数据启动SCW1唤醒测试
CAN_TestBox_PEPS_StartSCW1Wakeup(NULL, CAN_TESTBOX_PERIOD_200MS);
printf("SCW1唤醒测试已启动\r\n");

// 运行5秒
osDelay(5000);

// 停止测试
CAN_TestBox_PEPS_StopSCW1Wakeup();
printf("SCW1唤醒测试已停止\r\n");

// 使用自定义数据
uint8_t custom_data[8] = {0x10, 0x03, 0x3E, 0x01, 0x02, 0x03, 0x04, 0x05};
CAN_TestBox_PEPS_StartSCW1Wakeup(custom_data, CAN_TESTBOX_PERIOD_100MS);
```

#### 4. SCW2唤醒测试

```c
/**
 * @brief 启动SCW2 PEPS唤醒测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartSCW2Wakeup(const uint8_t *data, uint32_t period_ms);

/**
 * @brief 停止SCW2 PEPS唤醒测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopSCW2Wakeup(void);
```

#### 5. 钥匙位置测试

```c
/**
 * @brief 启动钥匙位置测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartKeyPosition(const uint8_t *data, uint32_t period_ms);

/**
 * @brief 停止钥匙位置测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopKeyPosition(void);
```

**钥匙位置数据格式：**
```c
// 钥匙位置数据结构（第5字节为位置信息）
uint8_t key_position_data[8] = {
    0x00, 0x00, 0x00, 0x00, 
    position_value,  // 位置值：0x00=无钥匙, 0x01=位置1, 0x02=位置2, etc.
    0x00, 0x00, 0x00
};
```

#### 6. BSI状态测试

```c
/**
 * @brief 启动BSI状态测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartBSIStatus(const uint8_t *data, uint32_t period_ms);

/**
 * @brief 停止BSI状态测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopBSIStatus(void);
```

#### 7. 统一控制接口

```c
/**
 * @brief 停止所有PEPS测试
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopAll(void);

/**
 * @brief 获取PEPS当前状态
 */
PEPS_State_t CAN_TestBox_PEPS_GetCurrentState(void);

/**
 * @brief 获取PEPS统计信息
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_GetStatistics(PEPS_Statistics_t *stats);
```

#### 8. 完整测试序列

```c
/**
 * @brief 执行PEPS完整测试序列
 * @param test_duration_ms 测试持续时间
 * @param result 测试结果输出
 * @retval CAN_TESTBOX_OK 测试成功
 */
CAN_TestBox_Status_t CAN_TestBox_PEPS_RunFullTest(
    uint32_t test_duration_ms, 
    CAN_TestBox_PEPS_Result_t *result);
```

**使用示例：**
```c
CAN_TestBox_PEPS_Result_t test_result;
CAN_TestBox_Status_t status = CAN_TestBox_PEPS_RunFullTest(10000, &test_result);

if (status == CAN_TESTBOX_OK) {
    printf("PEPS完整测试完成\r\n");
    printf("总执行时间: %lu ms\r\n", test_result.execution_time_ms);
    printf("总发送帧数: %lu\r\n", test_result.frames_sent);
    printf("最终状态: %d\r\n", test_result.final_state);
} else {
    printf("PEPS测试失败: %d\r\n", status);
}
```

### PEPS便捷接口

便捷接口提供了更简单易用的测试方法，适合快速测试和批量测试场景。

#### 1. 一键初始化

```c
#include "can_testbox_peps_helper.h"

// 一键初始化PEPS测试环境
CAN_TestBox_Status_t status = PEPS_Helper_QuickInit();
if (status == CAN_TESTBOX_OK) {
    printf("PEPS测试环境初始化完成\r\n");
}
```

#### 2. 快速测试序列

```c
// 获取默认测试配置
PEPS_QuickTest_Config_t config = PEPS_Helper_GetDefaultQuickConfig();

// 自定义配置
config.scw1_test_duration_ms = 3000;     // SCW1测试3秒
config.scw2_test_duration_ms = 3000;     // SCW2测试3秒
config.enable_debug_output = true;       // 启用调试输出

// 执行快速测试
PEPS_QuickTest_Result_t result;
status = PEPS_Helper_RunQuickTest(&config, &result);

if (status == CAN_TESTBOX_OK) {
    printf("快速测试完成\r\n");
    printf("总时间: %lu ms\r\n", result.total_test_time_ms);
    printf("总帧数: %lu\r\n", result.total_frames_sent);
}
```

#### 3. 压力测试

```c
// SCW1压力测试
uint32_t success_count;
status = PEPS_Helper_SCW1StressTest(100, 500, &success_count);
printf("SCW1压力测试: %lu/100 成功\r\n", success_count);

// SCW2压力测试
status = PEPS_Helper_SCW2StressTest(100, 500, &success_count);
printf("SCW2压力测试: %lu/100 成功\r\n", success_count);
```

#### 4. 循环测试

```c
// 钥匙位置循环测试
uint8_t key_positions[] = {0x00, 0x01, 0x02, 0x03, 0x04};
status = PEPS_Helper_KeyPositionCycleTest(key_positions, 5, 2000);

// BSI状态循环测试
uint8_t bsi_states[] = {0x00, 0x01, 0x02, 0x04, 0x08};
status = PEPS_Helper_BSIStateCycleTest(bsi_states, 5, 1500);
```

#### 5. 统计信息和环境检查

```c
// 获取测试统计摘要
char summary[512];
PEPS_Helper_GetTestSummary(summary, sizeof(summary));
printf("%s\r\n", summary);

// 检查测试环境
char env_status[256];
PEPS_Helper_CheckEnvironment(env_status, sizeof(env_status));
printf("%s\r\n", env_status);

// 紧急停止所有测试
PEPS_Helper_EmergencyStop();
```

### 预定义测试配置

便捷接口提供了三种预定义的测试配置：

```c
// 1. 默认配置（适合日常测试）
PEPS_QuickTest_Config_t default_config = PEPS_Helper_GetDefaultQuickConfig();

// 2. 压力测试配置（适合稳定性测试）
PEPS_QuickTest_Config_t stress_config = PEPS_Helper_GetStressTestConfig();

// 3. 兼容性测试配置（适合兼容性验证）
PEPS_QuickTest_Config_t compat_config = PEPS_Helper_GetCompatibilityTestConfig();
```

### 错误处理和调试

#### 常见错误码

- `CAN_TESTBOX_NOT_INITIALIZED`：模块未初始化
- `CAN_TESTBOX_INVALID_PARAM`：参数无效
- `CAN_TESTBOX_ERROR`：一般错误
- `CAN_TESTBOX_ALREADY_EXISTS`：资源已存在

#### 调试建议

1. **启用调试输出**：设置`enable_debug_output = true`
2. **检查初始化状态**：确保先调用初始化函数
3. **监控统计信息**：定期检查发送/接收统计
4. **使用环境检查**：调用`PEPS_Helper_CheckEnvironment()`

### 最佳实践

1. **初始化顺序**：先初始化CAN测试盒，再初始化PEPS模块
2. **资源管理**：测试完成后及时停止所有测试
3. **错误处理**：检查每个API调用的返回值
4. **性能监控**：定期获取统计信息监控性能
5. **测试隔离**：不同测试之间留有适当间隔

### PEPS完整示例代码

#### 示例1：基础PEPS测试任务

```c
#include "can_testbox_api.h"
#include "can_testbox_peps_helper.h"
#include "cmsis_os.h"

/**
 * @brief PEPS基础测试任务
 * @param argument 任务参数
 */
void PEPS_BasicTestTask(void const *argument)
{
    CAN_TestBox_Status_t status;
    
    // 1. 一键初始化PEPS测试环境
    printf("[PEPS Test] 正在初始化PEPS测试环境...\r\n");
    status = PEPS_Helper_QuickInit();
    if (status != CAN_TESTBOX_OK) {
        printf("[PEPS Test] 初始化失败: %d\r\n", status);
        return;
    }
    
    // 2. 执行基础功能测试
    printf("[PEPS Test] 开始执行基础功能测试...\r\n");
    
    // SCW1唤醒测试
    printf("[PEPS Test] 测试SCW1唤醒功能...\r\n");
    status = CAN_TestBox_PEPS_StartSCW1Wakeup(NULL, CAN_TESTBOX_PERIOD_200MS);
    if (status == CAN_TESTBOX_OK) {
        osDelay(3000);  // 运行3秒
        CAN_TestBox_PEPS_StopSCW1Wakeup();
        printf("[PEPS Test] SCW1唤醒测试完成\r\n");
    }
    
    osDelay(1000);  // 测试间隔
    
    // SCW2唤醒测试
    printf("[PEPS Test] 测试SCW2唤醒功能...\r\n");
    status = CAN_TestBox_PEPS_StartSCW2Wakeup(NULL, CAN_TESTBOX_PERIOD_200MS);
    if (status == CAN_TESTBOX_OK) {
        osDelay(3000);  // 运行3秒
        CAN_TestBox_PEPS_StopSCW2Wakeup();
        printf("[PEPS Test] SCW2唤醒测试完成\r\n");
    }
    
    osDelay(1000);  // 测试间隔
    
    // 钥匙位置测试
    printf("[PEPS Test] 测试钥匙位置功能...\r\n");
    uint8_t key_positions[] = {0x00, 0x01, 0x02, 0x03};
    status = PEPS_Helper_KeyPositionCycleTest(key_positions, 4, 2000);
    if (status == CAN_TESTBOX_OK) {
        printf("[PEPS Test] 钥匙位置测试完成\r\n");
    }
    
    // 3. 获取测试统计信息
    char summary[512];
    PEPS_Helper_GetTestSummary(summary, sizeof(summary));
    printf("[PEPS Test] 测试统计信息:\r\n%s\r\n", summary);
    
    printf("[PEPS Test] 基础测试任务完成\r\n");
}
```

#### 示例2：PEPS压力测试任务

```c
/**
 * @brief PEPS压力测试任务
 * @param argument 任务参数
 */
void PEPS_StressTestTask(void const *argument)
{
    CAN_TestBox_Status_t status;
    uint32_t success_count;
    
    // 初始化
    status = PEPS_Helper_QuickInit();
    if (status != CAN_TESTBOX_OK) {
        printf("[PEPS Stress] 初始化失败\r\n");
        return;
    }
    
    printf("[PEPS Stress] 开始压力测试...\r\n");
    
    // SCW1压力测试
    printf("[PEPS Stress] SCW1压力测试 (200次)...\r\n");
    status = PEPS_Helper_SCW1StressTest(200, 300, &success_count);
    printf("[PEPS Stress] SCW1结果: %lu/200 成功 (%.1f%%)\r\n", 
           success_count, (float)success_count * 100.0f / 200.0f);
    
    osDelay(2000);  // 测试间隔
    
    // SCW2压力测试
    printf("[PEPS Stress] SCW2压力测试 (200次)...\r\n");
    status = PEPS_Helper_SCW2StressTest(200, 300, &success_count);
    printf("[PEPS Stress] SCW2结果: %lu/200 成功 (%.1f%%)\r\n", 
           success_count, (float)success_count * 100.0f / 200.0f);
    
    // 获取最终统计
    char summary[512];
    PEPS_Helper_GetTestSummary(summary, sizeof(summary));
    printf("[PEPS Stress] 压力测试完成:\r\n%s\r\n", summary);
}
```

#### 示例3：PEPS自动化测试序列

```c
/**
 * @brief PEPS自动化测试序列
 * @param argument 任务参数
 */
void PEPS_AutoTestSequence(void const *argument)
{
    CAN_TestBox_Status_t status;
    
    // 初始化
    status = PEPS_Helper_QuickInit();
    if (status != CAN_TESTBOX_OK) {
        printf("[PEPS Auto] 初始化失败\r\n");
        return;
    }
    
    printf("[PEPS Auto] 开始自动化测试序列...\r\n");
    
    // 测试序列1：快速功能验证
    printf("[PEPS Auto] === 序列1: 快速功能验证 ===\r\n");
    PEPS_QuickTest_Config_t quick_config = PEPS_Helper_GetDefaultQuickConfig();
    PEPS_QuickTest_Result_t quick_result;
    
    status = PEPS_Helper_RunQuickTest(&quick_config, &quick_result);
    if (status == CAN_TESTBOX_OK) {
        printf("[PEPS Auto] 快速测试成功，耗时: %lu ms，发送帧数: %lu\r\n",
               quick_result.total_test_time_ms, quick_result.total_frames_sent);
    } else {
        printf("[PEPS Auto] 快速测试失败: %d\r\n", status);
    }
    
    osDelay(3000);  // 序列间隔
    
    // 测试序列2：压力测试
    printf("[PEPS Auto] === 序列2: 压力测试 ===\r\n");
    PEPS_QuickTest_Config_t stress_config = PEPS_Helper_GetStressTestConfig();
    PEPS_QuickTest_Result_t stress_result;
    
    status = PEPS_Helper_RunQuickTest(&stress_config, &stress_result);
    if (status == CAN_TESTBOX_OK) {
        printf("[PEPS Auto] 压力测试成功，耗时: %lu ms，发送帧数: %lu\r\n",
               stress_result.total_test_time_ms, stress_result.total_frames_sent);
    } else {
        printf("[PEPS Auto] 压力测试失败: %d\r\n", status);
    }
    
    osDelay(3000);  // 序列间隔
    
    // 测试序列3：兼容性测试
    printf("[PEPS Auto] === 序列3: 兼容性测试 ===\r\n");
    PEPS_QuickTest_Config_t compat_config = PEPS_Helper_GetCompatibilityTestConfig();
    PEPS_QuickTest_Result_t compat_result;
    
    status = PEPS_Helper_RunQuickTest(&compat_config, &compat_result);
    if (status == CAN_TESTBOX_OK) {
        printf("[PEPS Auto] 兼容性测试成功，耗时: %lu ms，发送帧数: %lu\r\n",
               compat_result.total_test_time_ms, compat_result.total_frames_sent);
    } else {
        printf("[PEPS Auto] 兼容性测试失败: %d\r\n", status);
    }
    
    // 生成测试报告
    printf("[PEPS Auto] === 测试报告 ===\r\n");
    printf("快速测试: %s (耗时: %lu ms, 帧数: %lu)\r\n",
           quick_result.final_status == CAN_TESTBOX_OK ? "通过" : "失败",
           quick_result.total_test_time_ms, quick_result.total_frames_sent);
    printf("压力测试: %s (耗时: %lu ms, 帧数: %lu)\r\n",
           stress_result.final_status == CAN_TESTBOX_OK ? "通过" : "失败",
           stress_result.total_test_time_ms, stress_result.total_frames_sent);
    printf("兼容性测试: %s (耗时: %lu ms, 帧数: %lu)\r\n",
           compat_result.final_status == CAN_TESTBOX_OK ? "通过" : "失败",
           compat_result.total_test_time_ms, compat_result.total_frames_sent);
    
    // 获取总体统计
    char summary[512];
    PEPS_Helper_GetTestSummary(summary, sizeof(summary));
    printf("[PEPS Auto] 总体统计:\r\n%s\r\n", summary);
    
    printf("[PEPS Auto] 自动化测试序列完成\r\n");
}
```

#### 示例4：PEPS实时监控任务

```c
/**
 * @brief PEPS实时监控任务
 * @param argument 任务参数
 */
void PEPS_MonitorTask(void const *argument)
{
    CAN_TestBox_Status_t status;
    PEPS_Statistics_t last_stats = {0};
    
    // 初始化
    status = PEPS_Helper_QuickInit();
    if (status != CAN_TESTBOX_OK) {
        printf("[PEPS Monitor] 初始化失败\r\n");
        return;
    }
    
    printf("[PEPS Monitor] 开始实时监控...\r\n");
    
    while (1) {
        // 检查环境状态
        char env_status[256];
        PEPS_Helper_CheckEnvironment(env_status, sizeof(env_status));
        
        // 获取当前统计信息
        PEPS_Statistics_t current_stats;
        CAN_TestBox_PEPS_GetStatistics(&current_stats);
        
        // 检查是否有新的活动
        if (current_stats.frames_sent != last_stats.frames_sent ||
            current_stats.frames_received != last_stats.frames_received) {
            
            printf("[PEPS Monitor] 活动检测:\r\n");
            printf("  发送帧数: %lu (+%lu)\r\n", 
                   current_stats.frames_sent, 
                   current_stats.frames_sent - last_stats.frames_sent);
            printf("  接收帧数: %lu (+%lu)\r\n", 
                   current_stats.frames_received,
                   current_stats.frames_received - last_stats.frames_received);
            printf("  当前状态: %d\r\n", CAN_TestBox_PEPS_GetCurrentState());
            
            last_stats = current_stats;
        }
        
        // 每5秒输出一次完整状态
        static uint32_t counter = 0;
        if (++counter >= 50) {  // 5秒 (100ms * 50)
            printf("[PEPS Monitor] === 定期状态报告 ===\r\n");
            printf("%s\r\n", env_status);
            
            char summary[512];
            PEPS_Helper_GetTestSummary(summary, sizeof(summary));
            printf("%s\r\n", summary);
            
            counter = 0;
        }
        
        osDelay(100);  // 100ms监控间隔
    }
}
```

#### 示例5：主函数集成示例

```c
#include "main.h"
#include "can_testbox_api.h"
#include "can_testbox_peps_helper.h"

// 任务句柄
osThreadId peps_basic_task_handle;
osThreadId peps_monitor_task_handle;

/**
 * @brief 主函数
 */
int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_USART1_UART_Init();
    
    // 启动FreeRTOS调度器之前的初始化
    printf("CAN测试盒PEPS系统启动...\r\n");
    
    // 创建RTOS对象
    osKernelInitialize();
    
    // 创建PEPS测试任务
    osThreadDef(peps_basic_task, PEPS_BasicTestTask, osPriorityNormal, 0, 512);
    peps_basic_task_handle = osThreadCreate(osThread(peps_basic_task), NULL);
    
    // 创建PEPS监控任务
    osThreadDef(peps_monitor_task, PEPS_MonitorTask, osPriorityLow, 0, 256);
    peps_monitor_task_handle = osThreadCreate(osThread(peps_monitor_task), NULL);
    
    // 启动调度器
    osKernelStart();
    
    // 不应该到达这里
    while (1) {
        HAL_Delay(1000);
    }
}

/**
 * @brief 错误处理函数
 */
void Error_Handler(void)
{
    printf("系统错误，停止所有PEPS测试\r\n");
    PEPS_Helper_EmergencyStop();
    
    while (1) {
        HAL_GPIO_TogglePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin);
        HAL_Delay(200);
    }
}
```

### 集成指导

#### 1. 项目配置

在STM32CubeIDE中添加以下文件到项目：

```
项目结构:
├── Core/
│   ├── Inc/
│   │   ├── can_testbox_api.h
│   │   ├── can_testbox_peps_helper.h
│   │   └── can_peps_state_machine.h
│   └── Src/
│       ├── can_testbox_api.c
│       ├── can_testbox_peps_helper.c
│       └── can_peps_state_machine.c
```

#### 2. 编译配置

在项目设置中添加包含路径：
- `Core/Inc`
- `Middlewares/Third_Party/FreeRTOS/Source/include`
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS`

#### 3. 依赖库

确保项目包含以下库：
- HAL库（CAN、GPIO、UART等）
- FreeRTOS
- CMSIS-RTOS2

#### 4. 内存配置

建议的任务堆栈大小：
- PEPS基础测试任务：512字节
- PEPS监控任务：256字节
- PEPS压力测试任务：1024字节

#### 5. 调试配置

启用以下调试功能：
```c
// 在main.c中添加
#define PEPS_DEBUG_ENABLE 1

#if PEPS_DEBUG_ENABLE
#define PEPS_DEBUG_PRINT(fmt, ...) printf("[PEPS DEBUG] " fmt, ##__VA_ARGS__)
#else
#define PEPS_DEBUG_PRINT(fmt, ...)
#endif
```

## 总结

CAN测试盒API提供了完整的专业级CAN总线测试功能，支持：

- ✅ 单帧事件报文发送
- ✅ 周期性报文发送
- ✅ 连续帧报文发送
- ✅ 完整的配置宏支持
- ✅ 专业的队列管理
- ✅ 统计和诊断功能
- ✅ 详细的使用示例
- ✅ PEPS专业测试接口

通过合理使用这些接口，可以构建功能强大的CAN总线测试和验证系统。建议根据具体需求，逐步添加高级功能如错误注入、网络层支持等。
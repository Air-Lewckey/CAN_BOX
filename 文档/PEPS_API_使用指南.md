# PEPS专业API测试接口使用指南

## 概述

本项目为STM32F407开发板提供了完整的PEPS（无钥匙进入启动系统）测试接口，基于正点原子的CAN测试盒框架开发。通过专业的API接口，可以方便地测试PEPS系统的各种功能，包括SCW1/SCW2唤醒、钥匙位置检测、BSI状态等。

## 文件结构

```
CAN_BOX/
├── Core/
│   ├── Inc/
│   │   ├── peps_simple_test.h          # 简单测试函数声明
│   │   ├── peps_api_test.h             # FreeRTOS任务测试函数声明
│   │   ├── can_testbox_api.h           # CAN测试盒API声明
│   │   └── can_testbox_peps_helper.h   # PEPS辅助函数声明
│   └── Src/
│       ├── peps_simple_test.c          # 简单测试函数实现（推荐）
│       ├── peps_api_test.c             # FreeRTOS任务测试函数实现
│       ├── can_testbox_peps_helper.c   # PEPS辅助函数实现
│       └── main.c                      # 主程序文件
├── main_integration_example.c          # 完整集成示例
├── peps_test_main_example.c           # 主程序调用示例
└── PEPS_API_使用指南.md               # 本文件
```

## 快速开始

### 1. 最简单的使用方法（推荐新手）

在您的`main.c`文件中添加以下代码：

```c
#include "peps_simple_test.h"

int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_USART1_UART_Init();
    
    // 等待系统稳定
    HAL_Delay(2000);
    
    // 执行PEPS测试
    printf("开始PEPS测试...\r\n");
    
    // 快速连通性测试
    PEPS_Quick_Connectivity_Test();
    
    // 完整功能测试
    PEPS_Simple_Test_All_Messages();
    
    while(1)
    {
        HAL_Delay(1000);
    }
}
```

### 2. 高级使用方法（FreeRTOS任务）

如果您的项目使用FreeRTOS，可以使用任务版本：

```c
#include "peps_api_test.h"

int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    // ... 其他初始化
    
    // 启动PEPS测试任务
    PEPS_Test_StartAllTests();
    
    // 启动FreeRTOS调度器
    osKernelStart();
    
    while(1); // 不应该到达这里
}
```

## API接口说明

### 简单测试接口（peps_simple_test.h）

#### `PEPS_Simple_Test_All_Messages()`
- **功能**：依次测试所有PEPS报文类型
- **测试内容**：
  - SCW1唤醒报文（运行3秒）
  - SCW2唤醒报文（运行3秒）
  - 钥匙位置报文（4个位置，每个2秒）
  - BSI状态报文（4个状态，每个2秒）
- **适用场景**：完整功能验证
- **执行时间**：约30秒

#### `PEPS_Quick_Connectivity_Test()`
- **功能**：快速验证PEPS系统连通性
- **测试内容**：发送一帧SCW1测试数据
- **适用场景**：快速调试和基本功能验证
- **执行时间**：约3秒

### FreeRTOS任务接口（peps_api_test.h）

#### `PEPS_Test_StartAllTests()`
- **功能**：启动所有PEPS测试任务
- **包含任务**：
  - 主测试任务：循环执行各种PEPS测试
  - 压力测试任务：高频率发送测试数据
- **适用场景**：长期运行和压力测试

#### `PEPS_Test_SingleVerification()`
- **功能**：执行单次完整验证
- **返回值**：`CAN_TestBox_Status_t`状态码
- **适用场景**：程序化验证和自动化测试

#### `PEPS_Test_QuickConnectivityCheck()`
- **功能**：快速连通性检查（FreeRTOS版本）
- **返回值**：`CAN_TestBox_Status_t`状态码
- **适用场景**：定期健康检查

## 核心API接口（can_testbox_api.h）

### 初始化接口

```c
// 初始化CAN测试盒
CAN_TestBox_Status_t CAN_TestBox_Init(CAN_HandleTypeDef *hcan);

// 初始化PEPS模块
CAN_TestBox_Status_t CAN_TestBox_PEPS_Init(void);
```

### SCW1唤醒接口

```c
// 启动SCW1唤醒测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartSCW1Wakeup(
    const uint8_t *data,           // 8字节测试数据
    CAN_TestBox_Period_t period    // 发送周期
);

// 停止SCW1唤醒测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopSCW1Wakeup(void);
```

### SCW2唤醒接口

```c
// 启动SCW2唤醒测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartSCW2Wakeup(
    const uint8_t *data,           // 8字节测试数据
    CAN_TestBox_Period_t period    // 发送周期
);

// 停止SCW2唤醒测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopSCW2Wakeup(void);
```

### 钥匙位置接口

```c
// 启动钥匙位置测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartKeyPosition(
    uint8_t position,              // 钥匙位置 (0-3)
    CAN_TestBox_Period_t period    // 发送周期
);

// 停止钥匙位置测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopKeyPosition(void);
```

钥匙位置定义：
- `0x00`：无钥匙
- `0x01`：ACC位置
- `0x02`：ON位置
- `0x03`：START位置

### BSI状态接口

```c
// 启动BSI状态测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StartBSIStatus(
    uint8_t status,                // BSI状态 (0-3)
    CAN_TestBox_Period_t period    // 发送周期
);

// 停止BSI状态测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopBSIStatus(void);
```

BSI状态定义：
- `0x00`：休眠
- `0x01`：唤醒
- `0x02`：运行
- `0x03`：诊断

### 统计和控制接口

```c
// 停止所有PEPS测试
CAN_TestBox_Status_t CAN_TestBox_PEPS_StopAll(void);

// 获取统计信息
CAN_TestBox_Status_t CAN_TestBox_PEPS_GetStatistics(PEPS_Statistics_t *stats);

// 重置统计信息
CAN_TestBox_Status_t CAN_TestBox_PEPS_ResetStatistics(void);
```

## 发送周期定义

```c
typedef enum {
    CAN_TESTBOX_PERIOD_100MS  = 100,   // 100毫秒周期
    CAN_TESTBOX_PERIOD_200MS  = 200,   // 200毫秒周期
    CAN_TESTBOX_PERIOD_500MS  = 500,   // 500毫秒周期
    CAN_TESTBOX_PERIOD_1000MS = 1000,  // 1秒周期
    CAN_TESTBOX_PERIOD_2000MS = 2000   // 2秒周期
} CAN_TestBox_Period_t;
```

## 状态码定义

```c
typedef enum {
    CAN_TESTBOX_OK = 0,        // 操作成功
    CAN_TESTBOX_ERROR,         // 一般错误
    CAN_TESTBOX_BUSY,          // 系统忙
    CAN_TESTBOX_TIMEOUT,       // 超时
    CAN_TESTBOX_INVALID_PARAM, // 无效参数
    CAN_TESTBOX_NOT_INIT       // 未初始化
} CAN_TestBox_Status_t;
```

## 统计信息结构

```c
typedef struct {
    uint32_t total_commands;      // 总命令数
    uint32_t duplicate_commands;  // 重复命令数
    uint32_t state_transitions;   // 状态转换次数
    uint32_t frames_sent;         // 发送帧总数
    uint32_t active_frames;       // 当前激活帧数
    uint8_t  current_state;       // 当前状态
    uint32_t uptime_ms;          // 运行时间(毫秒)
} PEPS_Statistics_t;
```

## 编译配置

### 必需的头文件路径

在项目设置中添加以下包含路径：
```
../Core/Inc
../Drivers/STM32F4xx_HAL_Driver/Inc
../Drivers/CMSIS/Device/ST/STM32F4xx/Include
../Drivers/CMSIS/Include
```

### 必需的源文件

确保以下文件被包含在编译中：
```
Core/Src/peps_simple_test.c
Core/Src/can_testbox_peps_helper.c
Core/Src/can_testbox_api.c
Core/Src/can_peps_state_machine.c
```

可选文件（如果使用FreeRTOS）：
```
Core/Src/peps_api_test.c
```

### 预处理器定义

```
USE_HAL_DRIVER
STM32F407xx
DEBUG
```

如果使用FreeRTOS任务测试：
```
USE_FREERTOS_TESTS
```

## 硬件配置

### CAN总线配置

- **波特率**：500Kbps（标准配置）
- **采样点**：87.5%
- **时钟源**：APB1时钟
- **引脚配置**：
  - CAN1_TX：PA12
  - CAN1_RX：PA11

### UART配置（用于调试输出）

- **波特率**：115200
- **数据位**：8
- **停止位**：1
- **校验位**：无
- **引脚配置**：
  - USART1_TX：PA9
  - USART1_RX：PA10

## 使用示例

### 示例1：基本功能测试

```c
#include "peps_simple_test.h"

void basic_peps_test(void)
{
    printf("开始基本PEPS功能测试...\r\n");
    
    // 快速连通性检查
    PEPS_Quick_Connectivity_Test();
    
    HAL_Delay(2000);
    
    // 完整功能测试
    PEPS_Simple_Test_All_Messages();
    
    printf("基本PEPS功能测试完成\r\n");
}
```

### 示例2：自定义测试序列

```c
#include "can_testbox_api.h"

void custom_peps_test(void)
{
    CAN_TestBox_Status_t status;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    // 初始化
    status = CAN_TestBox_Init(&hcan1);
    if (status != CAN_TESTBOX_OK) {
        printf("CAN测试盒初始化失败\r\n");
        return;
    }
    
    status = CAN_TestBox_PEPS_Init();
    if (status != CAN_TESTBOX_OK) {
        printf("PEPS模块初始化失败\r\n");
        return;
    }
    
    // 自定义SCW1测试
    printf("启动自定义SCW1测试...\r\n");
    status = CAN_TestBox_PEPS_StartSCW1Wakeup(test_data, CAN_TESTBOX_PERIOD_100MS);
    if (status == CAN_TESTBOX_OK) {
        HAL_Delay(5000);  // 运行5秒
        CAN_TestBox_PEPS_StopSCW1Wakeup();
        printf("SCW1测试完成\r\n");
    }
    
    // 自定义钥匙位置测试
    printf("启动钥匙位置测试...\r\n");
    for (int pos = 0; pos <= 3; pos++) {
        printf("测试钥匙位置: %d\r\n", pos);
        status = CAN_TestBox_PEPS_StartKeyPosition(pos, CAN_TESTBOX_PERIOD_500MS);
        if (status == CAN_TESTBOX_OK) {
            HAL_Delay(2000);  // 每个位置测试2秒
            CAN_TestBox_PEPS_StopKeyPosition();
        }
        HAL_Delay(500);  // 位置切换间隔
    }
    
    // 获取统计信息
    PEPS_Statistics_t stats;
    status = CAN_TestBox_PEPS_GetStatistics(&stats);
    if (status == CAN_TESTBOX_OK) {
        printf("=== 测试统计 ===\r\n");
        printf("总命令数: %lu\r\n", stats.total_commands);
        printf("发送帧数: %lu\r\n", stats.frames_sent);
        printf("运行时间: %lu ms\r\n", stats.uptime_ms);
    }
}
```

### 示例3：错误处理

```c
void peps_test_with_error_handling(void)
{
    CAN_TestBox_Status_t status;
    
    // 带错误检查的初始化
    status = CAN_TestBox_Init(&hcan1);
    switch (status) {
        case CAN_TESTBOX_OK:
            printf("CAN测试盒初始化成功\r\n");
            break;
        case CAN_TESTBOX_ERROR:
            printf("CAN测试盒初始化失败\r\n");
            return;
        case CAN_TESTBOX_INVALID_PARAM:
            printf("CAN句柄参数无效\r\n");
            return;
        default:
            printf("未知错误: %d\r\n", status);
            return;
    }
    
    // 带超时的测试
    uint32_t start_time = HAL_GetTick();
    status = CAN_TestBox_PEPS_StartSCW1Wakeup(test_data, CAN_TESTBOX_PERIOD_200MS);
    
    if (status == CAN_TESTBOX_OK) {
        printf("SCW1测试启动成功\r\n");
        
        // 运行测试，带超时保护
        while ((HAL_GetTick() - start_time) < 10000) {  // 10秒超时
            HAL_Delay(100);
            // 可以在这里检查测试状态
        }
        
        // 停止测试
        status = CAN_TestBox_PEPS_StopSCW1Wakeup();
        if (status == CAN_TESTBOX_OK) {
            printf("SCW1测试正常停止\r\n");
        } else {
            printf("SCW1测试停止失败: %d\r\n", status);
        }
    } else {
        printf("SCW1测试启动失败: %d\r\n", status);
    }
}
```

## 故障排除

### 常见问题

1. **编译错误：找不到头文件**
   - 检查包含路径设置
   - 确保所有必需的头文件都在正确位置

2. **链接错误：未定义的引用**
   - 检查所有必需的.c文件是否被包含在编译中
   - 确保函数声明和定义匹配

3. **运行时错误：CAN初始化失败**
   - 检查CAN硬件连接
   - 验证CAN时钟配置
   - 确保CAN引脚配置正确

4. **测试无输出**
   - 检查UART配置和连接
   - 确保printf重定向正确实现
   - 验证波特率设置

5. **CAN发送失败**
   - 检查CAN总线终端电阻
   - 验证目标设备是否在线
   - 检查CAN波特率匹配

### 调试技巧

1. **使用串口输出调试信息**
   ```c
   printf("[DEBUG] 当前状态: %d\r\n", current_state);
   ```

2. **检查返回状态码**
   ```c
   status = CAN_TestBox_PEPS_StartSCW1Wakeup(data, period);
   if (status != CAN_TESTBOX_OK) {
       printf("[ERROR] SCW1启动失败，状态码: %d\r\n", status);
   }
   ```

3. **使用统计信息监控**
   ```c
   PEPS_Statistics_t stats;
   CAN_TestBox_PEPS_GetStatistics(&stats);
   printf("发送帧数: %lu, 错误数: %lu\r\n", stats.frames_sent, stats.duplicate_commands);
   ```

4. **逐步测试**
   - 先测试基本初始化
   - 再测试单个功能
   - 最后进行综合测试

## 技术支持

如果您在使用过程中遇到问题，请：

1. 检查本文档的故障排除部分
2. 查看示例代码和注释
3. 确认硬件连接和配置
4. 联系正点原子技术支持

## 版本历史

- **V1.0** (2024-01-15)
  - 初始版本发布
  - 支持SCW1/SCW2唤醒测试
  - 支持钥匙位置和BSI状态测试
  - 提供简单测试和FreeRTOS任务两种接口
  - 完整的API文档和使用示例

---

**正点原子@ALIENTEK**  
**STM32F407开发板 - CAN测试盒PEPS专业测试接口**  
**技术支持：www.alientek.com**
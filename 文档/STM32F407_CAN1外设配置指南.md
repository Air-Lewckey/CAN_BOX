# STM32F407 CAN1外设配置指南

## 📋 **配置目标**
为STM32F407ZGT6添加内置CAN1外设配置，实现与WCMCU-230模块的双CAN节点通信。

---

## ⚙️ **详细配置步骤**

### **第一步：打开配置文件**

1. **在STM32CubeIDE中**：
   - 双击项目中的 `CAN_BOX.ioc` 文件
   - 等待STM32CubeMX界面加载完成

### **第二步：启用CAN1外设**

1. **在左侧面板中**：
   - 展开 `Connectivity` 分类
   - 点击 `CAN1`

2. **激活CAN1**：
   - 将 `Mode` 设置为 `Activated`
   - 确保CAN1外设被启用

### **第三步：配置CAN1参数**

#### **基本参数设置**
```
Prescaler (for Time Quantum): 5  ← 已修正为5
Time Quanta in Bit Segment 1: 13 Times
Time Quanta in Bit Segment 2: 2 Times
ReSynchronization Jump Width: 1 Times
```

#### **波特率计算**
```
系统时钟: 42MHz (APB1)  ← 修正：APB1实际为42MHz
波特率 = 42MHz / (5 × (13 + 2 + 1)) = 525 Kbps  ← 接近500Kbps目标

注意：原配置Prescaler=6时波特率为437.5Kbps，现已修正为525Kbps
```

#### **高级参数**
```
Time Triggered Communication Mode: Disabled
Automatic Bus-Off Management: Enabled
Automatic Wake-Up Mode: Enabled
Automatic Retransmission: Enabled
Receive FIFO Locked Mode: Disabled
Transmit FIFO Priority: Disabled
```

### **第四步：配置CAN1引脚**

#### **方案A：使用PA11/PA12引脚**
1. **在Pinout视图中**：
   - 找到 `PA11` 引脚，设置为 `CAN1_RX`
   - 找到 `PA12` 引脚，设置为 `CAN1_TX`

2. **GPIO配置**：
   - **PA11 (CAN1_RX)**：
     - GPIO mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: Pull Up
     - Maximum output speed: High
   - **PA12 (CAN1_TX)**：
     - GPIO mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: High

#### **方案B：使用PB8/PB9引脚（备选）**
1. **在Pinout视图中**：
   - 找到 `PB8` 引脚，设置为 `CAN1_RX`
   - 找到 `PB9` 引脚，设置为 `CAN1_TX`

2. **GPIO配置**：
   - **PB8 (CAN1_RX)**：
     - GPIO mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: Pull Up
     - Maximum output speed: High
   - **PB9 (CAN1_TX)**：
     - GPIO mode: Alternate Function Push Pull
     - GPIO Pull-up/Pull-down: No pull-up and no pull-down
     - Maximum output speed: High

### **第五步：配置CAN1中断**

1. **在NVIC Settings中**：
   - 展开 `CAN1 RX0 interrupts`，勾选 `Enabled`
   - 展开 `CAN1 TX interrupts`，勾选 `Enabled`（可选）
   - 展开 `CAN1 SCE interrupts`，勾选 `Enabled`（错误中断）

2. **设置中断优先级**：
   ```
   CAN1 RX0 interrupts:
   - Preemption Priority: 1
   - Sub Priority: 0
   
   CAN1 TX interrupts:
   - Preemption Priority: 2
   - Sub Priority: 0
   
   CAN1 SCE interrupts:
   - Preemption Priority: 1
   - Sub Priority: 1
   ```

### **第六步：添加新的FreeRTOS任务**

1. **在FreeRTOS配置中**：
   - 点击 `Tasks and Queues` 标签
   - 点击 `Add` 按钮添加新任务

2. **创建CAN双节点任务**：
   ```
   Task Name: CANDualNodeTask
   Priority: Normal
   Stack Size (Words): 512
   Entry Function: StartCANDualNodeTask
   Code Generation Option: As weak
   ```

### **第七步：生成代码**

1. **保存配置**：
   - 按 `Ctrl+S` 保存配置文件

2. **生成代码**：
   - 点击 `Generate Code` 按钮（齿轮图标）
   - 等待代码生成完成

3. **检查生成的文件**：
   - `main.c` - 应包含CAN1初始化代码
   - `can.c` 和 `can.h` - CAN外设驱动文件
   - 新的任务函数声明

---

## 🔧 **代码集成步骤**

### **第一步：修改main.h**

在 `main.h` 文件中添加：
```c
/* USER CODE BEGIN Includes */
#include "can_dual_node.h"
/* USER CODE END Includes */

/* USER CODE BEGIN EFP */
extern CAN_HandleTypeDef hcan1;
/* USER CODE END EFP */
```

### **第二步：修改main.c**

在 `main.c` 文件中添加：
```c
/* USER CODE BEGIN Includes */
#include "can_dual_node.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
// 在现有CAN应用初始化后添加
printf("\r\n=== 启动双CAN节点通信 ===\r\n");
printf("STM32F407 CAN1 + WCMCU-230 双节点通信系统\r\n");
/* USER CODE END 2 */
```

### **第三步：实现任务函数**

在 `main.c` 文件中添加任务实现：
```c
/* USER CODE BEGIN 4 */
/**
  * @brief  CAN双节点通信任务
  * @param  argument: 任务参数
  * @retval None
  */
void StartCANDualNodeTask(void *argument)
{
  /* USER CODE BEGIN StartCANDualNodeTask */
  // 调用双CAN节点通信任务主函数
  CAN_DualNodeTask(argument);
  /* USER CODE END StartCANDualNodeTask */
}
/* USER CODE END 4 */
```

---

## 📊 **配置验证清单**

### **硬件配置检查**
- [ ] CAN1外设已激活
- [ ] CAN1引脚配置正确（PA11/PA12 或 PB8/PB9）
- [ ] CAN1中断已启用
- [ ] 中断优先级设置合理

### **软件配置检查**
- [ ] CAN1波特率配置为525Kbps（接近500Kbps目标）
- [ ] 自动重传和错误管理已启用
- [ ] FreeRTOS任务已创建
- [ ] 代码生成无错误

### **文件检查**
- [ ] `can.c` 和 `can.h` 文件已生成
- [ ] `main.c` 包含CAN1初始化代码
- [ ] 新任务函数已声明
- [ ] 头文件包含正确

---

## ⚠️ **注意事项**

### **引脚冲突检查**
- 确保PA11/PA12没有被其他外设占用
- 如果有冲突，使用PB8/PB9备选方案
- 检查调试器引脚不要与CAN引脚冲突

### **时钟配置**
- 确保APB1时钟为42MHz（系统时钟168MHz，APB1预分频器为4）
- CAN时钟来源于APB1
- 波特率计算要准确：42MHz / (5 × 16TQ) = 525Kbps

### **中断优先级**
- CAN接收中断优先级要高于发送中断
- 避免与其他关键中断冲突
- FreeRTOS中断优先级要正确设置

---

## 🚀 **下一步操作**

配置完成后：

1. **编译项目**：
   - 检查是否有编译错误
   - 解决任何依赖问题

2. **硬件连接**：
   - 连接WCMCU-230模块
   - 安装终端电阻
   - 检查电源和地线

3. **功能测试**：
   - 下载程序到STM32
   - 观察串口输出
   - 验证双向通信

4. **性能优化**：
   - 监控通信统计
   - 调整发送周期
   - 优化错误处理

---

## 📞 **技术支持**

如果在配置过程中遇到问题：

1. **检查STM32CubeIDE版本**（推荐1.13.x或更高）
2. **参考STM32F407数据手册**
3. **查看CAN外设应用笔记**
4. **检查引脚复用表**

---

*本指南基于STM32F407ZGT6和STM32CubeIDE 1.13.x编写*
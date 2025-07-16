# STM32F407 + MCP2515 CAN通信工程创建详细步骤

## 硬件连接说明

### MCP2515+TJA1050模块与STM32F407ZGT6连接

**SPI接口连接：**
- MCP2515的SCK → STM32的PB3 (SPI1_SCK)
- MCP2515的SI (MOSI) → STM32的PB5 (SPI1_MOSI) 
- MCP2515的SO (MISO) → STM32的PB4 (SPI1_MISO)
- MCP2515的CS → STM32的PB12 (GPIO输出)

**中断引脚连接：**
- MCP2515的INT → STM32的PB10 (GPIO输入，外部中断)

**电源连接：**
- MCP2515的VCC → STM32的3.3V
- MCP2515的GND → STM32的GND

## STM32CubeIDE工程创建详细步骤

### 第一步：启动STM32CubeIDE并创建新工程

1. **打开STM32CubeIDE**
   - 双击桌面上的STM32CubeIDE图标
   - 等待软件完全加载

2. **创建新工程**
   - 点击菜单栏 `File` → `New` → `STM32 Project`
   - 或者直接点击欢迎页面的 `Start new STM32 project`

### 第二步：选择目标芯片

1. **在MCU/MPU Selector页面：**
   - 在搜索框中输入：`STM32F407ZGT6`
   - 在搜索结果中找到并点击 `STM32F407ZGTx`
   - 确认右侧显示的芯片信息正确
   - 点击 `Next >` 按钮

### 第三步：配置工程信息

1. **在Project Setup页面：**
   - **Project Name**: 输入 `CAN_BOX_MCP2515`
   - **Project Location**: 保持默认或选择您的工作目录
   - **Targeted Language**: 选择 `C`
   - **Targeted Binary Type**: 选择 `Executable`
   - **Targeted Project Type**: 选择 `STM32Cube`
   - 点击 `Finish` 按钮

### 第四步：配置时钟系统

1. **打开Clock Configuration页面：**
   - 在CubeMX界面中，点击顶部的 `Clock Configuration` 标签

2. **配置系统时钟：**
   - **HSE (High Speed External)**: 设置为 `8` MHz（正点原子板载晶振）
   - **PLL Source Mux**: 选择 `HSE`
   - **System Clock Mux**: 选择 `PLLCLK`
   - **HCLK (AHB Clock)**: 设置为 `168` MHz
   - **APB1 Clock**: 设置为 `42` MHz
   - **APB2 Clock**: 设置为 `84` MHz

3. **验证时钟配置：**
   - 确保所有时钟频率都在允许范围内（绿色显示）
   - 如有红色警告，请调整相应的分频系数

### 第五步：配置GPIO引脚

1. **打开Pinout & Configuration页面：**
   - 点击顶部的 `Pinout & Configuration` 标签

2. **配置SPI1引脚：**
   - 找到并点击 `PB3` 引脚
   - 在弹出菜单中选择 `SPI1_SCK`
   - 找到并点击 `PB4` 引脚
   - 选择 `SPI1_MISO`
   - 找到并点击 `PB5` 引脚
   - 选择 `SPI1_MOSI`

3. **配置CS片选引脚：**
   - 找到并点击 `PB12` 引脚
   - 选择 `GPIO_Output`
   - 在右侧配置面板中：
     - **GPIO output level**: 选择 `High`
     - **GPIO mode**: 选择 `Output Push Pull`
     - **GPIO Pull-up/Pull-down**: 选择 `No pull-up and no pull-down`
     - **Maximum output speed**: 选择 `High`
     - **User Label**: 输入 `MCP2515_CS`

4. **配置中断引脚：**
   - 找到并点击 `PB10` 引脚
   - 选择 `GPIO_EXTI10`
   - 在右侧配置面板中：
     - **GPIO mode**: 选择 `External Interrupt Mode with Falling edge trigger detection`
     - **GPIO Pull-up/Pull-down**: 选择 `Pull-up`
     - **User Label**: 输入 `MCP2515_INT`

### 第六步：配置SPI1外设

1. **在左侧Categories面板中：**
   - 展开 `Connectivity`
   - 点击 `SPI1`

2. **SPI1基本配置：**
   - **Mode**: 选择 `Full-Duplex Master`
   - **Hardware NSS Signal**: 选择 `Disable`

3. **SPI1参数配置：**
   - **Prescaler**: 选择 `32` (确保SPI时钟不超过10MHz)
   - **Clock Polarity (CPOL)**: 选择 `Low`
   - **Clock Phase (CPHA)**: 选择 `1 Edge`
   - **Data Size**: 选择 `8 Bits`
   - **First Bit**: 选择 `MSB First`

### 第七步：配置外部中断

1. **在左侧Categories面板中：**
   - 展开 `System Core`
   - 点击 `NVIC`

2. **使能外部中断：**
   - 找到 `EXTI line[15:10] interrupts`
   - 勾选 `Enabled` 复选框
   - **Preemption Priority**: 设置为 `1`
   - **Sub Priority**: 设置为 `0`

### 第八步：配置FreeRTOS（可选，推荐）

1. **在左侧Categories面板中：**
   - 展开 `Middleware`
   - 点击 `FREERTOS`

2. **FreeRTOS基本配置：**
   - **Interface**: 选择 `CMSIS_V2`
   - 勾选 `Enabled`

3. **创建任务：**
   - 在 `Tasks and Queues` 标签中
   - 点击 `Add` 按钮创建新任务
   
   **CAN发送任务：**
   - **Task Name**: `CANSendTask`
   - **Priority**: `osPriorityNormal`
   - **Stack Size**: `512`
   - **Entry Function**: `StartCANSendTask`
   
   **CAN接收任务：**
   - 再次点击 `Add` 按钮
   - **Task Name**: `CANReceiveTask`
   - **Priority**: `osPriorityNormal`
   - **Stack Size**: `512`
   - **Entry Function**: `StartCANReceiveTask`

4. **配置队列（用于任务间通信）：**
   - 点击 `Queues` 标签
   - 点击 `Add` 按钮
   - **Queue Name**: `CANRxQueue`
   - **Queue Size**: `10`
   - **Item Size**: `13` (CAN帧大小)

### 第九步：配置调试接口

1. **在左侧Categories面板中：**
   - 展开 `System Core`
   - 点击 `SYS`

2. **调试配置：**
   - **Debug**: 选择 `Serial Wire`
   - **Timebase Source**: 选择 `TIM1`

### 第十步：生成代码

1. **保存配置：**
   - 按 `Ctrl+S` 或点击 `File` → `Save`

2. **生成代码：**
   - 点击顶部工具栏的 `Generate Code` 按钮（齿轮图标）
   - 或者按 `Alt+K`

3. **代码生成选项：**
   - 在弹出的对话框中，确保以下选项被选中：
     - `Copy only the necessary library files`
     - `Generate peripheral initialization as a pair of '.c/.h' files per peripheral`
   - 点击 `Generate` 按钮

4. **等待代码生成完成：**
   - 生成完成后会提示是否打开项目
   - 点击 `Yes` 打开项目

## 工程创建完成检查清单

✅ **硬件连接确认：**
- [ ] SPI1引脚配置正确（PB3/PB4/PB5）
- [ ] CS引脚配置为GPIO输出（PB12）
- [ ] INT引脚配置为外部中断（PB10）

✅ **软件配置确认：**
- [ ] 系统时钟配置为168MHz
- [ ] SPI1配置为主模式，时钟不超过10MHz
- [ ] 外部中断EXTI10已使能
- [ ] FreeRTOS任务和队列已创建

✅ **代码生成确认：**
- [ ] 代码生成无错误
- [ ] 项目在STM32CubeIDE中正常打开
- [ ] 所有必要的驱动文件已生成

## 下一步

工程创建完成后，需要：
1. 添加MCP2515驱动库
2. 实现CAN通信协议栈
3. 编写应用层代码
4. 测试和调试

**注意：** 如果在创建过程中遇到任何问题，请检查：
- STM32CubeIDE版本是否为最新
- 芯片型号选择是否正确
- 引脚配置是否有冲突
- 时钟配置是否合理

---

*本文档基于STM32CubeIDE 1.13.x版本编写，不同版本界面可能略有差异*
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515.c
  * @brief          : MCP2515 CAN控制器驱动实现文件
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本驱动实现了MCP2515 CAN控制器的完整功能，包括：
  * 1. SPI底层通信
  * 2. 寄存器读写操作
  * 3. CAN控制器初始化和配置
  * 4. CAN消息的发送和接收
  * 5. 中断处理和状态查询
  * 6. 错误处理和调试功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "mcp2515.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define MCP2515_SPI_TIMEOUT     1000    // SPI通信超时时间(ms)
#define MCP2515_INIT_TIMEOUT    100     // 初始化超时时间(ms)
#define MCP2515_MODE_TIMEOUT    50      // 模式切换超时时间(ms)

/* Private variables ---------------------------------------------------------*/
static uint8_t mcp2515_initialized = 0;  // 初始化标志

/* 波特率配置表 (基于8MHz晶振) */
static const uint8_t mcp2515_baud_config[4][3] = {
    // CNF1, CNF2, CNF3
    {0x03, 0xFA, 0x87},  // 125Kbps
    {0x01, 0xFA, 0x87},  // 250Kbps
    {0x00, 0xFA, 0x87},  // 500Kbps
    {0x00, 0xD0, 0x82}   // 1Mbps
};

/* Private function prototypes -----------------------------------------------*/
static uint8_t MCP2515_WaitForMode(uint8_t mode, uint32_t timeout);
static uint8_t MCP2515_GetTxBuffer(void);
static void MCP2515_LoadTxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message);
static void MCP2515_ReadRxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message);

/* 底层SPI通信函数 -----------------------------------------------------------*/

/**
  * @brief  SPI读写一个字节
  * @param  data: 要发送的数据
  * @retval 接收到的数据
  */
uint8_t MCP2515_SPI_ReadWrite(uint8_t data)
{
    uint8_t rx_data = 0;
    
    // 使用HAL库进行SPI通信
    if (HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, MCP2515_SPI_TIMEOUT) != HAL_OK) {
        // SPI通信失败，返回0xFF表示错误
        return 0xFF;
    }
    
    return rx_data;
}

/**
  * @brief  拉低MCP2515片选信号
  * @param  None
  * @retval None
  */
void MCP2515_CS_Low(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  拉高MCP2515片选信号
  * @param  None
  * @retval None
  */
void MCP2515_CS_High(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
}

/* 寄存器读写函数 ------------------------------------------------------------*/

/**
  * @brief  读取MCP2515寄存器
  * @param  address: 寄存器地址
  * @retval 寄存器值
  */
uint8_t MCP2515_ReadRegister(uint8_t address)
{
    uint8_t data;
    
    MCP2515_CS_Low();                           // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_READ);    // 发送读指令
    MCP2515_SPI_ReadWrite(address);             // 发送寄存器地址
    data = MCP2515_SPI_ReadWrite(0x00);         // 读取数据
    MCP2515_CS_High();                          // 拉高片选
    
    return data;
}

/**
  * @brief  写入MCP2515寄存器
  * @param  address: 寄存器地址
  * @param  data: 要写入的数据
  * @retval None
  */
void MCP2515_WriteRegister(uint8_t address, uint8_t data)
{
    MCP2515_CS_Low();                           // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_WRITE);   // 发送写指令
    MCP2515_SPI_ReadWrite(address);             // 发送寄存器地址
    MCP2515_SPI_ReadWrite(data);                // 发送数据
    MCP2515_CS_High();                          // 拉高片选
}

/**
  * @brief  修改MCP2515寄存器的指定位
  * @param  address: 寄存器地址
  * @param  mask: 位掩码
  * @param  data: 新的位值
  * @retval None
  */
void MCP2515_ModifyRegister(uint8_t address, uint8_t mask, uint8_t data)
{
    MCP2515_CS_Low();                               // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_BIT_MODIFY);  // 发送位修改指令
    MCP2515_SPI_ReadWrite(address);                 // 发送寄存器地址
    MCP2515_SPI_ReadWrite(mask);                    // 发送位掩码
    MCP2515_SPI_ReadWrite(data);                    // 发送新数据
    MCP2515_CS_High();                              // 拉高片选
}

/**
  * @brief  读取多个连续寄存器
  * @param  address: 起始寄存器地址
  * @param  buffer: 数据缓冲区
  * @param  length: 读取长度
  * @retval None
  */
void MCP2515_ReadMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    
    MCP2515_CS_Low();                           // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_READ);    // 发送读指令
    MCP2515_SPI_ReadWrite(address);             // 发送起始地址
    
    for (i = 0; i < length; i++) {
        buffer[i] = MCP2515_SPI_ReadWrite(0x00); // 连续读取数据
    }
    
    MCP2515_CS_High();                          // 拉高片选
}

/**
  * @brief  写入多个连续寄存器
  * @param  address: 起始寄存器地址
  * @param  buffer: 数据缓冲区
  * @param  length: 写入长度
  * @retval None
  */
void MCP2515_WriteMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    
    MCP2515_CS_Low();                           // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_WRITE);   // 发送写指令
    MCP2515_SPI_ReadWrite(address);             // 发送起始地址
    
    for (i = 0; i < length; i++) {
        MCP2515_SPI_ReadWrite(buffer[i]);       // 连续写入数据
    }
    
    MCP2515_CS_High();                          // 拉高片选
}

/* 基本控制函数 --------------------------------------------------------------*/

/**
  * @brief  复位MCP2515
  * @param  None
  * @retval None
  */
void MCP2515_Reset(void)
{
    MCP2515_CS_Low();                           // 拉低片选
    MCP2515_SPI_ReadWrite(MCP2515_CMD_RESET);   // 发送复位指令
    MCP2515_CS_High();                          // 拉高片选
    
    osDelay(10);                                // 等待复位完成
}

/**
  * @brief  设置MCP2515工作模式
  * @param  mode: 工作模式
  * @retval MCP2515_OK: 成功, MCP2515_TIMEOUT: 超时
  */
uint8_t MCP2515_SetMode(uint8_t mode)
{
    // 修改CANCTRL寄存器的模式位
    MCP2515_ModifyRegister(MCP2515_CANCTRL, 0xE0, mode);
    
    // 等待模式切换完成
    return MCP2515_WaitForMode(mode, MCP2515_MODE_TIMEOUT);
}

/**
  * @brief  获取MCP2515当前工作模式
  * @param  None
  * @retval 当前工作模式
  */
uint8_t MCP2515_GetMode(void)
{
    uint8_t mode = MCP2515_ReadRegister(MCP2515_CANSTAT);
    return (mode & 0xE0);  // 返回模式位
}

/**
  * @brief  设置CAN波特率
  * @param  baudrate: 波特率选择 (MCP2515_BAUD_125K ~ MCP2515_BAUD_1000K)
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_SetBaudRate(uint8_t baudrate)
{
    if (baudrate > MCP2515_BAUD_1000K) {
        return MCP2515_ERROR;  // 无效的波特率参数
    }
    
    // 必须在配置模式下设置波特率
    if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    // 写入波特率配置寄存器
    MCP2515_WriteRegister(MCP2515_CNF1, mcp2515_baud_config[baudrate][0]);
    MCP2515_WriteRegister(MCP2515_CNF2, mcp2515_baud_config[baudrate][1]);
    MCP2515_WriteRegister(MCP2515_CNF3, mcp2515_baud_config[baudrate][2]);
    
    return MCP2515_OK;
}

/* 初始化和配置函数 ----------------------------------------------------------*/

/**
  * @brief  初始化MCP2515
  * @param  baudrate: CAN波特率
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_Init(uint8_t baudrate)
{
    // 复位MCP2515
    MCP2515_Reset();
    
    // 检查MCP2515是否响应
    if (MCP2515_SelfTest() != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    // 设置为配置模式
    if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    // 设置波特率
    if (MCP2515_SetBaudRate(baudrate) != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    // 配置接收缓冲区控制寄存器
    MCP2515_WriteRegister(MCP2515_RXB0CTRL, 0x60);  // 接收所有消息
    MCP2515_WriteRegister(MCP2515_RXB1CTRL, 0x60);  // 接收所有消息
    
    // 清除所有中断标志
    MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
    
    // 启用接收中断
    MCP2515_WriteRegister(MCP2515_CANINTE, MCP2515_INT_RX0IF | MCP2515_INT_RX1IF);
    
    // 切换到正常模式
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    mcp2515_initialized = 1;  // 设置初始化标志
    
    return MCP2515_OK;
}

/**
  * @brief  配置MCP2515中断
  * @param  interrupts: 中断使能位
  * @retval MCP2515_OK: 成功
  */
uint8_t MCP2515_ConfigureInterrupts(uint8_t interrupts)
{
    MCP2515_WriteRegister(MCP2515_CANINTE, interrupts);
    return MCP2515_OK;
}

/**
  * @brief  设置接收过滤器
  * @param  filter_num: 过滤器编号 (0-5)
  * @param  filter_id: 过滤器ID
  * @param  extended: 0=标准帧, 1=扩展帧
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_SetFilter(uint8_t filter_num, uint32_t filter_id, uint8_t extended)
{
    uint8_t sidh, sidl, eid8, eid0;
    uint8_t filter_regs[6][4] = {
        {0x00, 0x01, 0x02, 0x03},  // RXF0
        {0x04, 0x05, 0x06, 0x07},  // RXF1
        {0x08, 0x09, 0x0A, 0x0B},  // RXF2
        {0x10, 0x11, 0x12, 0x13},  // RXF3
        {0x14, 0x15, 0x16, 0x17},  // RXF4
        {0x18, 0x19, 0x1A, 0x1B}   // RXF5
    };
    
    if (filter_num > 5) {
        return MCP2515_ERROR;
    }
    
    // 必须在配置模式下设置过滤器
    uint8_t current_mode = MCP2515_GetMode();
    if (current_mode != MCP2515_MODE_CONFIG) {
        if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
            return MCP2515_ERROR;
        }
    }
    
    if (extended) {
        // 扩展帧ID配置
        sidh = (uint8_t)(filter_id >> 21);
        sidl = (uint8_t)(((filter_id >> 18) & 0x07) << 5) | 0x08 | (uint8_t)((filter_id >> 16) & 0x03);
        eid8 = (uint8_t)(filter_id >> 8);
        eid0 = (uint8_t)filter_id;
    } else {
        // 标准帧ID配置
        sidh = (uint8_t)(filter_id >> 3);
        sidl = (uint8_t)((filter_id & 0x07) << 5);
        eid8 = 0;
        eid0 = 0;
    }
    
    // 写入过滤器寄存器
    MCP2515_WriteRegister(filter_regs[filter_num][0], sidh);
    MCP2515_WriteRegister(filter_regs[filter_num][1], sidl);
    MCP2515_WriteRegister(filter_regs[filter_num][2], eid8);
    MCP2515_WriteRegister(filter_regs[filter_num][3], eid0);
    
    // 恢复原来的模式
    if (current_mode != MCP2515_MODE_CONFIG) {
        MCP2515_SetMode(current_mode);
    }
    
    return MCP2515_OK;
}

/**
  * @brief  设置接收掩码
  * @param  mask_num: 掩码编号 (0-1)
  * @param  mask_value: 掩码值
  * @param  extended: 0=标准帧, 1=扩展帧
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_SetMask(uint8_t mask_num, uint32_t mask_value, uint8_t extended)
{
    uint8_t sidh, sidl, eid8, eid0;
    uint8_t mask_regs[2][4] = {
        {0x20, 0x21, 0x22, 0x23},  // RXM0
        {0x24, 0x25, 0x26, 0x27}   // RXM1
    };
    
    if (mask_num > 1) {
        return MCP2515_ERROR;
    }
    
    // 必须在配置模式下设置掩码
    uint8_t current_mode = MCP2515_GetMode();
    if (current_mode != MCP2515_MODE_CONFIG) {
        if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
            return MCP2515_ERROR;
        }
    }
    
    if (extended) {
        // 扩展帧掩码配置
        sidh = (uint8_t)(mask_value >> 21);
        sidl = (uint8_t)(((mask_value >> 18) & 0x07) << 5) | 0x08 | (uint8_t)((mask_value >> 16) & 0x03);
        eid8 = (uint8_t)(mask_value >> 8);
        eid0 = (uint8_t)mask_value;
    } else {
        // 标准帧掩码配置
        sidh = (uint8_t)(mask_value >> 3);
        sidl = (uint8_t)((mask_value & 0x07) << 5);
        eid8 = 0;
        eid0 = 0;
    }
    
    // 写入掩码寄存器
    MCP2515_WriteRegister(mask_regs[mask_num][0], sidh);
    MCP2515_WriteRegister(mask_regs[mask_num][1], sidl);
    MCP2515_WriteRegister(mask_regs[mask_num][2], eid8);
    MCP2515_WriteRegister(mask_regs[mask_num][3], eid0);
    
    // 恢复原来的模式
    if (current_mode != MCP2515_MODE_CONFIG) {
        MCP2515_SetMode(current_mode);
    }
    
    return MCP2515_OK;
}

/* CAN消息收发函数 -----------------------------------------------------------*/

/**
  * @brief  发送CAN消息
  * @param  message: CAN消息结构体指针
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败, MCP2515_TIMEOUT: 超时
  */
uint8_t MCP2515_SendMessage(MCP2515_CANMessage_t *message)
{
    uint8_t buffer;
    uint32_t timeout = 0;
    
    if (!mcp2515_initialized || message == NULL) {
        return MCP2515_ERROR;
    }
    
    // 查找空闲的发送缓冲区
    buffer = MCP2515_GetTxBuffer();
    if (buffer == 0xFF) {
        return MCP2515_ERROR;  // 没有空闲的发送缓冲区
    }
    
    // 加载消息到发送缓冲区
    MCP2515_LoadTxBuffer(buffer, message);
    
    // 请求发送
    MCP2515_CS_Low();
    MCP2515_SPI_ReadWrite(MCP2515_CMD_RTS | (1 << buffer));
    MCP2515_CS_High();
    
    // 等待发送完成
    while (timeout < 1000) {
        uint8_t status = MCP2515_GetInterruptFlags();
        if (status & (MCP2515_INT_TX0IF << buffer)) {
            // 清除发送完成中断标志
            MCP2515_ClearInterruptFlags(MCP2515_INT_TX0IF << buffer);
            return MCP2515_OK;
        }
        osDelay(1);
        timeout++;
    }
    
    return MCP2515_TIMEOUT;
}

/**
  * @brief  接收CAN消息
  * @param  message: CAN消息结构体指针
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_ReceiveMessage(MCP2515_CANMessage_t *message)
{
    uint8_t status;
    
    if (!mcp2515_initialized || message == NULL) {
        return MCP2515_ERROR;
    }
    
    status = MCP2515_GetInterruptFlags();
    
    if (status & MCP2515_INT_RX0IF) {
        // 从接收缓冲区0读取消息
        MCP2515_ReadRxBuffer(0, message);
        MCP2515_ClearInterruptFlags(MCP2515_INT_RX0IF);
        return MCP2515_OK;
    } else if (status & MCP2515_INT_RX1IF) {
        // 从接收缓冲区1读取消息
        MCP2515_ReadRxBuffer(1, message);
        MCP2515_ClearInterruptFlags(MCP2515_INT_RX1IF);
        return MCP2515_OK;
    }
    
    return MCP2515_ERROR;  // 没有接收到消息
}

/**
  * @brief  检查是否有消息接收
  * @param  None
  * @retval 1: 有消息, 0: 无消息
  */
uint8_t MCP2515_CheckReceive(void)
{
    uint8_t status = MCP2515_GetInterruptFlags();
    return (status & (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF)) ? 1 : 0;
}

/**
  * @brief  检查发送缓冲区状态
  * @param  None
  * @retval 发送缓冲区空闲数量
  */
uint8_t MCP2515_CheckTransmit(void)
{
    uint8_t status = MCP2515_GetStatus();
    uint8_t free_buffers = 0;
    
    if (!(status & 0x04)) free_buffers++;  // TXB0空闲
    if (!(status & 0x10)) free_buffers++;  // TXB1空闲
    if (!(status & 0x40)) free_buffers++;  // TXB2空闲
    
    return free_buffers;
}

/* 中断处理函数 --------------------------------------------------------------*/

/**
  * @brief  获取中断标志
  * @param  None
  * @retval 中断标志寄存器值
  */
uint8_t MCP2515_GetInterruptFlags(void)
{
    return MCP2515_ReadRegister(MCP2515_CANINTF);
}

/**
  * @brief  清除中断标志
  * @param  flags: 要清除的中断标志
  * @retval None
  */
void MCP2515_ClearInterruptFlags(uint8_t flags)
{
    MCP2515_ModifyRegister(MCP2515_CANINTF, flags, 0x00);
}

/**
  * @brief  MCP2515中断处理函数
  * @param  None
  * @retval None
  * @note   此函数应在外部中断服务程序中调用
  */
void MCP2515_IRQHandler(void)
{
    uint8_t interrupt_flags = MCP2515_GetInterruptFlags();
    
    // 处理接收中断
    if (interrupt_flags & (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF)) {
        // 通知接收任务有新消息
        // 这里可以发送信号量或消息队列通知
        // 具体实现根据应用需求而定
    }
    
    // 处理发送完成中断
    if (interrupt_flags & (MCP2515_INT_TX0IF | MCP2515_INT_TX1IF | MCP2515_INT_TX2IF)) {
        // 发送完成处理
        // 可以通知发送任务或更新发送状态
    }
    
    // 处理错误中断
    if (interrupt_flags & MCP2515_INT_ERRIF) {
        // 错误处理
        uint8_t error_flags = MCP2515_GetErrorFlags();
        // 根据错误类型进行相应处理
        MCP2515_ClearErrorFlags();
    }
    
    // 清除已处理的中断标志
    MCP2515_ClearInterruptFlags(interrupt_flags);
}

/* 状态查询函数 --------------------------------------------------------------*/

/**
  * @brief  获取MCP2515状态
  * @param  None
  * @retval 状态寄存器值
  */
uint8_t MCP2515_GetStatus(void)
{
    uint8_t status;
    
    MCP2515_CS_Low();
    MCP2515_SPI_ReadWrite(MCP2515_CMD_READ_STATUS);
    status = MCP2515_SPI_ReadWrite(0x00);
    MCP2515_CS_High();
    
    return status;
}

/**
  * @brief  获取错误标志
  * @param  None
  * @retval 错误标志寄存器值
  */
uint8_t MCP2515_GetErrorFlags(void)
{
    return MCP2515_ReadRegister(MCP2515_EFLG);
}

/**
  * @brief  清除错误标志
  * @param  None
  * @retval None
  */
void MCP2515_ClearErrorFlags(void)
{
    MCP2515_WriteRegister(MCP2515_EFLG, 0x00);
}

/* 调试和测试函数 ------------------------------------------------------------*/

/**
  * @brief  MCP2515自检测试
  * @param  None
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_SelfTest(void)
{
    uint8_t test_data = 0xAA;
    uint8_t read_data;
    
    // 写入测试数据到一个可读写的寄存器
    MCP2515_WriteRegister(MCP2515_CNF1, test_data);
    
    // 读回数据进行比较
    read_data = MCP2515_ReadRegister(MCP2515_CNF1);
    
    if (read_data == test_data) {
        // 再次测试不同的数据
        test_data = 0x55;
        MCP2515_WriteRegister(MCP2515_CNF1, test_data);
        read_data = MCP2515_ReadRegister(MCP2515_CNF1);
        
        if (read_data == test_data) {
            return MCP2515_OK;
        }
    }
    
    return MCP2515_ERROR;
}

/**
  * @brief  打印MCP2515状态信息
  * @param  None
  * @retval None
  */
void MCP2515_PrintStatus(void)
{
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    uint8_t canctrl = MCP2515_ReadRegister(MCP2515_CANCTRL);
    uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    
    printf("MCP2515 Status:\r\n");
    printf("CANSTAT: 0x%02X\r\n", canstat);
    printf("CANCTRL: 0x%02X\r\n", canctrl);
    printf("CANINTF: 0x%02X\r\n", canintf);
    printf("EFLG: 0x%02X\r\n", eflg);
    printf("Mode: ");
    
    switch (canstat & 0xE0) {
        case MCP2515_MODE_NORMAL:
            printf("Normal\r\n");
            break;
        case MCP2515_MODE_SLEEP:
            printf("Sleep\r\n");
            break;
        case MCP2515_MODE_LOOPBACK:
            printf("Loopback\r\n");
            break;
        case MCP2515_MODE_LISTENONLY:
            printf("Listen Only\r\n");
            break;
        case MCP2515_MODE_CONFIG:
            printf("Configuration\r\n");
            break;
        default:
            printf("Unknown\r\n");
            break;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  等待模式切换完成
  * @param  mode: 目标模式
  * @param  timeout: 超时时间(ms)
  * @retval MCP2515_OK: 成功, MCP2515_TIMEOUT: 超时
  */
static uint8_t MCP2515_WaitForMode(uint8_t mode, uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    
    while ((HAL_GetTick() - start_time) < timeout) {
        if (MCP2515_GetMode() == mode) {
            return MCP2515_OK;
        }
        osDelay(1);
    }
    
    return MCP2515_TIMEOUT;
}

/**
  * @brief  获取空闲的发送缓冲区
  * @param  None
  * @retval 缓冲区编号 (0-2), 0xFF表示无空闲缓冲区
  */
static uint8_t MCP2515_GetTxBuffer(void)
{
    uint8_t status = MCP2515_GetStatus();
    
    if (!(status & 0x04)) return 0;  // TXB0空闲
    if (!(status & 0x10)) return 1;  // TXB1空闲
    if (!(status & 0x40)) return 2;  // TXB2空闲
    
    return 0xFF;  // 无空闲缓冲区
}

/**
  * @brief  加载消息到发送缓冲区
  * @param  buffer: 缓冲区编号 (0-2)
  * @param  message: CAN消息指针
  * @retval None
  */
static void MCP2515_LoadTxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message)
{
    uint8_t sidh, sidl, eid8, eid0, dlc;
    uint8_t base_addr = 0x30 + (buffer * 0x10);  // 计算缓冲区基地址
    uint8_t i;
    
    // 准备ID寄存器值
    if (message->ide) {
        // 扩展帧
        sidh = (uint8_t)(message->id >> 21);
        sidl = (uint8_t)(((message->id >> 18) & 0x07) << 5) | 0x08 | (uint8_t)((message->id >> 16) & 0x03);
        eid8 = (uint8_t)(message->id >> 8);
        eid0 = (uint8_t)message->id;
    } else {
        // 标准帧
        sidh = (uint8_t)(message->id >> 3);
        sidl = (uint8_t)((message->id & 0x07) << 5);
        eid8 = 0;
        eid0 = 0;
    }
    
    // 准备DLC寄存器值
    dlc = message->dlc & 0x0F;
    if (message->rtr) {
        dlc |= 0x40;  // 设置RTR位
    }
    
    // 写入ID和控制信息
    MCP2515_WriteRegister(base_addr + 1, sidh);  // SIDH
    MCP2515_WriteRegister(base_addr + 2, sidl);  // SIDL
    MCP2515_WriteRegister(base_addr + 3, eid8);  // EID8
    MCP2515_WriteRegister(base_addr + 4, eid0);  // EID0
    MCP2515_WriteRegister(base_addr + 5, dlc);   // DLC
    
    // 写入数据
    for (i = 0; i < message->dlc && i < 8; i++) {
        MCP2515_WriteRegister(base_addr + 6 + i, message->data[i]);
    }
}

/**
  * @brief  从接收缓冲区读取消息
  * @param  buffer: 缓冲区编号 (0-1)
  * @param  message: CAN消息指针
  * @retval None
  */
static void MCP2515_ReadRxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message)
{
    uint8_t sidh, sidl, eid8, eid0, dlc;
    uint8_t base_addr = 0x60 + (buffer * 0x10);  // 计算缓冲区基地址
    uint8_t i;
    
    // 读取ID和控制信息
    sidh = MCP2515_ReadRegister(base_addr + 1);  // SIDH
    sidl = MCP2515_ReadRegister(base_addr + 2);  // SIDL
    eid8 = MCP2515_ReadRegister(base_addr + 3);  // EID8
    eid0 = MCP2515_ReadRegister(base_addr + 4);  // EID0
    dlc = MCP2515_ReadRegister(base_addr + 5);   // DLC
    
    // 解析ID
    if (sidl & 0x08) {
        // 扩展帧
        message->ide = 1;
        message->id = ((uint32_t)sidh << 21) | 
                      ((uint32_t)(sidl & 0xE0) << 13) | 
                      ((uint32_t)(sidl & 0x03) << 16) | 
                      ((uint32_t)eid8 << 8) | 
                      eid0;
    } else {
        // 标准帧
        message->ide = 0;
        message->id = ((uint32_t)sidh << 3) | ((sidl & 0xE0) >> 5);
    }
    
    // 解析控制信息
    message->rtr = (dlc & 0x40) ? 1 : 0;
    message->dlc = dlc & 0x0F;
    
    // 读取数据
    for (i = 0; i < message->dlc && i < 8; i++) {
        message->data[i] = MCP2515_ReadRegister(base_addr + 6 + i);
    }
    
    // 清空剩余数据字节
    for (; i < 8; i++) {
        message->data[i] = 0;
    }
}
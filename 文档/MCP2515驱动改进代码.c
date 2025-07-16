/* MCP2515驱动改进代码片段 */
/* 用于替换原有的问题函数 */

#include "mcp2515.h"
#include <stdio.h>
#include <string.h>

/**
  * @brief  改进的SPI读写函数，增加详细错误处理
  * @param  data: 要发送的数据
  * @retval 接收到的数据，0xFF表示通信错误
  */
uint8_t MCP2515_SPI_ReadWrite_Enhanced(uint8_t data)
{
    uint8_t rx_data = 0;
    HAL_StatusTypeDef status;
    
    // 使用HAL库进行SPI通信
    status = HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, MCP2515_SPI_TIMEOUT);
    
    if (status != HAL_OK) {
        printf("SPI Error: Status=%d, TX=0x%02X\r\n", status, data);
        
        // 检查具体错误类型
        if (status == HAL_TIMEOUT) {
            printf("SPI Timeout - Check MISO connection and MCP2515 power\r\n");
        } else if (status == HAL_ERROR) {
            printf("SPI Hardware Error - Check SPI configuration\r\n");
            // 获取详细错误信息
            uint32_t error = HAL_SPI_GetError(&hspi1);
            printf("SPI Error Code: 0x%08lX\r\n", error);
        } else if (status == HAL_BUSY) {
            printf("SPI Busy - Previous operation not completed\r\n");
        }
        
        return 0xFF;  // 返回错误标志
    }
    
    return rx_data;
}

/**
  * @brief  改进的MCP2515复位函数，增加状态验证
  * @param  None
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_Reset_Enhanced(void)
{
    printf("\r\n=== MCP2515 Enhanced Reset ===\r\n");
    
    // 1. 发送复位命令
    printf("Step 1: Sending reset command...\r\n");
    MCP2515_CS_Low();
    printf("CS pulled low\r\n");
    
    uint8_t result = MCP2515_SPI_ReadWrite_Enhanced(MCP2515_CMD_RESET);
    printf("Reset command sent, SPI result: 0x%02X\r\n", result);
    
    MCP2515_CS_High();
    printf("CS pulled high\r\n");
    
    // 2. 等待复位完成
    printf("Step 2: Waiting for reset completion...\r\n");
    osDelay(20);  // 增加延时确保复位完成
    printf("Reset delay completed\r\n");
    
    // 3. 验证复位是否成功
    printf("Step 3: Verifying reset status...\r\n");
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    printf("CANSTAT after reset: 0x%02X (Expected: 0x80)\r\n", canstat);
    
    if (canstat == 0x80) {
        printf("✓ MCP2515 reset successful - Configuration mode detected\r\n");
        return MCP2515_OK;
    } else if (canstat == 0xFF) {
        printf("✗ MCP2515 reset failed - No SPI response (0xFF)\r\n");
        printf("  Possible causes:\r\n");
        printf("  - MISO line not connected\r\n");
        printf("  - MCP2515 power supply issue\r\n");
        printf("  - MCP2515 module damaged\r\n");
        return MCP2515_ERROR;
    } else {
        printf("✗ MCP2515 reset failed - Unexpected CANSTAT: 0x%02X\r\n", canstat);
        printf("  Possible causes:\r\n");
        printf("  - MCP2515 not in reset state\r\n");
        printf("  - Crystal oscillator issue\r\n");
        printf("  - SPI communication error\r\n");
        return MCP2515_ERROR;
    }
}

/**
  * @brief  MCP2515硬件连接测试函数
  * @param  None
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_HardwareTest(void)
{
    printf("\r\n=== MCP2515 Hardware Connection Test ===\r\n");
    
    // 1. CS引脚控制测试
    printf("Step 1: Testing CS pin control...\r\n");
    for (int i = 0; i < 3; i++) {
        MCP2515_CS_High();
        osDelay(1);
        MCP2515_CS_Low();
        osDelay(1);
    }
    MCP2515_CS_High();
    printf("✓ CS pin control test completed\r\n");
    
    // 2. SPI基础通信测试
    printf("Step 2: Testing basic SPI communication...\r\n");
    MCP2515_CS_Low();
    uint8_t dummy1 = MCP2515_SPI_ReadWrite_Enhanced(0x00);
    uint8_t dummy2 = MCP2515_SPI_ReadWrite_Enhanced(0xFF);
    MCP2515_CS_High();
    printf("SPI test results: 0x00->0x%02X, 0xFF->0x%02X\r\n", dummy1, dummy2);
    
    if (dummy1 == 0xFF && dummy2 == 0xFF) {
        printf("⚠ Warning: All SPI reads return 0xFF\r\n");
        printf("  This suggests MISO line issue or MCP2515 not responding\r\n");
    }
    
    // 3. 复位测试
    printf("Step 3: Testing MCP2515 reset...\r\n");
    if (MCP2515_Reset_Enhanced() != MCP2515_OK) {
        printf("✗ Hardware test failed at reset stage\r\n");
        return MCP2515_ERROR;
    }
    
    // 4. 寄存器读写测试
    printf("Step 4: Testing register read/write...\r\n");
    
    // 测试CNF1寄存器（可读写）
    uint8_t original = MCP2515_ReadRegister(MCP2515_CNF1);
    printf("CNF1 original value: 0x%02X\r\n", original);
    
    // 写入测试值
    uint8_t test_value = 0xAA;
    MCP2515_WriteRegister(MCP2515_CNF1, test_value);
    uint8_t read_back = MCP2515_ReadRegister(MCP2515_CNF1);
    printf("CNF1 write 0x%02X, read back 0x%02X\r\n", test_value, read_back);
    
    if (read_back == test_value) {
        printf("✓ Register write test 1 passed\r\n");
    } else {
        printf("✗ Register write test 1 failed\r\n");
        return MCP2515_ERROR;
    }
    
    // 测试另一个值
    test_value = 0x55;
    MCP2515_WriteRegister(MCP2515_CNF1, test_value);
    read_back = MCP2515_ReadRegister(MCP2515_CNF1);
    printf("CNF1 write 0x%02X, read back 0x%02X\r\n", test_value, read_back);
    
    if (read_back == test_value) {
        printf("✓ Register write test 2 passed\r\n");
    } else {
        printf("✗ Register write test 2 failed\r\n");
        return MCP2515_ERROR;
    }
    
    // 恢复原始值
    MCP2515_WriteRegister(MCP2515_CNF1, original);
    printf("CNF1 restored to original value: 0x%02X\r\n", original);
    
    printf("✓ All hardware tests passed!\r\n");
    return MCP2515_OK;
}

/**
  * @brief  MCP2515详细状态检查函数
  * @param  None
  * @retval None
  */
void MCP2515_DetailedStatusCheck(void)
{
    printf("\r\n=== MCP2515 Detailed Status Check ===\r\n");
    
    // 读取关键寄存器
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    uint8_t canctrl = MCP2515_ReadRegister(MCP2515_CANCTRL);
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    uint8_t caninte = MCP2515_ReadRegister(MCP2515_CANINTE);
    uint8_t tec = MCP2515_ReadRegister(MCP2515_TEC);
    uint8_t rec = MCP2515_ReadRegister(MCP2515_REC);
    
    printf("Register Status:\r\n");
    printf("  CANSTAT: 0x%02X (Mode: ", canstat);
    switch (canstat & 0xE0) {
        case 0x00: printf("Normal)"); break;
        case 0x20: printf("Sleep)"); break;
        case 0x40: printf("Loopback)"); break;
        case 0x60: printf("Listen-only)"); break;
        case 0x80: printf("Configuration)"); break;
        default: printf("Unknown)"); break;
    }
    printf("\r\n");
    
    printf("  CANCTRL: 0x%02X\r\n", canctrl);
    printf("  EFLG:    0x%02X", eflg);
    if (eflg != 0) {
        printf(" (Errors detected!)");
    }
    printf("\r\n");
    
    printf("  CANINTF: 0x%02X", canintf);
    if (canintf != 0) {
        printf(" (Interrupts pending!)");
    }
    printf("\r\n");
    
    printf("  CANINTE: 0x%02X\r\n", caninte);
    printf("  TEC:     %d (Transmit Error Count)\r\n", tec);
    printf("  REC:     %d (Receive Error Count)\r\n", rec);
    
    // 分析状态
    if (canstat == 0xFF) {
        printf("\r\n❌ Critical: No communication with MCP2515!\r\n");
    } else if ((canstat & 0xE0) == 0x80) {
        printf("\r\n✅ MCP2515 is in Configuration mode (ready for setup)\r\n");
    } else {
        printf("\r\n⚠ MCP2515 is not in Configuration mode\r\n");
    }
}

/**
  * @brief  改进的MCP2515初始化函数
  * @param  baudrate: CAN波特率
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_Init_Enhanced(uint8_t baudrate)
{
    printf("\r\n=== MCP2515 Enhanced Initialization ===\r\n");
    
    // 1. 硬件连接测试
    printf("Phase 1: Hardware connection test\r\n");
    if (MCP2515_HardwareTest() != MCP2515_OK) {
        printf("❌ Hardware test failed - Check connections\r\n");
        return MCP2515_ERROR;
    }
    
    // 2. 详细状态检查
    printf("\r\nPhase 2: Status verification\r\n");
    MCP2515_DetailedStatusCheck();
    
    // 3. 确保在配置模式
    printf("\r\nPhase 3: Mode configuration\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
        printf("❌ Failed to enter Configuration mode\r\n");
        return MCP2515_ERROR;
    }
    printf("✅ Successfully entered Configuration mode\r\n");
    
    // 4. 设置波特率
    printf("\r\nPhase 4: Baud rate configuration\r\n");
    if (MCP2515_SetBaudRate(baudrate) != MCP2515_OK) {
        printf("❌ Failed to set baud rate\r\n");
        return MCP2515_ERROR;
    }
    printf("✅ Baud rate configured successfully\r\n");
    
    // 5. 配置接收缓冲区
    printf("\r\nPhase 5: Receive buffer configuration\r\n");
    MCP2515_WriteRegister(MCP2515_RXB0CTRL, 0x60);  // 接收所有消息
    MCP2515_WriteRegister(MCP2515_RXB1CTRL, 0x60);  // 接收所有消息
    printf("✅ Receive buffers configured\r\n");
    
    // 6. 清除中断标志
    printf("\r\nPhase 6: Interrupt configuration\r\n");
    MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
    MCP2515_WriteRegister(MCP2515_CANINTE, MCP2515_INT_RX0IF | MCP2515_INT_RX1IF);
    printf("✅ Interrupts configured\r\n");
    
    // 7. 切换到正常模式
    printf("\r\nPhase 7: Switch to Normal mode\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        printf("❌ Failed to enter Normal mode\r\n");
        return MCP2515_ERROR;
    }
    printf("✅ Successfully entered Normal mode\r\n");
    
    // 8. 最终状态检查
    printf("\r\nPhase 8: Final status verification\r\n");
    MCP2515_DetailedStatusCheck();
    
    mcp2515_initialized = 1;  // 设置初始化标志
    printf("\r\n🎉 MCP2515 initialization completed successfully!\r\n");
    
    return MCP2515_OK;
}

/**
  * @brief  SPI速度测试函数
  * @param  None
  * @retval None
  */
void MCP2515_SPI_SpeedTest(void)
{
    printf("\r\n=== SPI Speed Test ===\r\n");
    
    uint32_t start_time = HAL_GetTick();
    
    // 执行100次寄存器读取
    for (int i = 0; i < 100; i++) {
        uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
        (void)canstat;  // 避免编译器警告
    }
    
    uint32_t end_time = HAL_GetTick();
    uint32_t duration = end_time - start_time;
    
    printf("100 register reads completed in %lu ms\r\n", duration);
    printf("Average time per read: %.2f ms\r\n", (float)duration / 100.0f);
    
    if (duration > 500) {
        printf("⚠ Warning: SPI communication is slow\r\n");
        printf("  Consider checking SPI clock configuration\r\n");
    } else {
        printf("✅ SPI communication speed is acceptable\r\n");
    }
}

/* 使用说明：
 * 1. 将 MCP2515_SPI_ReadWrite 替换为 MCP2515_SPI_ReadWrite_Enhanced
 * 2. 将 MCP2515_Reset 替换为 MCP2515_Reset_Enhanced  
 * 3. 将 MCP2515_Init 替换为 MCP2515_Init_Enhanced
 * 4. 在main函数中调用 MCP2515_HardwareTest() 进行硬件测试
 * 5. 使用 MCP2515_DetailedStatusCheck() 查看详细状态
 * 6. 使用 MCP2515_SPI_SpeedTest() 测试SPI通信速度
 */
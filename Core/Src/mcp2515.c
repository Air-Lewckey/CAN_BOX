/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515.c
  * @brief          : MCP2515 CAN controller driver implementation file
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This driver implements complete functionality for MCP2515 CAN controller, including:
  * 1. Low-level SPI communication
  * 2. Register read/write operations
  * 3. CAN controller initialization and configuration
  * 4. CAN message transmission and reception
  * 5. Interrupt handling and status query
  * 6. Error handling and debugging functions
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "mcp2515.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"  // For osDelay function in FreeRTOS environment

/* Private defines -----------------------------------------------------------*/
#define MCP2515_INIT_TIMEOUT    100     // Initialization timeout (ms)

/* Private variables ---------------------------------------------------------*/
static uint8_t mcp2515_initialized = 0;  // Initialization flag

/* Baud rate configuration table - Compatible with multiple crystal frequencies */
/* Optimized based on CAN1 actual configuration: Prescaler=6, TimeSeg1=11TQ, TimeSeg2=2TQ, 500Kbps */
static const uint8_t mcp2515_baud_config[4][3] = {
    // CNF1, CNF2, CNF3
    {0x07, 0xFA, 0x87},  // 125Kbps - Try original configuration
    {0x03, 0xFA, 0x87},  // 250Kbps - Try original configuration  
    {0x01, 0xFA, 0x87},  // 500Kbps - Try original configuration, may suit 8MHz crystal
    {0x00, 0xFA, 0x87}   // 1Mbps   - Try original configuration
};

/* Configuration notes: Multiple 500Kbps configurations for testing
 * CAN1 configuration: Prescaler=6, TimeSeg1=11TQ, TimeSeg2=2TQ, 500Kbps
 * Currently using original configuration {0x01, 0xFA, 0x87}, suitable for 8MHz crystal
 * If still not working, may need to try other configurations or check hardware connections
 */

/* Alternative 500Kbps configuration table for testing different crystal frequencies */
static const uint8_t mcp2515_500k_test_configs[][3] = {
    {0x01, 0xFA, 0x87},  // Config 1: 8MHz crystal standard configuration
    {0x00, 0xB5, 0x01},  // Config 2: 16MHz crystal configuration
    {0x00, 0x92, 0x01},  // Config 3: 16MHz crystal compact configuration
    {0x00, 0xAC, 0x01},  // Config 4: 16MHz crystal alternative configuration
    {0x01, 0xB5, 0x01},  // Config 5: 8MHz crystal relaxed configuration
};

/* Private function prototypes -----------------------------------------------*/
static uint8_t MCP2515_GetTxBuffer(void);
static void MCP2515_LoadTxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message);
static void MCP2515_ReadRxBuffer(uint8_t buffer, MCP2515_CANMessage_t *message);

/**
  * @brief  Configure MCP2515 filter settings to receive all messages
  * @param  None
  * @retval None
  */
void MCP2515_SetFilterForAll(void) {
    printf("[MCP2515-FILTER] Setting filters to accept all messages...\r\n");
    
    // Set receive masks to 0 (accept all IDs)
    MCP2515_WriteRegister(MCP2515_RXM0SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXM0SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXM1SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXM1SIDL, 0x00);
    
    // Set filters to 0 (accept all IDs)
    MCP2515_WriteRegister(MCP2515_RXF0SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF0SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF1SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF1SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF2SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF2SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF3SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF3SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF4SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF4SIDL, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF5SIDH, 0x00);
    MCP2515_WriteRegister(MCP2515_RXF5SIDL, 0x00);
    
    printf("[MCP2515-FILTER] All filters and masks set to accept all messages\r\n");
}

/**
  * @brief  Ensure MCP2515 is in normal operating mode
  * @param  None
  * @retval None
  */
void MCP2515_ModeNormal(void) {
    printf("[MCP2515-MODE] Setting to Normal mode...\r\n");
    MCP2515_BitModify(MCP2515_CANCTRL, 0xE0, 0x00);  // Clear top 3 bits (mode bits)
    osDelay(10);
    
    // Verify mode setting
    uint8_t mode = MCP2515_ReadRegister(MCP2515_CANCTRL) & 0xE0;
    if (mode == 0x00) {
        printf("[MCP2515-MODE] Successfully set to Normal mode\r\n");
    } else {
        printf("[MCP2515-MODE] Failed to set Normal mode, current mode: 0x%02X\r\n", mode);
    }
}

/**
  * @brief  Test different 500Kbps baud rate configurations
  * @param  None
  * @retval None
  */
void MCP2515_Test500KConfigs(void)
{
    printf("\r\n========== MCP2515 500Kbps Configuration Test ==========\r\n");
    
    for(int config_idx = 0; config_idx < 5; config_idx++) {
        printf("\r\nTesting Configuration %d: CNF1=0x%02X, CNF2=0x%02X, CNF3=0x%02X\r\n", 
               config_idx + 1,
               mcp2515_500k_test_configs[config_idx][0],
               mcp2515_500k_test_configs[config_idx][1], 
               mcp2515_500k_test_configs[config_idx][2]);
        
        // Switch to configuration mode
        if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
            printf("[ERROR] Failed to enter config mode\r\n");
            continue;
        }
        
        // Apply test configuration
        MCP2515_WriteRegister(MCP2515_CNF1, mcp2515_500k_test_configs[config_idx][0]);
        MCP2515_WriteRegister(MCP2515_CNF2, mcp2515_500k_test_configs[config_idx][1]);
        MCP2515_WriteRegister(MCP2515_CNF3, mcp2515_500k_test_configs[config_idx][2]);
        
        // Clear error flags
        MCP2515_WriteRegister(MCP2515_EFLG, 0x00);
        MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
        
        // Switch to normal mode
        if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
            printf("[ERROR] Failed to enter normal mode\r\n");
            continue;
        }
        
        printf("[INFO] Configuration applied, monitoring for 3 seconds...\r\n");
        
        // Monitor for 3 seconds
        uint32_t start_time = HAL_GetTick();
        uint8_t initial_rx_count = MCP2515_ReadRegister(MCP2515_REC);
        
        while((HAL_GetTick() - start_time) < 3000) {
            // Check if messages are received
            if(MCP2515_CheckReceive()) {
                printf("[SUCCESS] Messages detected with Configuration %d!\r\n", config_idx + 1);
                printf("[INFO] Use this configuration for optimal performance\r\n");
                return;
            }
            osDelay(100);
        }
        
        // Check error count
        uint8_t final_rx_count = MCP2515_ReadRegister(MCP2515_REC);
        uint8_t error_flags = MCP2515_ReadRegister(MCP2515_EFLG);
        
        printf("[RESULT] RX Error Count: %d -> %d, Error Flags: 0x%02X\r\n", 
               initial_rx_count, final_rx_count, error_flags);
        
        if(final_rx_count < initial_rx_count || final_rx_count < 10) {
            printf("[GOOD] This configuration shows improvement\r\n");
        } else {
            printf("[POOR] This configuration is not suitable\r\n");
        }
    }
    
    printf("\r\n[CONCLUSION] All configurations tested. Check results above.\r\n");
    printf("========================================================\r\n");
}

/* Low-level SPI communication functions -----------------------------------------------------------*/

/**
  * @brief  SPI read/write one byte
  * @param  data: Data to be sent
  * @retval Received data
  */
uint8_t MCP2515_SPI_ReadWrite(uint8_t data)
{
    uint8_t rx_data = 0;
    HAL_StatusTypeDef status;
    
    // Ensure SPI is not busy
    uint32_t timeout = HAL_GetTick() + 100;
    while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) && (HAL_GetTick() < timeout));
    
    // Clear possible error flags
    __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
    
    // Use HAL library for SPI communication
    status = HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, MCP2515_SPI_TIMEOUT);
    
    if (status == HAL_OK) {
        return rx_data;
    }
    
    // Error handling
    if (status == HAL_TIMEOUT) {
        printf("[MCP2515-SPI] Timeout - Check MISO connection\r\n");
    } else if (status == HAL_ERROR) {
        uint32_t error = HAL_SPI_GetError(&hspi1);
        printf("[MCP2515-SPI] Hardware Error - Code: 0x%08lX\r\n", error);
        
        // Try to reset SPI state
        __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
        
        // If serious error, try to reinitialize SPI
        if(error & HAL_SPI_ERROR_OVR) {
            printf("[MCP2515-SPI] Reinitializing SPI due to overrun error\r\n");
            HAL_SPI_DeInit(&hspi1);
            osDelay(1);
            HAL_SPI_Init(&hspi1);
        }
    } else if (status == HAL_BUSY) {
        printf("[MCP2515-SPI] Busy - Previous operation not completed\r\n");
    }
    
    return 0xFF;  // Communication failed
}

/*
// MCP2515 related functions commented out - Hardware removed
/*
  * @brief  Pull down MCP2515 chip select signal
  * @param  None
  * @retval None
  */
/*
void MCP2515_CS_Low(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET);
    // Add small delay to ensure CS signal stability
    for(volatile int i = 0; i < 10; i++);
}
*/

/*
  * @brief  Pull up MCP2515 chip select signal
  * @param  None
  * @retval None
  */
/*
void MCP2515_CS_High(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
    // 添加小延时确保CS信号稳定
    for(volatile int i = 0; i < 10; i++);
}
*/

/* Register read/write functions ------------------------------------------------------------*/

/**
  * @brief  Read MCP2515 register
  * @param  address: Register address
  * @retval Register value
  */
uint8_t MCP2515_ReadRegister(uint8_t address)
{
    uint8_t data;
    
    MCP2515_CS_Low();                           // Pull down chip select
    MCP2515_SPI_ReadWrite(MCP2515_CMD_READ);    // Send read command
    MCP2515_SPI_ReadWrite(address);             // Send register address
    data = MCP2515_SPI_ReadWrite(0x00);         // Read data
    MCP2515_CS_High();                          // Pull up chip select
    
    return data;
}

/**
  * @brief  Write to MCP2515 register
  * @param  address: Register address
  * @param  data: Data to be written
  * @retval None
  */
void MCP2515_WriteRegister(uint8_t address, uint8_t data)
{
    MCP2515_CS_Low();                           // Pull down chip select
    MCP2515_SPI_ReadWrite(MCP2515_CMD_WRITE);   // Send write command
    MCP2515_SPI_ReadWrite(address);             // Send register address
    MCP2515_SPI_ReadWrite(data);                // Send data
    MCP2515_CS_High();                          // Pull up chip select
}

/**
  * @brief  Modify specified bits of MCP2515 register
  * @param  address: Register address
  * @param  mask: Bit mask
  * @param  data: New bit values
  * @retval None
  */
void MCP2515_ModifyRegister(uint8_t address, uint8_t mask, uint8_t data)
{
    MCP2515_CS_Low();                               // Pull down chip select
    MCP2515_SPI_ReadWrite(MCP2515_CMD_BIT_MODIFY);  // Send bit modify command
    MCP2515_SPI_ReadWrite(address);                 // Send register address
    MCP2515_SPI_ReadWrite(mask);                    // Send bit mask
    MCP2515_SPI_ReadWrite(data);                    // Send new data
    MCP2515_CS_High();                              // Pull up chip select
}

/**
  * @brief  Bit modify MCP2515 register (same function as ModifyRegister)
  * @param  address: Register address
  * @param  mask: Bit mask
  * @param  data: New bit values
  * @retval None
  */
void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data)
{
    // Call ModifyRegister function, exactly the same functionality
    MCP2515_ModifyRegister(address, mask, data);
}

/**
  * @brief  Read multiple consecutive registers
  * @param  address: Starting register address
  * @param  buffer: Data buffer
  * @param  length: Read length
  * @retval None
  */
void MCP2515_ReadMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    
    MCP2515_CS_Low();                           // Pull down chip select
    MCP2515_SPI_ReadWrite(MCP2515_CMD_READ);    // Send read command
    MCP2515_SPI_ReadWrite(address);             // Send starting address
    
    for (i = 0; i < length; i++) {
        buffer[i] = MCP2515_SPI_ReadWrite(0x00); // Read data continuously
    }
    
    MCP2515_CS_High();                          // Pull up chip select
}

/**
  * @brief  Write to multiple consecutive registers
  * @param  address: Starting register address
  * @param  buffer: Data buffer
  * @param  length: Write length
  * @retval None
  */
void MCP2515_WriteMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length)
{
    uint8_t i;
    
    MCP2515_CS_Low();                           // Pull down chip select
    MCP2515_SPI_ReadWrite(MCP2515_CMD_WRITE);   // Send write command
    MCP2515_SPI_ReadWrite(address);             // Send starting address
    
    for (i = 0; i < length; i++) {
        MCP2515_SPI_ReadWrite(buffer[i]);       // Write data continuously
    }
    
    MCP2515_CS_High();                          // Pull up chip select
}

/* Basic control functions --------------------------------------------------------------*/

/**
  * @brief  Reset MCP2515
  * @param  None
  * @retval None
  */
void MCP2515_Reset(void)
{
    printf("[MCP2515-RESET] Starting MCP2515 reset...\r\n");
    
    // Ensure CS pin initial state is correct
    MCP2515_CS_High();
    osDelay(5);
    
    MCP2515_CS_Low();
    printf("[MCP2515-RESET] CS pulled low\r\n");
    
    uint8_t reset_result = MCP2515_SPI_ReadWrite(MCP2515_CMD_RESET);
    printf("[MCP2515-RESET] Reset command sent, SPI response: 0x%02X\r\n", reset_result);
    
    MCP2515_CS_High();
    printf("[MCP2515-RESET] CS pulled high\r\n");
    
    osDelay(50);  // Increase delay to ensure reset completion
    printf("[MCP2515-RESET] Reset delay completed\r\n");
    
    // 多次读取状态进行验证
    for(int retry = 0; retry < 5; retry++) {
        uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
        printf("[MCP2515-RESET] CANSTAT check #%d: 0x%02X\r\n", retry + 1, canstat);
        
        // 检查复位状态 - 允许多种有效状态
        if (canstat == 0x80) {
            printf("[MCP2515-RESET] Reset successful (Configuration mode)\r\n");
            return;
        } else if (canstat == 0x40) {
            printf("[MCP2515-RESET] Reset successful (Loopback mode detected)\r\n");
            return;
        } else if (canstat == 0x00) {
            printf("[MCP2515-RESET] Reset successful (Normal mode detected)\r\n");
            return;
        } else if (canstat == 0xFF) {
            printf("[MCP2515-RESET] No SPI response - Check MISO connection\r\n");
        } else {
            printf("[MCP2515-RESET] Unexpected reset state: 0x%02X\r\n", canstat);
        }
        
        osDelay(10);
    }
    
    printf("[MCP2515-RESET] Reset verification completed with warnings\r\n");
}

/**
  * @brief  设置MCP2515工作模式
  * @param  mode: 工作模式
  * @retval MCP2515_OK: 成功, MCP2515_TIMEOUT: 超时
  */
uint8_t MCP2515_SetMode(uint8_t mode)
{
    // printf("Setting mode to 0x%02X...", mode);
    
    // 修改CANCTRL寄存器的模式位
    MCP2515_ModifyRegister(MCP2515_CANCTRL, 0xE0, mode);
    
    // 等待模式切换完成
    uint8_t result = MCP2515_WaitForMode(mode, MCP2515_MODE_TIMEOUT);
    
    if (result == MCP2515_OK) {
        // printf(" SUCCESS\r\n");
    } else {
        // printf(" TIMEOUT\r\n");
        // printf("Current mode: 0x%02X, Expected: 0x%02X\r\n", MCP2515_GetMode(), mode);
    }
    
    return result;
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
    printf("[MCP2515-INIT] Starting MCP2515 initialization...\r\n");
    
    // 确保CS引脚为高电平
    MCP2515_CS_High();
    osDelay(10);
    
    // 测试SPI通信
    printf("[MCP2515-INIT] Testing SPI communication...\r\n");
    
    // 进行多次SPI测试以确保通信稳定
    uint8_t spi_test_passed = 0;
    printf("[MCP2515-INIT] Performing comprehensive SPI tests...\r\n");
    
    // 测试1: 基本SPI通信测试
    for(int test_count = 0; test_count < 5; test_count++) {
        uint8_t test_data = 0x55;
        uint8_t spi_result = MCP2515_SPI_ReadWrite(test_data);
        printf("[MCP2515-INIT] SPI test #%d: sent 0x%02X, received 0x%02X\r\n", test_count + 1, test_data, spi_result);
        
        // 检查是否收到有效响应（不是0xFF或0x00）
        if(spi_result != 0xFF && spi_result != 0x00) {
            spi_test_passed = 1;
            printf("[MCP2515-INIT] SPI communication test PASSED\r\n");
            break;
        }
        osDelay(20);  // 增加测试间隔
    }
    
    // 测试2: 如果基本测试失败，尝试读取MCP2515寄存器
    if(!spi_test_passed) {
        printf("[MCP2515-INIT] Basic SPI test failed, trying register read test...\r\n");
        
        // 尝试读取CANSTAT寄存器（复位后应该是0x80）
        for(int reg_test = 0; reg_test < 3; reg_test++) {
            uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
            printf("[MCP2515-INIT] Register test #%d: CANSTAT = 0x%02X\r\n", reg_test + 1, canstat);
            
            if(canstat == 0x80 || canstat == 0x00 || canstat == 0x40) {
                spi_test_passed = 1;
                printf("[MCP2515-INIT] Register read test PASSED\r\n");
                break;
            }
            osDelay(30);
        }
    }
    
    if(!spi_test_passed) {
        printf("[MCP2515-INIT] ERROR: All SPI tests failed! Running hardware diagnosis...\r\n");
        MCP2515_HardwareDiagnosis();
        printf("[MCP2515-INIT] Continuing initialization with warnings...\r\n");
    }
    
    // 复位MCP2515
    printf("[MCP2515-INIT] Resetting MCP2515...\r\n");
    MCP2515_Reset();
    
    // 强制切换到配置模式，不依赖复位状态
    // printf("Forcing switch to configuration mode...\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
        // printf("[ERROR] Failed to enter configuration mode\r\n");
        // 尝试再次复位和切换
        // printf("Retrying reset and mode switch...\r\n");
        MCP2515_Reset();
        osDelay(50);  // 增加延时
        if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
            // printf("[ERROR] Second attempt failed\r\n");
            return MCP2515_ERROR;
        }
    }
    // printf("[OK] Successfully entered configuration mode\r\n");
    
    // 检查MCP2515是否响应（在配置模式下测试）
    if (MCP2515_SelfTest() != MCP2515_OK) {
        // printf("[ERROR] MCP2515 self-test failed\r\n");
        return MCP2515_ERROR;
    }
    // printf("[OK] MCP2515 self-test passed\r\n");
    
    // 已经在配置模式下，直接进行波特率设置
    
    // 设置波特率
    if (MCP2515_SetBaudRate(baudrate) != MCP2515_OK) {
        return MCP2515_ERROR;
    }
    
    // 配置接收缓冲区控制寄存器 - 禁用过滤器，接收所有消息
    MCP2515_WriteRegister(MCP2515_RXB0CTRL, 0x60);  // 接收所有消息，禁用过滤器
    MCP2515_WriteRegister(MCP2515_RXB1CTRL, 0x60);  // 接收所有消息，禁用过滤器
    // printf("RX buffer configuration: RXB0CTRL=0x60, RXB1CTRL=0x60 (accept all messages)\r\n");
    
    // 清除所有中断标志
    MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
    
    // 启用接收中断
    MCP2515_WriteRegister(MCP2515_CANINTE, MCP2515_INT_RX0IF | MCP2515_INT_RX1IF);
    
    // 验证中断配置
    uint8_t caninte_verify = MCP2515_ReadRegister(MCP2515_CANINTE);
    printf("[MCP2515-INIT] Interrupt enable verification: CANINTE = 0x%02X\r\n", caninte_verify);
    if (caninte_verify != (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF)) {
        printf("[MCP2515-INIT] WARNING: Interrupt enable register mismatch!\r\n");
        printf("[MCP2515-INIT] Expected: 0x%02X, Actual: 0x%02X\r\n", 
               (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF), caninte_verify);
    }
    
    // 配置过滤器以接收所有消息（在配置模式下）
    printf("[MCP2515-INIT] Configuring filters to accept all messages...\r\n");
    MCP2515_SetFilterForAll();
    
    // 切换到正常模式
    printf("[MCP2515-INIT] Switching to Normal mode...\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        printf("[MCP2515-INIT] ERROR: Failed to switch to Normal mode\r\n");
        return MCP2515_ERROR;
    }
    
    // 确保正常模式设置成功
    MCP2515_ModeNormal();
    
    // 初始化完成后的状态验证
    printf("[MCP2515-INIT] Post-initialization status verification:\r\n");
    MCP2515_VerifyInitialization();
    
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
        printf("[MCP2515-TX-ERROR] Send failed: %s\r\n", 
               !mcp2515_initialized ? "Not initialized" : "NULL message");
        return MCP2515_ERROR;
    }
    
    // 发送前检查Bus-Off状态
    uint8_t eflg_check = MCP2515_ReadRegister(MCP2515_EFLG);
    if (eflg_check & 0x20) {
        printf("[MCP2515-TX-ERROR] Bus-Off state detected before send, attempting recovery...\r\n");
        if (MCP2515_RecoverFromBusOff() == MCP2515_OK) {
            printf("[MCP2515-TX-RECOVERY] Pre-send Bus-Off recovery successful\r\n");
        } else {
            printf("[MCP2515-TX-ERROR] Pre-send Bus-Off recovery failed\r\n");
            return MCP2515_ERROR;
        }
    }
    
    // 查找空闲的发送缓冲区
    buffer = MCP2515_GetTxBuffer();
    if (buffer == 0xFF) {
        uint8_t status = MCP2515_GetStatus();
        printf("[MCP2515-TX-ERROR] No free TX buffer, status: 0x%02X\r\n", status);
        printf("[MCP2515-TX-ERROR] TXB0: %s, TXB1: %s, TXB2: %s\r\n",
               (status & 0x04) ? "BUSY" : "FREE",
               (status & 0x10) ? "BUSY" : "FREE",
               (status & 0x40) ? "BUSY" : "FREE");
        return MCP2515_ERROR;  // 没有空闲的发送缓冲区
    }
    
    printf("[MCP2515-TX-DEBUG] Using TX buffer %d for ID:0x%03X\r\n", buffer, (unsigned int)message->id);
    
    // 加载消息到发送缓冲区
    MCP2515_LoadTxBuffer(buffer, message);
    
    // 请求发送
    MCP2515_CS_Low();
    uint8_t rts_cmd = MCP2515_CMD_RTS | (1 << buffer);
    MCP2515_SPI_ReadWrite(rts_cmd);
    MCP2515_CS_High();
    
    printf("[MCP2515-TX-DEBUG] RTS command sent: 0x%02X\r\n", rts_cmd);
    
    // 等待发送完成
    while (timeout < 1000) {
        // 临时修复：直接使用READ_STATUS指令检查发送完成状态
        // 因为CANINTF寄存器读取有问题，我们使用READ_STATUS的位来判断
        uint8_t status_reg = MCP2515_GetStatus();
        uint8_t tx_status_bit = 0;
        
        // READ_STATUS指令返回的状态位定义：
        // bit2 (0x04): TXB0空闲状态 (1=忙碌, 0=空闲)
        // bit4 (0x10): TXB1空闲状态 (1=忙碌, 0=空闲) 
        // bit6 (0x40): TXB2空闲状态 (1=忙碌, 0=空闲)
        switch(buffer) {
            case 0: tx_status_bit = 0x04; break;  // TXB0
            case 1: tx_status_bit = 0x10; break;  // TXB1
            case 2: tx_status_bit = 0x40; break;  // TXB2
        }
        
        // 发送完成的条件：对应的状态位为0（空闲）
        uint8_t tx_completed = !(status_reg & tx_status_bit);
        
        // 为了调试，仍然读取CANINTF但不用于判断
        uint8_t interrupt_flags = MCP2515_GetInterruptFlags();
        uint8_t expected_flag = MCP2515_INT_TX0IF << buffer;
        
        // 同时检查发送缓冲区控制寄存器的TXREQ位
        uint8_t txb_ctrl_addr = MCP2515_TXB0CTRL + (buffer * 0x10);
        uint8_t txb_ctrl = MCP2515_ReadRegister(txb_ctrl_addr);
        uint8_t txreq_bit = txb_ctrl & 0x08;  // TXREQ位在bit3
        
        // 发送完成的条件：使用READ_STATUS检查发送缓冲区状态 或 TXREQ位清零
        if (tx_completed || (!txreq_bit)) {
            // 打印MCP2515扩展CAN发送日志
            printf("[MCP2515-EXT-TX] ID:0x%03X, DLC:%d, %s, Data:", 
                   (unsigned int)message->id, 
                   message->dlc,
                   message->ide ? "Ext" : "Std");
            if (!message->rtr) {
                for (int i = 0; i < message->dlc && i < 8; i++) {
                    printf("%02X ", message->data[i]);
                }
            } else {
                printf("RTR ");
            }
            printf("\r\n");
            
            // 清除发送完成中断标志（如果有的话）
            if (interrupt_flags & expected_flag) {
                MCP2515_ClearInterruptFlags(expected_flag);
            }
            
            printf("[MCP2515-TX-SUCCESS] Send completed via %s detection\r\n", 
                   tx_completed ? "READ_status" : "TXREQ clear");
            return MCP2515_OK;
        }
        
        // 每100ms打印一次调试信息
        if (timeout % 100 == 0 && timeout > 0) {
            printf("[MCP2515-TX-DEBUG] Waiting for TX completion, timeout: %lu\r\n", timeout);
            printf("[MCP2515-TX-DEBUG] - READ_STATUS: 0x%02X, TX%d bit: 0x%02X, completed: %s\r\n", 
                   status_reg, buffer, tx_status_bit, tx_completed ? "YES" : "NO");
            printf("[MCP2515-TX-DEBUG] - CANINTF flags: 0x%02X (debug only, not used)\r\n", interrupt_flags);
            printf("[MCP2515-TX-DEBUG] - TXB%d CTRL: 0x%02X, TXREQ: %s\r\n", 
                   buffer, txb_ctrl, txreq_bit ? "SET" : "CLEAR");
            
            // 使用调试版本的CANINTF读取函数
            if (timeout == 100) {  // 只在第一次调试时详细输出
                printf("[MCP2515-TX-DEBUG] === Detailed CANINTF Read Debug ===\r\n");
                (void)MCP2515_GetInterruptFlags_Debug();  // 调用调试函数但不保存返回值
                printf("[MCP2515-TX-DEBUG] === End Detailed Debug ===\r\n");
            }
        }
        
        // 检查是否需要强制清除发送缓冲区（在500ms后开始尝试）
        if (timeout >= 500 && (timeout % 200 == 0)) {
            printf("[MCP2515-TX-FORCE] Attempting to force clear TX buffer %d at timeout %lu\r\n", buffer, timeout);
            
            // 检查错误标志
            uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
            printf("[MCP2515-TX-FORCE] Error flags: 0x%02X\r\n", eflg);
            
            // 如果有错误，清除错误标志
            if (eflg != 0x00) {
                printf("[MCP2515-TX-FORCE] Clearing error flags\r\n");
                MCP2515_WriteRegister(MCP2515_EFLG, 0x00);
            }
            
            // 强制清除TXREQ位
            printf("[MCP2515-TX-FORCE] Force clearing TXREQ bit in TXB%d\r\n", buffer);
            uint8_t txb_ctrl_addr = MCP2515_TXB0CTRL + (buffer * 0x10);
            MCP2515_ModifyRegister(txb_ctrl_addr, 0x08, 0x00);  // 清除TXREQ位
            
            // 等待一段时间后重新检查
            osDelay(10);
            
            uint8_t new_status = MCP2515_GetStatus();
            uint8_t new_ctrl = MCP2515_ReadRegister(txb_ctrl_addr);
            printf("[MCP2515-TX-FORCE] After force clear - Status: 0x%02X, Ctrl: 0x%02X\r\n", 
                   new_status, new_ctrl);
            
            // 检查是否成功清除
            uint8_t new_tx_status_bit = 0;
            switch(buffer) {
                case 0: new_tx_status_bit = 0x04; break;
                case 1: new_tx_status_bit = 0x10; break;
                case 2: new_tx_status_bit = 0x40; break;
            }
            
            if (!(new_status & new_tx_status_bit) || !(new_ctrl & 0x08)) {
                printf("[MCP2515-TX-FORCE] TX buffer %d cleared successfully\r\n", buffer);
                printf("[MCP2515-TX-FORCE] Message may not have been transmitted to bus\r\n");
                return MCP2515_OK;
            }
        }
        
        osDelay(1);
        timeout++;
    }
    
    printf("[MCP2515-TX-ERROR] Send timeout after %lu ms for ID:0x%03X\r\n", timeout, (unsigned int)message->id);
    
    // 超时后检查错误状态
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    uint8_t tec, rec;
    MCP2515_GetErrorCounters(&tec, &rec);
    printf("[MCP2515-TX-ERROR] Error flags: 0x%02X, TEC: %d, REC: %d\r\n", eflg, tec, rec);
    
    // 最终尝试强制清除发送缓冲区
    printf("[MCP2515-TX-FINAL] Final attempt to force clear TX buffer %d\r\n", buffer);
    uint8_t txb_ctrl_addr = MCP2515_TXB0CTRL + (buffer * 0x10);
    MCP2515_ModifyRegister(txb_ctrl_addr, 0x08, 0x00);  // 清除TXREQ位
    
    // 清除可能的错误标志
    if (eflg != 0x00) {
        printf("[MCP2515-TX-FINAL] Clearing error flags: 0x%02X\r\n", eflg);
        MCP2515_WriteRegister(MCP2515_EFLG, 0x00);
    }
    
    // 验证清除结果
    osDelay(5);
    uint8_t final_status = MCP2515_GetStatus();
    uint8_t final_ctrl = MCP2515_ReadRegister(txb_ctrl_addr);
    printf("[MCP2515-TX-FINAL] After final clear - Status: 0x%02X, Ctrl: 0x%02X\r\n", 
           final_status, final_ctrl);
    
    // 检查是否为Bus-Off状态，如果是则尝试恢复
    if (eflg & 0x20) {
        printf("[MCP2515-TX-ERROR] Bus-Off state detected, attempting recovery...\r\n");
        if (MCP2515_RecoverFromBusOff() == MCP2515_OK) {
            printf("[MCP2515-TX-RECOVERY] Bus-Off recovery successful\r\n");
            // 恢复成功后重试发送一次
            printf("[MCP2515-TX-RETRY] Retrying message send after recovery...\r\n");
            buffer = MCP2515_GetTxBuffer();
            if (buffer != 0xFF) {
                MCP2515_LoadTxBuffer(buffer, message);
                MCP2515_CS_Low();
                uint8_t rts_cmd = MCP2515_CMD_RTS | (1 << buffer);
                MCP2515_SPI_ReadWrite(rts_cmd);
                MCP2515_CS_High();
                
                // 等待发送完成（短时间）
                timeout = 0;
                while (timeout < 500) {
                    // 使用READ_STATUS指令检查发送完成状态
                    uint8_t status_reg = MCP2515_GetStatus();
                    uint8_t tx_status_bit = 0;
                    
                    switch(buffer) {
                        case 0: tx_status_bit = 0x04; break;  // TXB0
                        case 1: tx_status_bit = 0x10; break;  // TXB1
                        case 2: tx_status_bit = 0x40; break;  // TXB2
                    }
                    
                    uint8_t tx_completed = !(status_reg & tx_status_bit);
                    
                    // 同时检查发送缓冲区控制寄存器的TXREQ位
                    uint8_t txb_ctrl_addr = MCP2515_TXB0CTRL + (buffer * 0x10);
                    uint8_t txb_ctrl = MCP2515_ReadRegister(txb_ctrl_addr);
                    uint8_t txreq_bit = txb_ctrl & 0x08;  // TXREQ位在bit3
                    
                    // 发送完成的条件：READ_STATUS检查 或 TXREQ位清零
                    if (tx_completed || (!txreq_bit)) {
                        printf("[MCP2515-TX-RETRY] Retry successful for ID:0x%03X via %s detection\r\n", 
                               (unsigned int)message->id,
                               tx_completed ? "read_status" : "TXREQ clear");
                        return MCP2515_OK;
                    }
                    osDelay(1);
                    timeout++;
                }
                printf("[MCP2515-TX-RETRY] Retry also timed out\r\n");
            } else {
                printf("[MCP2515-TX-RETRY] No free buffer after recovery\r\n");
            }
        } else {
            printf("[MCP2515-TX-ERROR] Bus-Off recovery failed\r\n");
        }
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
  * @brief  调试版本的获取中断标志函数
  * @param  None
  * @retval 中断标志寄存器值
  */
uint8_t MCP2515_GetInterruptFlags_Debug(void)
{
    uint8_t canintf_value;
    
    printf("[MCP2515-DEBUG] Reading CANINTF register (0x%02X)...\r\n", MCP2515_CANINTF);
    
    // 手动执行读取过程，添加调试信息
    MCP2515_CS_Low();
    printf("[MCP2515-DEBUG] CS pulled low\r\n");
    
    uint8_t cmd_response = MCP2515_SPI_ReadWrite(MCP2515_CMD_READ);
    printf("[MCP2515-DEBUG] READ command (0x%02X) sent, response: 0x%02X\r\n", MCP2515_CMD_READ, cmd_response);
    
    uint8_t addr_response = MCP2515_SPI_ReadWrite(MCP2515_CANINTF);
    printf("[MCP2515-DEBUG] CANINTF address (0x%02X) sent, response: 0x%02X\r\n", MCP2515_CANINTF, addr_response);
    
    canintf_value = MCP2515_SPI_ReadWrite(0x00);
    printf("[MCP2515-DEBUG] Data read: 0x%02X\r\n", canintf_value);
    
    MCP2515_CS_High();
    printf("[MCP2515-DEBUG] CS pulled high\r\n");
    
    // 为了对比，也读取一下READ_STATUS的结果
    uint8_t status_value = MCP2515_GetStatus();
    printf("[MCP2515-DEBUG] READ_STATUS result: 0x%02X\r\n", status_value);
    
    printf("[MCP2515-DEBUG] CANINTF=0x%02X, READ_STATUS=0x%02X\r\n", canintf_value, status_value);
    
    return canintf_value;
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
// 全局变量用于中断标志
static volatile uint8_t mcp2515_irq_pending = 0;

void MCP2515_IRQHandler(void)
{
    // 在中断上下文中，避免复杂的SPI操作
    // 只设置标志，让主任务处理
    mcp2515_irq_pending = 1;
    
    // 简单的调试输出（避免复杂的SPI读取）
    printf("[MCP2515-IRQ] Interrupt triggered, flag set\r\n");
}

// 新增函数：在主任务中处理中断
uint8_t MCP2515_ProcessPendingInterrupt(void)
{
    if (!mcp2515_irq_pending) {
        return 0;  // 没有待处理的中断
    }
    
    // 清除中断标志
    mcp2515_irq_pending = 0;
    
    // 现在在主任务上下文中安全地进行SPI操作
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    uint8_t caninte = MCP2515_ReadRegister(MCP2515_CANINTE);
    uint8_t interrupt_flags = MCP2515_GetInterruptFlags();
    
    printf("[MCP2515-IRQ] Processing: CANSTAT: 0x%02X, CANINTE: 0x%02X, CANINTF: 0x%02X\r\n", 
           canstat, caninte, interrupt_flags);
    
    // 如果中断标志为0，可能是SPI通信问题或中断配置问题
    if (interrupt_flags == 0x00) {
        printf("[MCP2515-IRQ] WARNING: No interrupt flags set\r\n");
        
        // 详细诊断中断问题
        printf("[MCP2515-IRQ] Interrupt diagnosis:\r\n");
        printf("[MCP2515-IRQ] - CANINTE (enabled): 0x%02X\r\n", caninte);
        printf("[MCP2515-IRQ] - CANINTF (flags): 0x%02X\r\n", interrupt_flags);
        printf("[MCP2515-IRQ] - CANSTAT (status): 0x%02X\r\n", canstat);
        
        // 检查错误标志
        uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
        printf("[MCP2515-IRQ] - EFLG (errors): 0x%02X\r\n", eflg);
        
        // 检查接收缓冲区状态
        uint8_t rxb0ctrl = MCP2515_ReadRegister(MCP2515_RXB0CTRL);
        uint8_t rxb1ctrl = MCP2515_ReadRegister(MCP2515_RXB1CTRL);
        printf("[MCP2515-IRQ] - RXB0CTRL: 0x%02X, RXB1CTRL: 0x%02X\r\n", rxb0ctrl, rxb1ctrl);
        
        // 检查发送缓冲区状态
        uint8_t txb0ctrl = MCP2515_ReadRegister(MCP2515_TXB0CTRL);
        uint8_t txb1ctrl = MCP2515_ReadRegister(MCP2515_TXB1CTRL);
        uint8_t txb2ctrl = MCP2515_ReadRegister(MCP2515_TXB2CTRL);
        printf("[MCP2515-IRQ] - TXB0CTRL: 0x%02X, TXB1CTRL: 0x%02X, TXB2CTRL: 0x%02X\r\n", 
               txb0ctrl, txb1ctrl, txb2ctrl);
        
        /*
        // MCP2515相关代码已注释 - 硬件已移除
        // 检查INT引脚状态
        GPIO_PinState int_pin = HAL_GPIO_ReadPin(MCP2515_INT_GPIO_Port, MCP2515_INT_Pin);
        printf("[MCP2515-IRQ] - INT pin state: %s\r\n", int_pin ? "HIGH" : "LOW");
        
        // 如果INT引脚为LOW但没有中断标志，可能是硬件问题
        if (int_pin == GPIO_PIN_RESET) {
            printf("[MCP2515-IRQ] ERROR: INT pin is LOW but no interrupt flags set!\r\n");
            printf("[MCP2515-IRQ] This suggests hardware connection or MCP2515 internal issue\r\n");
        }
        */
        
        return 0;
    }
    
    // 处理接收中断
    if (interrupt_flags & (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF)) {
        printf("[MCP2515-IRQ] Receive interrupt detected\r\n");
        
        // 立即处理接收到的消息
        MCP2515_CANMessage_t received_message;
        if (MCP2515_ReceiveMessage(&received_message) == MCP2515_OK) {
            printf("[MCP2515-IRQ] Message received: ID=0x%03X, DLC=%d\r\n", 
                   (unsigned int)received_message.id, received_message.dlc);
            
            /*
            // MCP2515相关代码已注释 - 硬件已移除
            // 处理循环测试消息
            extern void CAN_LoopTest_ProcessMCP2515Message(MCP2515_CANMessage_t* message);
            CAN_LoopTest_ProcessMCP2515Message(&received_message);
            */
        }
    }
    
    // 处理发送完成中断
    if (interrupt_flags & (MCP2515_INT_TX0IF | MCP2515_INT_TX1IF | MCP2515_INT_TX2IF)) {
        printf("[MCP2515-IRQ] Transmit complete interrupt\r\n");
        // 发送完成处理
        // 可以通知发送任务或更新发送状态
    }
    
    // 处理错误中断
    if (interrupt_flags & MCP2515_INT_ERRIF) {
        printf("[MCP2515-IRQ] Error interrupt detected\r\n");
        // 错误处理
        uint8_t error_flags = MCP2515_GetErrorFlags();
        printf("[MCP2515-IRQ] Error flags: 0x%02X\r\n", error_flags);
        // 根据错误类型进行相应处理
        MCP2515_ClearErrorFlags();
    }
    
    // 清除已处理的中断标志
    MCP2515_ClearInterruptFlags(interrupt_flags);
    
    return 1;  // 返回1表示处理了中断
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
  * @brief  MCP2515硬件连接测试函数
  * @param  None
  * @retval MCP2515_OK: 成功, MCP2515_ERROR: 失败
  */
uint8_t MCP2515_HardwareTest(void)
{
    // printf("\r\n=== MCP2515 Hardware Connection Test ===\r\n");
    
    // 1. CS引脚控制测试
    // printf("Step 1: Testing CS pin control...\r\n");
    for (int i = 0; i < 3; i++) {
        MCP2515_CS_High();
        osDelay(1);
        MCP2515_CS_Low();
        osDelay(1);
    }
    MCP2515_CS_High();
    // printf("[OK] CS pin control test completed\r\n");
    
    // 2. SPI基础通信测试
    // printf("Step 2: Testing basic SPI communication...\r\n");
    MCP2515_CS_Low();
    uint8_t dummy1 = MCP2515_SPI_ReadWrite(0x00);
    uint8_t dummy2 = MCP2515_SPI_ReadWrite(0xFF);
    MCP2515_CS_High();
    // printf("SPI test results: 0x00->0x%02X, 0xFF->0x%02X\r\n", dummy1, dummy2);
    
    if (dummy1 == 0xFF && dummy2 == 0xFF) {
        // printf("[WARN] Warning: All SPI reads return 0xFF\r\n");
        // printf("  This suggests MISO line issue or MCP2515 not responding\r\n");
    }
    
    // 3. 复位测试
    // printf("Step 3: Testing MCP2515 reset...\r\n");
    MCP2515_Reset();
    
    // 4. 寄存器读写测试
    // printf("Step 4: Testing register read/write...\r\n");
    
    // 测试CNF1寄存器（可读写）
    uint8_t original = MCP2515_ReadRegister(MCP2515_CNF1);
    // printf("CNF1 original value: 0x%02X\r\n", original);
    
    // 写入测试值
    uint8_t test_value = 0xAA;
    MCP2515_WriteRegister(MCP2515_CNF1, test_value);
    uint8_t read_back = MCP2515_ReadRegister(MCP2515_CNF1);
    // printf("CNF1 write 0x%02X, read back 0x%02X\r\n", test_value, read_back);
    
    if (read_back == test_value) {
        // printf("[OK] Register write test 1 passed\r\n");
    } else {
        // printf("[ERROR] Register write test 1 failed\r\n");
        return MCP2515_ERROR;
    }
    
    // 测试另一个值
    test_value = 0x55;
    MCP2515_WriteRegister(MCP2515_CNF1, test_value);
    read_back = MCP2515_ReadRegister(MCP2515_CNF1);
    // printf("CNF1 write 0x%02X, read back 0x%02X\r\n", test_value, read_back);
    
    if (read_back == test_value) {
        // printf("[OK] Register write test 2 passed\r\n");
    } else {
        // printf("[ERROR] Register write test 2 failed\r\n");
        return MCP2515_ERROR;
    }
    
    // 恢复原始值
    MCP2515_WriteRegister(MCP2515_CNF1, original);
    // printf("CNF1 restored to original value: 0x%02X\r\n", original);
    
    // printf("[OK] All hardware tests passed!\r\n");
    return MCP2515_OK;
}

/**
  * @brief  打印MCP2515状态信息
  * @param  None
  * @retval None
  */
void MCP2515_PrintStatus(void)
{
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    // uint8_t canctrl = MCP2515_ReadRegister(MCP2515_CANCTRL);
    // uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    // uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    
    // printf("MCP2515 Status:\r\n");
    // printf("CANSTAT: 0x%02X\r\n", canstat);
    // printf("CANCTRL: 0x%02X\r\n", canctrl);
    // printf("CANINTF: 0x%02X\r\n", canintf);
    // printf("EFLG: 0x%02X\r\n", eflg);
    // printf("Mode: ");
    
    switch (canstat & 0xE0) {
        case MCP2515_MODE_NORMAL:
            // printf("Normal\r\n");
            break;
        case MCP2515_MODE_SLEEP:
            // printf("Sleep\r\n");
            break;
        case MCP2515_MODE_LOOPBACK:
            // printf("Loopback\r\n");
            break;
        case MCP2515_MODE_LISTENONLY:
            // printf("Listen Only\r\n");
            break;
        case MCP2515_MODE_CONFIG:
            // printf("Configuration\r\n");
            break;
        default:
            // printf("Unknown\r\n");
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
uint8_t MCP2515_WaitForMode(uint8_t mode, uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    uint32_t check_count = 0;
    
    while ((HAL_GetTick() - start_time) < timeout) {
        uint8_t current_mode = MCP2515_GetMode();
        check_count++;
        
        if (current_mode == mode) {
            // printf("Mode switch completed after %lu checks\r\n", check_count);
            return MCP2515_OK;
        }
        
        // 每100次检查输出一次状态
        if (check_count % 100 == 0) {
            // printf("Waiting for mode 0x%02X, current: 0x%02X (check #%lu)\r\n", 
            //        mode, current_mode, check_count);
        }
        
        osDelay(1);
    }
    
    // printf("Mode switch timeout after %lu checks\r\n", check_count);
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

/*
// MCP2515相关函数已注释 - 硬件已移除
void Simple_CS_Test(void)
{
    // printf("Testing CS pin control...\r\n");
    
    for(int i = 0; i < 5; i++) {
        HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET);
        // printf("CS Low\r\n");
        HAL_Delay(100);
        
        HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
        // printf("CS High\r\n");
        HAL_Delay(100);
    }
    // printf("CS test completed\r\n");
}
*/

/* 错误诊断和修复功能 --------------------------------------------------------*/

// 错误计数器寄存器定义
#define MCP2515_TEC     0x1C    // 发送错误计数器
#define MCP2515_REC     0x1D    // 接收错误计数器

/**
  * @brief  Read MCP2515 error counters
  * @param  tec: Transmit error counter pointer
  * @param  rec: Receive error counter pointer
  * @retval None
  */
void MCP2515_GetErrorCounters(uint8_t *tec, uint8_t *rec)
{
    *tec = MCP2515_ReadRegister(MCP2515_TEC);
    *rec = MCP2515_ReadRegister(MCP2515_REC);
}

/**
  * @brief  Detailed error status diagnosis
  * @param  None
  * @retval None
  */
void MCP2515_DiagnoseErrors(void)
{
    uint8_t canintf, eflg, tec, rec;
    
    // printf("\r\n=== MCP2515 Error Diagnosis ===\r\n");
    
    // Read status registers
    canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    MCP2515_GetErrorCounters(&tec, &rec);
    
    // printf("CANINTF: 0x%02X\r\n", canintf);
    // printf("EFLG: 0x%02X\r\n", eflg);
    // printf("Transmit Error Counter (TEC): %d\r\n", tec);
    // printf("Receive Error Counter (REC): %d\r\n", rec);
    
    // Analyze CANINTF
    // printf("\r\n--- CANINTF Analysis ---\r\n");
    if (canintf & 0x80) /* printf("WARNING: MERRF - Message Error Interrupt\r\n"); */;
    if (canintf & 0x40) /* printf("INFO: WAKIF - Wake-up Interrupt\r\n"); */;
    if (canintf & 0x20) /* printf("WARNING: ERRIF - Error Interrupt\r\n"); */;
    if (canintf & 0x10) /* printf("OK: TX2IF - Transmit Buffer 2 Interrupt\r\n"); */;
    if (canintf & 0x08) /* printf("OK: TX1IF - Transmit Buffer 1 Interrupt\r\n"); */;
    if (canintf & 0x04) /* printf("OK: TX0IF - Transmit Buffer 0 Interrupt\r\n"); */;
    if (canintf & 0x02) /* printf("INFO: RX1IF - Receive Buffer 1 Interrupt\r\n"); */;
    if (canintf & 0x01) /* printf("INFO: RX0IF - Receive Buffer 0 Interrupt\r\n"); */;
    
    // Analyze EFLG
    // printf("\r\n--- EFLG Analysis ---\r\n");
    if (eflg & 0x80) /* printf("ERROR: RX1OVR - Receive Buffer 1 Overflow\r\n"); */;
    if (eflg & 0x40) /* printf("ERROR: RX0OVR - Receive Buffer 0 Overflow\r\n"); */;
    if (eflg & 0x20) /* printf("ERROR: TXBO - Bus-Off State\r\n"); */;
    if (eflg & 0x10) /* printf("WARNING: TXEP - Transmit Error Passive\r\n"); */;
    if (eflg & 0x08) /* printf("WARNING: RXEP - Receive Error Passive\r\n"); */;
    if (eflg & 0x04) /* printf("WARNING: TXWAR - Transmit Error Warning\r\n"); */;
    if (eflg & 0x02) /* printf("WARNING: RXWAR - Receive Error Warning\r\n"); */;
    if (eflg & 0x01) /* printf("WARNING: EWARN - Error Warning\r\n"); */;
    
    // Error level assessment
    // printf("\r\n--- Error Level Assessment ---\r\n");
    if (eflg & 0x20) {
        // printf("CRITICAL: Bus-Off state, requires re-initialization\r\n");
        // printf("Attempting automatic Bus-Off recovery...\r\n");
        if (MCP2515_RecoverFromBusOff() == MCP2515_OK) {
            // printf("[SUCCESS] Bus-Off recovery completed\r\n");
        } else {
            // printf("[ERROR] Bus-Off recovery failed\r\n");
        }
    } else if (eflg & 0x10) {
        // printf("WARNING: Transmit Error Passive, TEC >= 128\r\n");
        // printf("   Suggestion: Check bus connection and termination resistors\r\n");
    } else if (eflg & 0x04) {
        // printf("INFO: Transmit Error Warning, TEC >= 96\r\n");
    }
    
    // printf("===============================\r\n");
}

/**
  * @brief  Clear all error flags and interrupt flags
  * @param  None
  * @retval None
  */
void MCP2515_ClearAllErrors(void)
{
    // printf("Clearing error flags...\r\n");
    
    // Clear interrupt flags
    MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
    
    // printf("Error flags cleared\r\n");
}

/**
  * @brief  Force recovery from Bus-Off state
  * @param  None
  * @retval MCP2515_OK: Success, MCP2515_ERROR: Failed
  */
uint8_t MCP2515_RecoverFromBusOff(void)
{
    // printf("\r\n=== Bus-Off Recovery Process ===\r\n");
    
    // 检查当前错误状态
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    // printf("Current EFLG: 0x%02X\r\n", eflg);
    
    if (!(eflg & 0x20)) {
        // printf("[INFO] Not in Bus-Off state, no recovery needed\r\n");
        return MCP2515_OK;
    }
    
    // printf("[CRITICAL] Bus-Off state detected, starting recovery...\r\n");
    
    // Step 1: Force enter configuration mode
    // printf("Step 1: Force enter configuration mode...\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_CONFIG) != MCP2515_OK) {
        // printf("[ERROR] Failed to enter configuration mode\r\n");
        return MCP2515_ERROR;
    }
    // printf("[OK] Entered configuration mode\r\n");
    
    // Step 2: Reset MCP2515
    // printf("Step 2: Reset MCP2515...\r\n");
    MCP2515_Reset();
    osDelay(100);
    // printf("[OK] MCP2515 reset completed\r\n");
    
    // Step 3: Re-initialize
    // printf("Step 3: Re-initialize MCP2515...\r\n");
    if (MCP2515_Init(MCP2515_BAUD_500K) != MCP2515_OK) {
        // printf("[ERROR] Re-initialization failed\r\n");
        return MCP2515_ERROR;
    }
    // printf("[OK] Re-initialization successful\r\n");
    
    // Step 4: Verify recovery status
    // printf("Step 4: Verify recovery...\r\n");
    eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    // uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    // printf("After recovery - EFLG: 0x%02X, CANSTAT: 0x%02X\r\n", eflg, canstat);
    
    if (eflg & 0x20) {
        // printf("[ERROR] Still in Bus-Off state after recovery\r\n");
        return MCP2515_ERROR;
    }
    
    // printf("[SUCCESS] Bus-Off recovery completed successfully\r\n");
    // printf("================================\r\n");
    return MCP2515_OK;
}

/**
  * @brief  Loopback mode test
  * @param  None
  * @retval Test result (MCP2515_OK: Success, MCP2515_ERROR: Failed)
  */
uint8_t MCP2515_LoopbackTest(void)
{
    MCP2515_CANMessage_t test_msg;
    MCP2515_CANMessage_t recv_msg;
    uint8_t result = MCP2515_ERROR;
    
    printf("\r\n=== Loopback Mode Test ===\r\n");
    
    // Check Bus-Off status
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    if (eflg & 0x20) {
        // printf("[WARNING] Bus-Off state detected before loopback test\r\n");
        // printf("Attempting Bus-Off recovery...\r\n");
        if (MCP2515_RecoverFromBusOff() != MCP2515_OK) {
            // printf("[ERROR] Bus-Off recovery failed, cannot proceed with loopback test\r\n");
            return MCP2515_ERROR;
        }
        // printf("[OK] Bus-Off recovery successful, proceeding with loopback test\r\n");
    }
    
    // Switch to loopback mode
    printf("Switching to loopback mode...\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_LOOPBACK) != MCP2515_OK) {
        printf("ERROR: Failed to switch to loopback mode\r\n");
        return MCP2515_ERROR;
    }
    
    printf("[OK] Switched to loopback mode\r\n");
    
    // Read current mode and receive buffer status
    // uint8_t current_mode = MCP2515_ReadRegister(MCP2515_CANSTAT) & 0xE0;
    // uint8_t rxb0ctrl = MCP2515_ReadRegister(MCP2515_RXB0CTRL);
    // uint8_t rxb1ctrl = MCP2515_ReadRegister(MCP2515_RXB1CTRL);
    // printf("Mode verification: CANSTAT=0x%02X (should be 0x40 for loopback)\r\n", current_mode);
    // printf("RX buffer config: RXB0CTRL=0x%02X, RXB1CTRL=0x%02X\r\n", rxb0ctrl, rxb1ctrl);
    
    // printf("[INFO] Starting loopback test...\r\n");
    // printf("Waiting 100ms for mode stabilization...\r\n");
    osDelay(100);  // Wait for mode switch completion - using osDelay for FreeRTOS compatibility
    // printf("Wait completed, preparing test message...\r\n");
    
    // Prepare test message
    // printf("Preparing test message structure...\r\n");
    test_msg.id = 0x123;
    test_msg.dlc = 8;
    test_msg.rtr = 0;
    test_msg.ide = 0;
    // printf("Filling test data...\r\n");
    for (int i = 0; i < 8; i++) {
        test_msg.data[i] = 0xA0 + i;
    }
    // printf("Test message prepared successfully\r\n");
    
    printf("Sending test message ID:0x%03lX...\r\n", test_msg.id);
    // printf("Test data: ");
    // for (int i = 0; i < test_msg.dlc; i++) {
    //     printf("0x%02X ", test_msg.data[i]);
    // }
    // printf("\r\n");
    
    // Send message
    // printf("Calling MCP2515_SendMessage...\r\n");
    uint8_t send_result = MCP2515_SendMessage(&test_msg);
    // printf("Send result: %d\r\n", send_result);
    
    if (send_result == MCP2515_OK) {
        printf("✓ Message sent successfully\r\n");
        
        // Wait for a while
        // printf("Waiting 50ms for loopback...\r\n");
        osDelay(50);
        
        // Detailed check of MCP2515 status
        // printf("Checking MCP2515 status after loopback delay...\r\n");
        // uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
        // uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
        // uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
        // printf("CANINTF: 0x%02X, CANSTAT: 0x%02X, EFLG: 0x%02X\r\n", canintf, canstat, eflg);
        
        // Analyze CANINTF flags
        // if (canintf & 0x01) printf("  RX0IF: Receive Buffer 0 Full\r\n");
        // if (canintf & 0x02) printf("  RX1IF: Receive Buffer 1 Full\r\n");
        // if (canintf & 0x04) printf("  TX0IF: Transmit Buffer 0 Empty\r\n");
        // if (canintf & 0x08) printf("  TX1IF: Transmit Buffer 1 Empty\r\n");
        // if (canintf & 0x10) printf("  TX2IF: Transmit Buffer 2 Empty\r\n");
        // if (canintf & 0x20) printf("  ERRIF: Error Interrupt\r\n");
        // if (canintf & 0x40) printf("  WAKIF: Wake-up Interrupt\r\n");
        // if (canintf & 0x80) printf("  MERRF: Message Error Interrupt\r\n");
        
        // Check if message received
        // printf("Checking for received message...\r\n");
        uint8_t check_result = MCP2515_CheckReceive();
        // printf("Check receive result: %d\r\n", check_result);
        
        if (check_result == 1) {  // MCP2515_CheckReceive returns 1 if message available
            if (MCP2515_ReceiveMessage(&recv_msg) == MCP2515_OK) {
                // printf("OK: Received loopback message ID:0x%03lX\r\n", recv_msg.id);
                
                // Verify data
                if (recv_msg.id == test_msg.id && recv_msg.dlc == test_msg.dlc) {
                    uint8_t data_match = 1;
                    for (int i = 0; i < test_msg.dlc; i++) {
                        if (recv_msg.data[i] != test_msg.data[i]) {
                            data_match = 0;
                            break;
                        }
                    }
                    
                    if (data_match) {
                        printf("✅ Loopback test PASSED! MCP2515 hardware is working correctly\r\n");
                        result = MCP2515_OK;
                    } else {
                        printf("❌ Data mismatch in loopback test\r\n");
                    }
                } else {
                    printf("❌ ID or DLC mismatch in loopback test\r\n");
                }
            } else {
                printf("❌ Failed to receive loopback message\r\n");
            }
        } else {
            printf("❌ No loopback message received\r\n");
            // printf("Detailed analysis:\r\n");
            // printf("   - CANINTF register: 0x%02X\r\n", canintf);
            // printf("   - Expected RX0IF(0x01) or RX1IF(0x02) to be set\r\n");
            // if (canintf & 0x80) printf("   - MERRF flag indicates message error\r\n");
            // if (canintf & 0x20) printf("   - ERRIF flag indicates general error\r\n");
            // printf("Possible causes:\r\n");
            // printf("  - MCP2515 not in loopback mode\r\n");
            // printf("  - Receive buffer configuration issue\r\n");
            // printf("  - Message filtering problem\r\n");
            // printf("  - Internal loopback path not working\r\n");
        }
    } else {
        printf("❌ Message send failed (result: %d)\r\n", send_result);
        // if (send_result == MCP2515_TIMEOUT) {
        //     printf("  - Send timeout occurred\r\n");
        // } else if (send_result == MCP2515_ERROR) {
        //     printf("  - General send error\r\n");
        // }
        // printf("Possible causes:\r\n");
        // printf("  - No available transmit buffer\r\n");
        // printf("  - MCP2515 not in correct mode\r\n");
        // printf("  - SPI communication problem\r\n");
    }
    
    // Switch back to normal mode
    printf("Switching back to normal mode...\r\n");
    MCP2515_SetMode(MCP2515_MODE_NORMAL);
    osDelay(100);
    
    printf("==========================\r\n");
    printf("Loopback test result: %s\r\n", (result == MCP2515_OK) ? "PASSED" : "FAILED");
    return result;
}

/**
  * @brief  CANOE test function - Send CAN messages and output via serial
  * @param  None
  * @retval None
  * @note   This function is specifically designed for CAN bus testing with CANOE tool
  *         Outputs sent data via serial immediately after sending messages for comparison
  */
void MCP2515_CANOETest(void)
{
    MCP2515_CANMessage_t test_msg;
    uint8_t result;
    static uint32_t test_counter = 0;
    
    // printf("\r\n=== CANOE Test Mode - CAN Message Transmission ===\r\n");
    
    // Ensure in normal mode
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        // printf("ERROR: Failed to switch to normal mode\r\n");
        return;
    }
    
    // Prepare test message 1 - Standard frame
    test_msg.id = 0x123;
    test_msg.dlc = 8;
    test_msg.rtr = 0;
    test_msg.ide = 0;  // 标准帧
    test_msg.data[0] = 0x11;
    test_msg.data[1] = 0x22;
    test_msg.data[2] = 0x33;
    test_msg.data[3] = 0x44;
    test_msg.data[4] = (uint8_t)(test_counter & 0xFF);
    test_msg.data[5] = (uint8_t)((test_counter >> 8) & 0xFF);
    test_msg.data[6] = 0xAA;
    test_msg.data[7] = 0xBB;
    
    // printf("\r\n--- Test Message 1 (Standard Frame) ---\r\n");
    // printf("Sending CAN message to bus...\r\n");
    // printf("CAN ID: 0x%03lX (Standard Frame)\r\n", test_msg.id);
    // printf("DLC: %d bytes\r\n", test_msg.dlc);
    // printf("Data: ");
    // for (int i = 0; i < test_msg.dlc; i++) {
    //     printf("0x%02X ", test_msg.data[i]);
    // }
    // printf("\r\n");
    
    // Send message
    result = MCP2515_SendMessage(&test_msg);
    if (result == MCP2515_OK) {
        // printf("[OK] Message sent successfully to CAN bus\r\n");
        // printf(">> Check CANOE for received message with ID 0x123\r\n");
    } else if (result == MCP2515_TIMEOUT) {
        // printf("[WARN] Message send timeout - No ACK received\r\n");
        // printf(">> This is normal if no other CAN nodes are connected\r\n");
        // printf(">> Check CANOE for transmitted message attempt\r\n");
    } else {
        // printf("[ERROR] Message send failed\r\n");
    }
    
    osDelay(500);  // Delay 500ms
    
    // Prepare test message 2 - Extended frame
    test_msg.id = 0x12345678;
    test_msg.dlc = 6;
    test_msg.rtr = 0;
    test_msg.ide = 1;  // 扩展帧
    test_msg.data[0] = 0xCA;
    test_msg.data[1] = 0xFE;
    test_msg.data[2] = 0xBA;
    test_msg.data[3] = 0xBE;
    test_msg.data[4] = (uint8_t)(HAL_GetTick() & 0xFF);
    test_msg.data[5] = (uint8_t)((HAL_GetTick() >> 8) & 0xFF);
    
    // printf("\r\n--- Test Message 2 (Extended Frame) ---\r\n");
    // printf("Sending CAN message to bus...\r\n");
    // printf("CAN ID: 0x%08lX (Extended Frame)\r\n", test_msg.id);
    // printf("DLC: %d bytes\r\n", test_msg.dlc);
    // printf("Data: ");
    // for (int i = 0; i < test_msg.dlc; i++) {
    //     printf("0x%02X ", test_msg.data[i]);
    // }
    // printf("\r\n");
    
    // 发送报文
    result = MCP2515_SendMessage(&test_msg);
    if (result == MCP2515_OK) {
        // printf("[OK] Message sent successfully to CAN bus\r\n");
        // printf(">> Check CANOE for received message with ID 0x12345678\r\n");
    } else if (result == MCP2515_TIMEOUT) {
        // printf("[WARN] Message send timeout - No ACK received\r\n");
        // printf(">> This is normal if no other CAN nodes are connected\r\n");
        // printf(">> Check CANOE for transmitted message attempt\r\n");
    } else {
        // printf("[ERROR] Message send failed\r\n");
    }
    
    osDelay(500);  // Delay 500ms
    
    // Prepare test message 3 - RTR frame
    test_msg.id = 0x456;
    test_msg.dlc = 4;
    test_msg.rtr = 1;  // RTR帧
    test_msg.ide = 0;  // 标准帧
    
    // printf("\r\n--- Test Message 3 (RTR Frame) ---\r\n");
    // printf("Sending RTR message to bus...\r\n");
    // printf("CAN ID: 0x%03lX (Standard RTR Frame)\r\n", test_msg.id);
    // printf("DLC: %d bytes (RTR - no data)\r\n", test_msg.dlc);
    
    // Send RTR message
    result = MCP2515_SendMessage(&test_msg);
    if (result == MCP2515_OK) {
        // printf("[OK] RTR message sent successfully to CAN bus\r\n");
        // printf(">> Check CANOE for received RTR message with ID 0x456\r\n");
    } else if (result == MCP2515_TIMEOUT) {
        // printf("[WARN] RTR message send timeout - No ACK received\r\n");
        // printf(">> This is normal if no other CAN nodes are connected\r\n");
        // printf(">> Check CANOE for transmitted RTR message attempt\r\n");
    } else {
        // printf("[ERROR] RTR message send failed\r\n");
    }
    
    test_counter++;
    
    // printf("\r\n=== CANOE Test Summary ===\r\n");
    // printf("Test sequence #%lu completed\r\n", test_counter);
    // printf("Messages sent to CAN bus:\r\n");
    // printf("  1. Standard Frame: ID=0x123, 8 bytes data\r\n");
    // printf("  2. Extended Frame: ID=0x12345678, 6 bytes data\r\n");
    // printf("  3. RTR Frame: ID=0x456, 4 bytes requested\r\n");
    // printf("\r\nPlease check CANOE trace window for these messages\r\n");
    // printf("If messages appear in CANOE, CAN transmission is working!\r\n");
    // printf("===============================\r\n");
}

/**
  * @brief  Initialization failure diagnosis function
  * @param  None
  * @retval None
  */
void MCP2515_InitFailureDiagnosis(void)
{
    // printf("\r\n=== MCP2515 Initialization Failure Diagnosis ===\r\n");
    
    // 1. Detailed hardware connection test
    // printf("\r\nStep 1: Comprehensive hardware test...\r\n");
    if (MCP2515_HardwareTest() == MCP2515_OK) {
        // printf("[OK] Hardware connections appear to be working\r\n");
    } else {
        // printf("[ERROR] Hardware test failed - Check connections\r\n");
        // printf("\r\nHardware troubleshooting checklist:\r\n");
        // printf("  - SPI connections: SCK(PB3), MISO(PB4), MOSI(PB5)\r\n");
        // printf("  - CS connection: PB12\r\n");
        // printf("  - Power supply: 3.3V to MCP2515 VCC\r\n");
        // printf("  - Ground connection: GND\r\n");
        // printf("  - Crystal oscillator: 8MHz or 16MHz\r\n");
        // printf("  - Decoupling capacitors: 100nF near MCP2515\r\n");
        return;
    }
    
    // 2. Multiple reset attempts
    // printf("\r\nStep 2: Multiple reset attempts...\r\n");
    for (int i = 0; i < 3; i++) {
        // printf("Reset attempt %d/3:\r\n", i + 1);
        MCP2515_Reset();
        osDelay(100);
        
        uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
        // printf("  CANSTAT: 0x%02X\r\n", canstat);
        
        if (canstat != 0xFF) {
            // printf("  [OK] MCP2515 responding\r\n");
            break;
        } else {
            // printf("  [ERROR] No response\r\n");
        }
    }
    
    // 3. Force initialization attempt
    // printf("\r\nStep 3: Force initialization attempt...\r\n");
    
    // Directly set configuration mode
    // printf("Attempting to force configuration mode...\r\n");
    MCP2515_WriteRegister(MCP2515_CANCTRL, MCP2515_MODE_CONFIG);
    osDelay(50);
    
    uint8_t mode = MCP2515_GetMode();
    // printf("Current mode: 0x%02X\r\n", mode);
    
    if (mode == MCP2515_MODE_CONFIG) {
        // printf("[OK] Successfully entered configuration mode\r\n");
        
        // Try to configure baud rate
        // printf("Configuring 500K baud rate...\r\n");
        MCP2515_WriteRegister(MCP2515_CNF1, 0x00);
        MCP2515_WriteRegister(MCP2515_CNF2, 0xB1);
        MCP2515_WriteRegister(MCP2515_CNF3, 0x85);
        
        // Configure receive buffers
        MCP2515_WriteRegister(MCP2515_RXB0CTRL, 0x60);
        MCP2515_WriteRegister(MCP2515_RXB1CTRL, 0x60);
        
        // Clear interrupt flags
        MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
        
        // Try to switch to normal mode
        // printf("Switching to normal mode...\r\n");
        if (MCP2515_SetMode(MCP2515_MODE_NORMAL) == MCP2515_OK) {
            // printf("[OK] Force initialization successful!\r\n");
            // printf("\r\n--- Starting CANOE Test Mode ---\r\n");
            MCP2515_CANOETest();
        } else {
            // printf("[ERROR] Failed to switch to normal mode\r\n");
        }
    } else {
        // printf("[ERROR] Cannot enter configuration mode\r\n");
    }
    
    // printf("\r\n=== Diagnosis completed ===\r\n");
}

/**
  * @brief  Complete CAN problem diagnosis and repair process
  * @param  None
  * @retval None
  */
void CAN_DiagnoseAndFix(void)
{
    // printf("\r\nStarting CAN problem diagnosis and repair process...\r\n");
    
    // Step 1: Diagnose current error status
    MCP2515_DiagnoseErrors();
    
    // Step 2: Clear error flags
    MCP2515_ClearAllErrors();
    
    // Step 3: Loopback mode test
    if (MCP2515_LoopbackTest() == MCP2515_OK) {
        // printf("\r\nSUCCESS: MCP2515 hardware is functioning normally\r\n");
        // printf("Possible issues:\r\n");
        // printf("   1. No other CAN nodes on the bus to acknowledge\r\n");
        // printf("   2. Termination resistors not properly installed\r\n");
        // printf("   3. CAN transceiver connection problems\r\n");
        
        // printf("\r\nSuggested solutions:\r\n");
        // printf("   1. Add 120 ohm resistor between CAN_H and CAN_L\r\n");
        // printf("   2. Connect a second CAN node or CAN analyzer\r\n");
        // printf("   3. Check TJA1050 transceiver connections\r\n");
        
        // printf("\r\n--- Starting CANOE Test Mode ---\r\n");
        // printf("Since hardware is OK, testing CAN transmission for CANOE...\r\n");
        MCP2515_CANOETest();
    } else {
        // printf("\r\nERROR: MCP2515 hardware may have problems\r\n");
        // printf("Suggested checks:\r\n");
        // printf("   1. Verify SPI connections are correct\r\n");
        // printf("   2. Check MCP2515 power supply\r\n");
        // printf("   3. Verify crystal oscillator is working\r\n");
    }
    
    // Step 4: Re-initialize
    // printf("\r\nRe-initializing MCP2515...\r\n");
    if (MCP2515_Init(MCP2515_BAUD_500K) == MCP2515_OK) {
        // printf("OK: MCP2515 re-initialization successful\r\n");
    } else {
        // printf("ERROR: MCP2515 re-initialization failed\r\n");
    }
    
    // printf("\r\nDiagnosis and repair process completed\r\n");
}

/*
// MCP2515相关函数已注释 - 硬件已移除
/*
  * @brief  Hardware connection diagnosis function
  * @param  None
  * @retval None
  */
/*
void MCP2515_HardwareDiagnosis(void)
{
    printf("[MCP2515-DIAG] Starting hardware diagnosis...\r\n");
    
    // 1. Check CS pin status
    // GPIO_PinState cs_state = HAL_GPIO_ReadPin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin);
    // printf("[MCP2515-DIAG] CS pin state: %s\r\n", cs_state ? "HIGH" : "LOW");
    
    // 2. Check INT pin status
    // GPIO_PinState int_state = HAL_GPIO_ReadPin(MCP2515_INT_GPIO_Port, MCP2515_INT_Pin);
    // printf("[MCP2515-DIAG] INT pin state: %s\r\n", int_state ? "HIGH" : "LOW");
    
    // 3. Test CS pin control
    // printf("[MCP2515-DIAG] Testing CS pin control...\r\n");
    // HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
    // osDelay(10);
    // cs_state = HAL_GPIO_ReadPin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin);
    // printf("[MCP2515-DIAG] CS set HIGH: %s\r\n", cs_state ? "OK" : "FAIL");
    // 
    // HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET);
    // osDelay(10);
    // cs_state = HAL_GPIO_ReadPin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin);
    // printf("[MCP2515-DIAG] CS set LOW: %s\r\n", cs_state ? "FAIL" : "OK");
    // 
    // HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
    
    // 4. Test basic SPI communication
    // printf("[MCP2515-DIAG] Testing SPI basic communication...\r\n");
    // uint8_t test_patterns[] = {0x00, 0xFF, 0x55, 0xAA, 0x5A, 0xA5};
    // 
    // for(int i = 0; i < 6; i++) {
    //     uint8_t tx_data = test_patterns[i];
    //     uint8_t rx_data = 0;
    //     
    //     HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET);
    //     HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, &tx_data, &rx_data, 1, 100);
    //     HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
    //     
    //     printf("[MCP2515-DIAG] Pattern 0x%02X -> 0x%02X (Status: %s)\r\n", 
    //            tx_data, rx_data, (status == HAL_OK) ? "OK" : "ERROR");
    //     
    //     if(status != HAL_OK) {
    //         printf("[MCP2515-DIAG] SPI Error State: 0x%08lX\r\n", hspi1.ErrorCode);
    //     }
    //     
    //     osDelay(10);
    // }
    
    // 5. Check SPI clock and data lines
    printf("[MCP2515-DIAG] Hardware connection checklist:\r\n");
    printf("[MCP2515-DIAG] - SCK (PB3) -> MCP2515 Pin 13\r\n");
    printf("[MCP2515-DIAG] - MISO (PB4) -> MCP2515 Pin 14\r\n");
    printf("[MCP2515-DIAG] - MOSI (PB5) -> MCP2515 Pin 12\r\n");
    printf("[MCP2515-DIAG] - CS (PB12) -> MCP2515 Pin 11\r\n");
    printf("[MCP2515-DIAG] - INT (PB10) -> MCP2515 Pin 21\r\n");
    printf("[MCP2515-DIAG] - VCC -> 3.3V or 5V\r\n");
    printf("[MCP2515-DIAG] - GND -> Ground\r\n");
    printf("[MCP2515-DIAG] - Crystal: 8MHz or 16MHz on pins 7&8\r\n");
    
    printf("[MCP2515-DIAG] Hardware diagnosis completed.\r\n");
}
*/

/*
// MCP2515相关函数已注释 - 硬件已移除
/*
  * @brief  Verify MCP2515 initialization status
  * @param  None
  * @retval None
  */
/*
void MCP2515_VerifyInitialization(void)
{
    printf("[MCP2515-VERIFY] Starting initialization verification...\r\n");
    
    // 1. Check working mode
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    uint8_t current_mode = canstat & 0xE0;
    printf("[MCP2515-VERIFY] Current mode: 0x%02X (%s)\r\n", current_mode,
           (current_mode == MCP2515_MODE_NORMAL) ? "NORMAL" :
           (current_mode == MCP2515_MODE_CONFIG) ? "CONFIG" :
           (current_mode == MCP2515_MODE_LOOPBACK) ? "LOOPBACK" :
           (current_mode == MCP2515_MODE_LISTENONLY) ? "LISTEN-ONLY" :
           (current_mode == MCP2515_MODE_SLEEP) ? "SLEEP" : "UNKNOWN");
    
    // 2. Check interrupt configuration
    uint8_t caninte = MCP2515_ReadRegister(MCP2515_CANINTE);
    uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    printf("[MCP2515-VERIFY] Interrupt config: CANINTE=0x%02X, CANINTF=0x%02X\r\n", caninte, canintf);
    
    if (caninte & MCP2515_INT_RX0IF) printf("[MCP2515-VERIFY] - RX0 interrupt enabled\r\n");
    if (caninte & MCP2515_INT_RX1IF) printf("[MCP2515-VERIFY] - RX1 interrupt enabled\r\n");
    if (caninte & MCP2515_INT_TX0IF) printf("[MCP2515-VERIFY] - TX0 interrupt enabled\r\n");
    if (caninte & MCP2515_INT_TX1IF) printf("[MCP2515-VERIFY] - TX1 interrupt enabled\r\n");
    if (caninte & MCP2515_INT_TX2IF) printf("[MCP2515-VERIFY] - TX2 interrupt enabled\r\n");
    if (caninte & MCP2515_INT_ERRIF) printf("[MCP2515-VERIFY] - Error interrupt enabled\r\n");
    
    // 3. Check receive buffer configuration
    uint8_t rxb0ctrl = MCP2515_ReadRegister(MCP2515_RXB0CTRL);
    uint8_t rxb1ctrl = MCP2515_ReadRegister(MCP2515_RXB1CTRL);
    printf("[MCP2515-VERIFY] RX buffer config: RXB0CTRL=0x%02X, RXB1CTRL=0x%02X\r\n", rxb0ctrl, rxb1ctrl);
    
    if ((rxb0ctrl & 0x60) == 0x60) printf("[MCP2515-VERIFY] - RXB0: Accept all messages (filters disabled)\r\n");
    if ((rxb1ctrl & 0x60) == 0x60) printf("[MCP2515-VERIFY] - RXB1: Accept all messages (filters disabled)\r\n");
    
    // 4. Check baud rate configuration
    uint8_t cnf1 = MCP2515_ReadRegister(MCP2515_CNF1);
    uint8_t cnf2 = MCP2515_ReadRegister(MCP2515_CNF2);
    uint8_t cnf3 = MCP2515_ReadRegister(MCP2515_CNF3);
    printf("[MCP2515-VERIFY] Baud rate config: CNF1=0x%02X, CNF2=0x%02X, CNF3=0x%02X\r\n", cnf1, cnf2, cnf3);
    
    // 5. Check error status
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    uint8_t tec = MCP2515_ReadRegister(MCP2515_TEC);
    uint8_t rec = MCP2515_ReadRegister(MCP2515_REC);
    printf("[MCP2515-VERIFY] Error status: EFLG=0x%02X, TEC=%d, REC=%d\r\n", eflg, tec, rec);
    
    if (eflg == 0x00) {
        printf("[MCP2515-VERIFY] - No error flags set\r\n");
    } else {
        if (eflg & 0x80) printf("[MCP2515-VERIFY] - RX1OVR: Receive Buffer 1 Overflow\r\n");
        if (eflg & 0x40) printf("[MCP2515-VERIFY] - RX0OVR: Receive Buffer 0 Overflow\r\n");
        if (eflg & 0x20) printf("[MCP2515-VERIFY] - TXBO: Bus-Off State\r\n");
        if (eflg & 0x10) printf("[MCP2515-VERIFY] - TXEP: Transmit Error Passive\r\n");
        if (eflg & 0x08) printf("[MCP2515-VERIFY] - RXEP: Receive Error Passive\r\n");
        if (eflg & 0x04) printf("[MCP2515-VERIFY] - TXWAR: Transmit Error Warning\r\n");
        if (eflg & 0x02) printf("[MCP2515-VERIFY] - RXWAR: Receive Error Warning\r\n");
        if (eflg & 0x01) printf("[MCP2515-VERIFY] - EWARN: Error Warning\r\n");
    }
    
    // 6. Check INT pin status
    // GPIO_PinState int_pin = HAL_GPIO_ReadPin(MCP2515_INT_GPIO_Port, MCP2515_INT_Pin);
    // printf("[MCP2515-VERIFY] INT pin state: %s\r\n", int_pin ? "HIGH (inactive)" : "LOW (active)");
    
    // 7. Summarize verification results
    printf("[MCP2515-VERIFY] Verification summary:\r\n");
    if (current_mode == MCP2515_MODE_NORMAL) {
        printf("[MCP2515-VERIFY] ✓ Mode: Normal mode active\r\n");
    } else {
        printf("[MCP2515-VERIFY] ✗ Mode: Not in normal mode\r\n");
    }
    
    if (caninte & (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF)) {
        printf("[MCP2515-VERIFY] ✓ Interrupts: RX interrupts enabled\r\n");
    } else {
        printf("[MCP2515-VERIFY] ✗ Interrupts: RX interrupts not enabled\r\n");
    }
    
    if ((rxb0ctrl & 0x60) == 0x60 && (rxb1ctrl & 0x60) == 0x60) {
        printf("[MCP2515-VERIFY] ✓ RX Buffers: Configured to accept all messages\r\n");
    } else {
        printf("[MCP2515-VERIFY] ✗ RX Buffers: Filter configuration may block messages\r\n");
    }
    
    if (eflg == 0x00) {
        printf("[MCP2515-VERIFY] ✓ Errors: No error flags set\r\n");
    } else {
        printf("[MCP2515-VERIFY] ✗ Errors: Error flags detected\r\n");
    }
    
    printf("[MCP2515-VERIFY] Initialization verification completed.\r\n");
}
*/
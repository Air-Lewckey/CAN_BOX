/**
 * @file main_integration_example.c
 * @brief PEPS测试代码集成到main函数的完整示例
 * @author 正点原子团队
 * @version 1.0
 * @date 2024-01-15
 * 
 * 本文件展示如何将PEPS测试代码集成到STM32项目的main函数中
 * 提供了完整的初始化和测试流程
 */

/* ========================= 包含头文件 ========================= */

#include "main.h"
#include "can.h"
#include "usart.h"
#include "gpio.h"

// PEPS测试相关头文件
#include "peps_simple_test.h"      // 简单测试函数（推荐）
#include "peps_api_test.h"         // FreeRTOS任务测试函数（可选）
#include "can_testbox_api.h"       // CAN测试盒API
#include "can_testbox_peps_helper.h" // PEPS辅助函数

#include <stdio.h>

/* ========================= 私有函数声明 ========================= */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_USART1_UART_Init(void);

/* ========================= 重定向printf到UART ========================= */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ========================= 主函数 ========================= */

/**
 * @brief 主函数 - 集成PEPS测试的完整示例
 * @retval int
 */
int main(void)
{
    /* ==================== MCU配置 ==================== */
    
    // 复位所有外设，初始化Flash接口和Systick
    HAL_Init();
    
    // 配置系统时钟
    SystemClock_Config();
    
    /* ==================== 初始化所有配置的外设 ==================== */
    
    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_USART1_UART_Init();
    
    /* ==================== 系统启动信息 ==================== */
    
    printf("\r\n\r\n");
    printf("========================================\r\n");
    printf("    STM32F407 CAN测试盒 - PEPS测试\r\n");
    printf("    正点原子@ALIENTEK\r\n");
    printf("    固件版本: V1.0\r\n");
    printf("========================================\r\n");
    
    // 等待系统稳定
    printf("[Main] 系统初始化完成，等待2秒...\r\n");
    HAL_Delay(2000);
    
    /* ==================== PEPS测试选择 ==================== */
    
    printf("[Main] 开始PEPS功能测试...\r\n");
    
    // 方法1：快速连通性测试（推荐用于调试和验证基本功能）
    printf("[Main] 执行快速连通性测试...\r\n");
    PEPS_Quick_Connectivity_Test();
    
    HAL_Delay(3000);  // 测试间隔
    
    // 方法2：完整功能测试（推荐用于全面验证）
    printf("[Main] 执行完整功能测试...\r\n");
    PEPS_Simple_Test_All_Messages();
    
    /* ==================== 可选：FreeRTOS任务测试 ==================== */
    
    #ifdef USE_FREERTOS_TESTS
    // 如果需要使用FreeRTOS任务进行测试，可以取消注释以下代码
    printf("[Main] 启动FreeRTOS测试任务...\r\n");
    
    // 启动所有PEPS测试任务
    CAN_TestBox_Status_t task_status = PEPS_Test_StartAllTests();
    if (task_status == CAN_TESTBOX_OK) {
        printf("[Main] FreeRTOS测试任务启动成功\r\n");
    } else {
        printf("[Main] FreeRTOS测试任务启动失败: %d\r\n", task_status);
    }
    #endif
    
    /* ==================== 主循环 ==================== */
    
    printf("[Main] 进入主循环，每10秒执行一次快速测试...\r\n");
    
    uint32_t loop_count = 0;
    
    while (1)
    {
        // 每10秒执行一次快速连通性测试
        HAL_Delay(10000);
        
        loop_count++;
        printf("\r\n[Main] 循环 #%lu - 执行定期连通性检查...\r\n", loop_count);
        
        PEPS_Quick_Connectivity_Test();
        
        // 可选：每5次循环执行一次完整测试
        if (loop_count % 5 == 0) {
            printf("[Main] 执行定期完整测试...\r\n");
            PEPS_Simple_Test_All_Messages();
        }
    }
}

/* ========================= 系统配置函数 ========================= */

/**
 * @brief 系统时钟配置
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /** Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    
    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO初始化函数
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
 * @brief 错误处理函数
 * @retval None
 */
void Error_Handler(void)
{
    /* 用户可以在这里添加自己的错误处理代码 */
    printf("[ERROR] 系统错误，进入错误处理模式\r\n");
    
    __disable_irq();
    while (1)
    {
        // 错误指示：LED闪烁或其他指示
        HAL_Delay(500);
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief 断言失败时的报告函数
 * @param file: 指向源文件名的指针
 * @param line: 断言失败的行号
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printf("[ASSERT] 断言失败: file %s on line %lu\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */

/* ========================= 使用说明 ========================= */

/*
集成步骤：

1. 将以下文件添加到您的项目中：
   - peps_simple_test.c/.h     (简单测试函数)
   - peps_api_test.c/.h        (FreeRTOS任务测试函数，可选)
   - can_testbox_api.c/.h      (CAN测试盒API)
   - can_testbox_peps_helper.c/.h (PEPS辅助函数)

2. 在您的main.c中包含必要的头文件：
   #include "peps_simple_test.h"
   #include "can_testbox_api.h"

3. 确保正确初始化：
   - HAL库
   - 系统时钟
   - CAN外设
   - UART外设（用于printf输出）

4. 在main函数中调用测试函数：
   PEPS_Quick_Connectivity_Test();        // 快速测试
   PEPS_Simple_Test_All_Messages();       // 完整测试

5. 编译配置：
   - 确保包含所有必要的头文件路径
   - 链接所有相关的.c文件
   - 如果使用FreeRTOS，定义USE_FREERTOS_TESTS宏

6. 硬件连接：
   - 确保CAN总线正确连接
   - 确保目标ECU已准备好接收PEPS报文
   - 串口连接用于查看测试输出

测试输出：
- 所有测试信息通过printf输出到串口
- 可以通过串口助手查看详细的测试过程和结果
- 测试包括：SCW1唤醒、SCW2唤醒、钥匙位置、BSI状态等

注意事项：
- 简单测试函数使用HAL_Delay()，会阻塞主线程
- FreeRTOS任务测试函数可以并行运行，不阻塞主线程
- 确保CAN总线波特率配置正确
- 测试前请确认目标设备已准备好接收报文
*/
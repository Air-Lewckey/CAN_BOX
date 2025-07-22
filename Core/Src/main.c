/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can.h"
#include "usart.h"
#include "can_simple_demo.h"
#include "can2_demo.h"  // 新增CAN2演示模块
#include "can2_test.h"  // 新增CAN2测试功能
#include "can1_can2_bridge_test.h"
#include "can_dual_node.h"  // 添加双节点通信头文件
// #include "mcp2515_test_demo.h"  // 注释MCP2515相关代码
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* CAN和UART句柄在各自的模块文件中定义 */
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANSendTask */
osThreadId_t CANSendTaskHandle;
const osThreadAttr_t CANSendTask_attributes = {
  .name = "CANSendTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANReceiveTask */
osThreadId_t CANReceiveTaskHandle;
const osThreadAttr_t CANReceiveTask_attributes = {
  .name = "CANReceiveTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myQueue01 */
osMessageQueueId_t myQueue01Handle;
const osMessageQueueAttr_t myQueue01_attributes = {
  .name = "myQueue01"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
void MX_USART2_UART_Init(void);
void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void StartDefaultTask(void *argument);
void StartCANSendTask(void *argument);
void StartCANReceiveTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
  // Initialize CAN1 Simple Demo
  if (CAN_SimpleDemo_Init() == HAL_OK) {
    // printf("\r\n=== CAN2 Receiver Mode ===\r\n");
    // printf("CAN1: Initialized (Silent Mode)\r\n");
  } else {
    // printf("\r\nCAN1 initialization failed!\r\n");
    Error_Handler();
  }
  
  // Initialize CAN2 Demo (仅接收模式)
  if (CAN2_Demo_Init() == HAL_OK) {
    // printf("CAN2: Initialized (Receive Only Mode)\r\n");
    // printf("CAN2 will display all CAN bus messages\r\n\r\n");
  } else {
    // printf("\r\nCAN2 initialization failed!\r\n");
    Error_Handler();
  }
  
  // CAN2增强测试已禁用 - 仅接收模式
  // if (CAN2_Test_Init() == HAL_OK) {
  //   printf("CAN2 Enhanced Test: Initialized\r\n");
  // } else {
  //   printf("CAN2 Enhanced Test initialization failed!\r\n");
  // }
  
  // CAN1-CAN2桥接测试已禁用 - CAN2仅接收
  // if (CAN1_CAN2_BridgeTest_Init() == HAL_OK) {
  //   printf("\r\n=== CAN1-CAN2 Bridge Test ===\r\n");
  //   printf("CAN1-CAN2 Bridge Test: Initialized\r\n");
  // } else {
  //   printf("CAN1-CAN2 Bridge Test initialization failed!\r\n");
  // }
  
  /*
  // MCP2515相关代码已注释 - 硬件已移除
  // Initialize MCP2515 Test Demo
  if (MCP2515_TestDemo_Init() == HAL_OK) {
    printf("MCP2515 Status: Initialized (500Kbps)\r\n");
    printf("\r\n=== Dual CAN System Configuration ===\r\n");
    printf("CAN1 Message ID: 0x100-0x500 (STM32 Built-in CAN)\r\n");
    printf("MCP2515 Message ID: 0x600-0x670 (External CAN Controller)\r\n");
    printf("Starting periodic test message transmission...\r\n\r\n");
  } else {
    printf("\r\nMCP2515 Test Demo initialization failed!\r\n");
    printf("Continue running CAN1 test...\r\n\r\n");
  }
  */
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myQueue01 */
  myQueue01Handle = osMessageQueueNew (10, 13, &myQueue01_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CANSendTask */
  CANSendTaskHandle = osThreadNew(StartCANSendTask, NULL, &CANSendTask_attributes);

  /* creation of CANReceiveTask */
  CANReceiveTaskHandle = osThreadNew(StartCANReceiveTask, NULL, &CANReceiveTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* CAN1-CAN2桥接测试任务已禁用 - CAN2仅作为接收器 */
  // const osThreadAttr_t bridgeTestTaskAttributes = {
  //   .name = "BridgeTestTask",
  //   .stack_size = 512 * 4,
  //   .priority = (osPriority_t) osPriorityNormal,
  // };
  // osThreadNew(CAN1_CAN2_BridgeTest_Task, NULL, &bridgeTestTaskAttributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
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
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

/* CAN1和CAN2初始化函数已移至can.c文件中 */

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/* USART2初始化函数已移至usart.c文件中 */

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /*
  // MCP2515相关GPIO初始化代码已注释 - 硬件已移除
  // Set MCP2515 CS pin to high level (deselected state)
  HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET);
  */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  Redirect printf to USART2
  * @param  file: File descriptor
  * @param  ptr: Data pointer
  * @param  len: Data length
  * @retval Number of bytes sent
  */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

/* CAN接收回调函数已移至can_dual_node.c文件中 */
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* 恢复CAN1和CAN2的状态监控 */
  
  /* Infinite loop */
  for(;;)
  {
    // 打印CAN1和CAN2的统计信息
    // CAN_PrintStats();
    // CAN_PrintNodeStatus();
    
    // 打印CAN1状态
    // uint32_t can1_state = HAL_CAN_GetState(&hcan1);
    // printf("CAN1 State: %lu\r\n", can1_state);
    
    // 打印CAN2状态
    // uint32_t can2_state = HAL_CAN_GetState(&hcan2);
    // printf("CAN2 State: %lu\r\n", can2_state);
    
    // printf("\r\n");
    
    osDelay(10000);  // 10秒延时
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartCANSendTask */
/**
* @brief Function implementing the CANSendTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCANSendTask */
void StartCANSendTask(void *argument)
{
  /* USER CODE BEGIN StartCANSendTask */
  /* CAN1发送任务 - 恢复CAN1发送功能 */
  /* Infinite loop */
  for(;;)
  {
    CAN_SimpleDemo_Task(argument);  // 恢复CAN1发送消息
    osDelay(10);  // 正常延时
  }
  /* USER CODE END StartCANSendTask */
}

/* USER CODE BEGIN Header_StartCANReceiveTask */
/**
* @brief Function implementing the CANReceiveTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCANReceiveTask */
void StartCANReceiveTask(void *argument)
{
  /* USER CODE BEGIN StartCANReceiveTask */
  /* Infinite loop */
  for(;;)
  {
    // 启用CAN2任务以进行状态监控和硬件连接测试
    CAN2_Demo_Task(argument);  // 重新启用 - 用于状态监控和连接测试
    // CAN2_Test_Task();          // 保持禁用 - CAN2仅接收
    
    osDelay(10);  // 减少延时以确保任务正常运行
  }
  
  /*
  // MCP2515相关任务代码已注释 - 硬件已移除
  // Run MCP2515 test demo task directly (it has its own infinite loop)
  MCP2515_TestDemo_Task(argument);
  */
  /* USER CODE END StartCANReceiveTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

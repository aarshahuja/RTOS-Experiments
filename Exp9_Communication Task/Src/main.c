#include "main.h"
#include "cmsis_os.h"
#include<stdio.h>

/* Thread handles */
osThreadId_t Sensing_TaskHandle;
osThreadId_t Navigation_TaskHandle;

/* Queue handle */
osMessageQueueId_t myQueue01Handle;

/* Thread attributes */
const osThreadAttr_t Sensing_Task_attributes = {
  .name = "Sensing_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t Navigation_Task_attributes = {
  .name = "Navigation_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* ✅ FIXED: Correct function names */
void Sensor_Read(void *argument);
void Motion_Control(void *argument);

/* SWV printf support */
int _write(int file, char *ptr, int len) {
  for (int i = 0; i < len; i++) {
    ITM_SendChar(*ptr++);
  }
  return len;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* Init RTOS */
  osKernelInitialize();

  /* Create Queue */
  myQueue01Handle = osMessageQueueNew(16, sizeof(unsigned int), NULL);

  /* ✅ FIXED: Correct task function */
  Sensing_TaskHandle = osThreadNew(Sensor_Read, NULL, &Sensing_Task_attributes);
  Navigation_TaskHandle = osThreadNew(Motion_Control, NULL, &Navigation_Task_attributes);

  /* Start scheduler */
  osKernelStart();

  while (1)
  {
  }
}

/* ================= TASKS ================= */

/* Producer Task */
void Sensor_Read(void *argument)
{
  unsigned int dist = 0;

  for(;;)
  {
    printf("Inside Data Producer Task\n");

    dist++;
    osMessageQueuePut(myQueue01Handle, &dist, 0, osWaitForever);

    osDelay(1000);
  }
}

/* Consumer Task */
void Motion_Control(void *argument)
{
  unsigned int distance;

  for(;;)
  {
    printf("Inside Data Consumer Task\n");

    osMessageQueueGet(myQueue01Handle, &distance, NULL, osWaitForever);

    printf("Distance is %u\n", distance);
  }
}

/* ================= GPIO ================= */

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ================= CLOCK ================= */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;

  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                               RCC_CLOCKTYPE_HCLK |
                               RCC_CLOCKTYPE_PCLK1 |
                               RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ================= ERROR ================= */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

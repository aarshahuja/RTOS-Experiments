/* USER CODE BEGIN Header */
/*
==============================================================================
RTOS BASED SMART WATER TANK INDICATOR
STM32F446RE + HC-SR04 + LEDs + Buzzer + Tera Term
==============================================================================
*/
/* USER CODE END Header */

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart2;

QueueHandle_t WaterQueue;

/* Function Prototypes -------------------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

void StartSensorTask(void *argument);
void StartAlertTask(void *argument);

uint32_t Read_Distance(void);

/* Redirect printf to UART ---------------------------------------------------*/

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* Ultrasonic Distance Function ----------------------------------------------*/

uint32_t Read_Distance(void)
{
    uint32_t duration = 0;
    uint32_t distance = 0;

    /* Send Trigger Pulse */

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    /* Wait for Echo High */

    while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET);

    uint32_t start = HAL_GetTick();

    /* Wait for Echo Low */

    while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET);

    duration = HAL_GetTick() - start;

    /* Convert Time to Distance */

    distance = duration * 17;

    return distance;
}

/* Main Function -------------------------------------------------------------*/

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    /* Create Queue */

    WaterQueue = xQueueCreate(5, sizeof(int));

    /* Create Sensor Task */

    xTaskCreate(StartSensorTask,
                "SensorTask",
                128,
                NULL,
                2,
                NULL);

    /* Create Alert Task */

    xTaskCreate(StartAlertTask,
                "AlertTask",
                128,
                NULL,
                1,
                NULL);

    /* Start Scheduler */

    vTaskStartScheduler();

    while (1)
    {

    }
}

/* Sensor Task ---------------------------------------------------------------*/

void StartSensorTask(void *argument)
{
    int distance;

    while(1)
    {
        distance = Read_Distance();

        xQueueSend(WaterQueue, &distance, portMAX_DELAY);

        printf("\r\n");
        printf("[SensorTask] Distance = %d cm\r\n", distance);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Alert Task ----------------------------------------------------------------*/

void StartAlertTask(void *argument)
{
    int distance;

    while(1)
    {
        if(xQueueReceive(WaterQueue, &distance, portMAX_DELAY))
        {
            /* FULL TANK */

            if(distance < 5)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);

                printf("[AlertTask] TANK FULL\r\n");
            }

            /* MEDIUM LEVEL */

            else if(distance >= 5 && distance < 15)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);

                printf("[AlertTask] TANK MEDIUM\r\n");
            }

            /* LOW LEVEL */

            else
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);

                printf("[AlertTask] TANK LOW\r\n");
            }
        }
    }
}

/* GPIO Initialization -------------------------------------------------------*/

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO Clocks */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* TRIG PIN -> PA0 */

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ECHO PIN -> PA1 */

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LED + BUZZER PINS */

    GPIO_InitStruct.Pin = GPIO_PIN_0 |
                          GPIO_PIN_1 |
                          GPIO_PIN_2 |
                          GPIO_PIN_10;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USART2 Initialization -----------------------------------------------------*/

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;

    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* System Clock Configuration ------------------------------------------------*/

void SystemClock_Config(void)
{

}

/* Error Handler -------------------------------------------------------------*/

void Error_Handler(void)
{
    while(1)
    {

    }
}

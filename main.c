/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

Задание
С использованием средств демо-платы и ОСРВ FreeRTOS реализовать игру под названием
«Повтори за мной».
Игра делится на раунды, в каждом раунде она показывает пользователю комбинацию вспышек
светодиодов. Игрок должен воспроизвести последовательность этих вспышек с помощью кнопок.
Развитие игры между раундами представляется в виде «снежного кома», это значит, что в новом раунде
игра показывает все вспышки из предыдущего и дополняет её 1 новой. 
Основные условия:
1. Для отображения комбинации вспышек используются пользовательские светодиоды, для ввода
используются пользовательские кнопки на демо-плате. Количество определяется разработчиком, но в
игре должно быть задействовано максимально возможное количество пользовательских светодиодов
и кнопок.
2. Для взаимодействия с пользователем (выбор уровня сложности, вывод информационных сообщений)
должен использоваться последовательный интерфейс UART с параметрами: 115200 бит/с, 8 бит
данных, 1 стоповый бит, без контроля чётности.
3. В игре должны присутствовать два уровня сложности. Каждый уровень определяет время, в течение
которого система будет удерживать светодиод во включённом состоянии для запоминания
пользователем текущей вспышки. Длительность удержания по уровням, следующая:
  Первый уровень – 1 секунда,
  Второй уровень – 0.5 секунды.
4. Требование к начальному состоянию: после включения и загрузки игра выводит в терминал игроку
строку приветствия и предлагает выбрать уровень сложности.
5. Выбор уровня происходит при отправке пользователем соответствующей команды в терминале
(формат команды: «1» - для первого уровня, «2» - для второго уровня). Игра начинается с задержкой
2 секунды после выбора сложности, – это необходимо чтобы игрок успел переключить внимание на
светодиоды и кнопки демо-платы.
6. Пользователь не может сменить уровень сложности, если уже начал игру.
7. Каждый элемент «снежного кома» должен быть сгенерирован случайным образом (с использованием
аппаратного генератора случайных чисел микроконтроллера или с использованием функции rand).
Любой элемент «снежного кома» всегда соответствует вспышке одного светодиода, не существует
ситуации, когда элементом является выключенное состояние всех диодов.
8. Допускается два варианта реализации «снежного кома» (выбирается разработчиком самостоятельно):

Максимальный размер «снежного кома» установить равным 5 (определить через директиву #define
для гибкой смены при необходимости).
10. После отображения всех вспышек на текущем раунде, игра ждёт начала ввода от пользователя.
Ожидание действия игрока выполнить с использованием программного таймера в интервальном
режиме с периодом 3 секунды (определить через директиву #define для гибкой смены при
необходимости).
11. Каждое нажатие игровых кнопок пользователем:
   сигнализируется включением соответствующего светодиода,
   сбрасывает период таймера ожидания действия игрока.
12. Во время игры, основной алгоритм должен проверять на лету каждое нажатие кнопки пользователем.
Если во время ввода пользователь допустил ошибку, то игра сразу выводит сообщение в
последовательный интерфейс о том, что пользователь проиграл и снова предлагает выбрать уровень
сложности для запуска новой игры.
13. Если пользователь повторил без ошибок всю комбинацию последнего раунда игры, то происходит три
вспышки всеми светодиодами. По последовательному интерфейсу передаётся строка поздравления с
победой и предложение выбрать уровень сложности для старта новый игры.
14. Новая игра после проигрыша или победы выполняется по аналогичным правилам.
Последовательность вспышек для «снежного кома» каждой новой игры генерируется в виде новой
случайной последовательности.
15. Требование к подсистеме управления памятью – использовать смешанный режим: динамическая
схема heap_4 и одна иззадач реализованной системы должна быть создана со статической аллокацией.
16. Требование к подсистеме чтения состояния кнопок – использование прерываний с отложенной
обработкой и механизмом программного антидребезга. Допустимые варианты:
   С использованием семафора или очереди и отдельной задачи, программный антидребезг на
примитивной задержке (упрощённый вариант).
   С использованием очереди и отдельной задачи, программный антидребезг через
использование механизма программных таймеров в интервальном режиме (усложнённый
вариант).

USING STM32F407VET6 board:

SWD interface
PA9 & PA10 - USART1
Key1 - PE10
Key2 - PE11
Key3 - PE12
LED1 - PE13
LED2 - PE14
LED3 - PE15
*/
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "main.h"
#include "RCC_Init.h"
#include "gpio.h"
#include "usart_dbg.h"
#include "rng.h"

#include "string.h"
#include <stdio.h>
#include <stdlib.h>

/************************global values********************************/
#define SEMAPHORE_USART1_TIMEOUT 500
#define QUEUE_REC_NUM 3
/* Task static */
StaticTask_t xTaskButtonControlBlock;
StackType_t xTaskButtonStack[configMINIMAL_STACK_SIZE];
/* Task Handler */
xTaskHandle taskPreparation;
xTaskHandle taskGame;
/* Semaphore handler */
SemaphoreHandle_t semphr_usart1;
/* Queue handler */
xQueueHandle queueReceiveButton;
xQueueHandle queueReceiveValueButton;
/* Timer Handler */
xTimerHandle timeReceiveButton;

/* send buffer USART1 */
char *pt_buffer;

/* mode game */
#define EASY_MODE 1
#define NORMAL_MODE 2

typedef enum{
  STOP_GAME = 0,
  NORMAL_GAME = 1,
  GAME_OVER = 2,
  WIN_GAME = 3

}game_mode_t;
game_mode_t gameMode = STOP_GAME;

uint16_t gameTimePulse;
uint8_t *pxArrayRandLed;

uint8_t maxNumBlinkLed = 0;

/************************Callback func********************************/
void vEndTimeReceiveButton(xTimerHandle xTimer){
  EXTI->IMR &= ~(EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12);
  gameMode = GAME_OVER;
  vTaskDelete(taskGame);
  vTaskResume(taskPreparation);
}

/****************************Tasks************************************/
void vTaskButton(void * pvParameters){
  uint16_t receivButton;
  while(1){
    xQueueReceive(queueReceiveButton, &receivButton, portMAX_DELAY);
    vTaskDelay(mainTIMEOUT_BOUNCE);
    EXTI->IMR |= EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12;

    if(gameMode == NORMAL_GAME){
      xTimerReset(timeReceiveButton, 10);
      uint8_t valMsg = 0;
      switch(receivButton & (EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12)){
        case EXTI_PR_PR10:
          valMsg = 1;
          break;
        case EXTI_PR_PR11:
          valMsg = 2;
          break;
        case EXTI_PR_PR12:
          valMsg = 3;
          break;
      }
      xQueueSend(queueReceiveValueButton, &valMsg, 100);
    }
  }
}

void vTaskGame(void * pvParameters){
  uint8_t receivValueButton;
  while(gameMode == NORMAL_GAME){
    for(uint8_t level = 2; level<maxNumBlinkLed; level++){
      /* Led Blink */
      for(uint8_t cur_lvl=0; cur_lvl<=level; cur_lvl++){
        vLedOn(pxArrayRandLed[cur_lvl]);
        vTaskDelay(gameTimePulse);
        vLedOff(pxArrayRandLed[cur_lvl]);
        vTaskDelay(gameTimePulse);
      }
      //clear read buttons queue 
      vQueueDelete(queueReceiveValueButton);
      queueReceiveValueButton = xQueueCreate(mainMAX_NUM_BLINK_LED, sizeof(uint8_t));

      /* Read Buttons */
      xTimerStart(timeReceiveButton,0);
      EXTI->IMR |= EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12;
      for(uint8_t cur_lvl1=0; cur_lvl1<=level; cur_lvl1++){
        if(xQueueReceive(queueReceiveValueButton, &receivValueButton, mainTIMEOUT_BUTTON) == pdTRUE){
          if(pxArrayRandLed[cur_lvl1] == receivValueButton){
            /* right */
            vLedOn(pxArrayRandLed[cur_lvl1]);
            vTaskDelay(300);
            vLedOff(pxArrayRandLed[cur_lvl1]);
          }else{
            /* loos */
            EXTI->IMR &= ~(EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12);
            xTimerStop(timeReceiveButton, 10);
            gameMode = GAME_OVER;
            vTaskResume(taskPreparation);
            vTaskDelete(taskGame);
          }
        }
      }
      EXTI->IMR &= ~(EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12);
      xTimerStop(timeReceiveButton, 10);
      vTaskDelay(500);
    }
    /* afte all levels */
    gameMode = WIN_GAME;
    vTaskResume(taskPreparation);
    vTaskDelete(taskGame);
  }
  vTaskDelete(NULL);
}

void vPreparation(void * pvParameters){
  uint8_t arrayRandLed[maxNumBlinkLed];
  char chose[19] = {"You chose level  \n"};

  while(1){
    if(gameMode == GAME_OVER){
      vLedAllOff();
      xSemaphoreTake(semphr_usart1, portMAX_DELAY);
      send_USART_STR("Game Over\nYou loos!!!\n");
      gameMode = STOP_GAME;
    }
    if(gameMode == WIN_GAME){
      vLedAllOff();
      xSemaphoreTake(semphr_usart1, portMAX_DELAY);
      send_USART_STR("Game Over\nYou winn!!!\n");
      for(uint8_t blinc=0; blinc<3; blinc++){
        vLedAllOn();
        vTaskDelay(750);
        vLedAllOff();
        vTaskDelay(750);
      }
      gameMode = STOP_GAME;
    }
    while(gameMode == STOP_GAME){
      xSemaphoreTake(semphr_usart1, portMAX_DELAY);
      send_USART_STR("\nHello GAMERS\nPlease selected Difficulty level:\n1-easy\n2-normal\n");
      /* receive USART1 */
      xSemaphoreTake(semphr_usart1, portMAX_DELAY);
      USART1->CR1 |= USART_CR1_RE;
      while(!(USART1->SR & USART_SR_RXNE)){}
      char receive = (uint16_t)USART1->DR & (uint16_t)0x01FF;
      USART1->CR1 &= ~(USART_CR1_RE);
      xSemaphoreGive(semphr_usart1);
      chose[16] = receive;
      int8_t receive_numer = receive - '0';
      switch (receive_numer) {
        case EASY_MODE:
          gameMode = NORMAL_GAME;
          gameTimePulse = mainTIME_LED_ON_EASY;
        break;
        case NORMAL_MODE:
          gameMode  = NORMAL_GAME;
          gameTimePulse = mainTIME_LED_ON_NORMAL;
        break;
        default:
          gameMode = STOP_GAME;
          gameTimePulse = 0;
        break;
      }
    }
    xSemaphoreTake(semphr_usart1, portMAX_DELAY);
    send_USART_STR(chose);
    /* generate array */
    for(uint8_t i=0; i<maxNumBlinkLed; i++){
      arrayRandLed[i] = (((float)rng_random() / RNG_RANDOM_MAX)*3 + 1);
    }
    pxArrayRandLed = arrayRandLed;
    vTaskDelay(2000);
    /* start game */
    xTaskCreate(vTaskGame, "vTaskGame", configMINIMAL_STACK_SIZE, NULL, 1, &taskGame);

    vTaskSuspend(taskPreparation);
  }
  vTaskDelete(NULL);
}

/****************************main************************************/
 int main(void) {
 /* Init hardware */
  RCC_Init();
  GPIO_init();
  rng_init();
  
  /* values init */
  maxNumBlinkLed = mainMAX_NUM_BLINK_LED>3 ? mainMAX_NUM_BLINK_LED : 3;
  /* USART */
  usart1_init();
  semphr_usart1 = xSemaphoreCreateBinary();
  send_USART_STR("---system init---\n");
  /* Timers and Queues */
  queueReceiveButton = xQueueCreate(mainMAX_NUM_BLINK_LED, sizeof(uint16_t));
  queueReceiveValueButton = xQueueCreate(mainMAX_NUM_BLINK_LED, sizeof(uint8_t));
  timeReceiveButton = xTimerCreate("timeReceiveButton", mainTIMEOUT_BUTTON, pdFALSE, (void *) 0, vEndTimeReceiveButton);

  /* Tasks create */
  xTaskCreateStatic(vTaskButton, "vTaskButton", configMINIMAL_STACK_SIZE, NULL, 2, xTaskButtonStack, &xTaskButtonControlBlock);
  xTaskCreate(vPreparation, "vPreparation", configMINIMAL_STACK_SIZE, NULL, 1, &taskPreparation);
  
  vTaskStartScheduler();
  while(1){}
}
/****************************func************************************/
/* send String */
void send_USART_STR(char *sendbuffer){
  while((USART1->SR & USART_SR_TC) == 0){}
  pt_buffer = &(*sendbuffer);
  USART1->DR = (*pt_buffer & (uint16_t)0x01FF);
  USART1->CR1 |= USART_CR1_TCIE;
}
/************************Interrupt func******************************/
void USART1_IRQHandler(void){
  if(USART1->SR & USART_SR_TC){
    if(*++pt_buffer){
      USART1->DR = (*pt_buffer & (uint16_t)0x01FF);
    }else{
      USART1->CR1 &= ~(USART_CR1_TCIE);
      BaseType_t taskWoken;
      xSemaphoreGiveFromISR(semphr_usart1, &taskWoken);
      portYIELD_FROM_ISR(taskWoken);
    } 
  }
  NVIC_ClearPendingIRQ(USART1_IRQn);
}

void EXTI15_10_IRQHandler(void){
  if(EXTI->PR & (EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12)){
    EXTI->IMR &= ~(EXTI_IMR_IM10|EXTI_IMR_IM11|EXTI_IMR_IM12);
    BaseType_t taskWoken;
    uint16_t tempMSG = (EXTI->PR);
    xQueueSendFromISR(queueReceiveButton, &tempMSG, &taskWoken);
    EXTI->PR |= EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12;
    portYIELD_FROM_ISR(taskWoken);
    
  }
}
/************************Service func static allocation******************************/

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
   implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
   used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
       state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
       configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
   application must provide an implementation of vApplicationGetTimerTaskMemory()
   to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
       task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
      configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/************************* End of file ******************************/

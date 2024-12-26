#ifndef _ATC_H_
#define _ATC_H_

/***********************************************************************************************************

  Author:     Nima Askari
  Github:     https://www.github.com/NimaLTD
  LinkedIn:   https://www.linkedin.com/in/nimaltd
  Youtube:    https://www.youtube.com/@nimaltd
  Instagram:  https://instagram.com/github.NimaLTD

  Version:    4.2.2

  History:

              4.2.2
              - Fixed SetEvent
			  
              4.2.1
              - Fixed Debug print
              - Removed Temp Callback :)

              4.2.0
              - Fixed Returned response
              - Added Temp Callback

              4.1.2
              - Fixed Definitions
              
              4.1.1
              - Fixed RX Items counter

              4.1.0
              - Added ATC_Send and ATC_Receive functions
              - Changed declaration

              4.0.2
              - Fixed Debug Printing
        
              4.0.1
              - Fixed Initialization 
              - Changed ATC_Delay function to public
        
              4.0.0
              - Rewrite again
              - Working with RX/TX DMA, Less CPU load
              - A separate callback for each event received (auto searching string)
              - Support STM32CubeMx Packet installer

***********************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "NimaLTD.I-CUBE-ATC_conf.h"
#include "usart.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#if ATC_RTOS == ATC_RTOS_DISABLE
#elif ATC_RTOS == ATC_RTOS_CMSIS_V1
#include "cmsis_os.h"
#include "freertos.h"
#elif ATC_RTOS == ATC_RTOS_CMSIS_V2
#include "cmsis_os2.h"
#include "freertos.h"
#elif ATC_RTOS == ATC_RTOS_THREADX
#include "app_threadx.h"
#endif

/************************************************************************************************************
**************    Public Definitions
************************************************************************************************************/

#define ATC_RESP_MAX              5

#define ATC_RESP_ITEMS            -5
#define ATC_RESP_TX_BUSY          -4
#define ATC_RESP_MEM_ERROR        -3
#define ATC_RESP_SENDING_TIMEOUT  -2
#define ATC_RESP_SENDING_ERROR    -1
#define ATC_RESP_NOT_FOUND        0

/************************************************************************************************************
**************    Public struct/enum
************************************************************************************************************/

typedef struct
{
  char*                      Event;
  void                       (*EventCallback)(const char*);

} ATC_EventTypeDef;

typedef struct
{
  UART_HandleTypeDef*        hUart;
  char                       Name[8];
  ATC_EventTypeDef*          psEvents;
  uint32_t                   Events;
  uint16_t                   Size;
  uint16_t                   RespCount;
  uint16_t                   RxIndex;
  uint16_t                   TxLen;
  uint8_t*                   pRxBuff;
  uint8_t*                   pTxBuff;
  uint8_t*                   pReadBuff;
  uint8_t*                   ppResp[ATC_RESP_MAX];

} ATC_HandleTypeDef;

/************************************************************************************************************
**************    Public Functions
************************************************************************************************************/

bool    ATC_Init(ATC_HandleTypeDef* hAtc, UART_HandleTypeDef* hUart, uint16_t BufferSize, const char* pName);
void    ATC_DeInit(ATC_HandleTypeDef* hAtc);
bool    ATC_SetEvents(ATC_HandleTypeDef* hAtc, const ATC_EventTypeDef* psEvents);
void    ATC_Loop(ATC_HandleTypeDef* hAtc);
int     ATC_SendReceive(ATC_HandleTypeDef* hAtc, const char* pCommand, uint32_t TxTimeout, char** ppResp, uint32_t RxTimeout, uint8_t Items, ...);
bool    ATC_Send(ATC_HandleTypeDef* hAtc, const char* pCommand, uint32_t TxTimeout, ...);
int     ATC_Receive(ATC_HandleTypeDef* hAtc, char** ppResp, uint32_t RxTimeout, uint8_t Items, ...);

void    ATC_IdleLineCallback(ATC_HandleTypeDef* hAtc, uint16_t Len);
void    ATC_Delay(uint32_t Delay);

#ifdef __cplusplus
}
#endif
#endif

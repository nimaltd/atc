
/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "atc.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

#if ATC_DEBUG == ATC_DEBUG_DISABLE
#define dprintf(...)
#else
#define dprintf(...) printf(__VA_ARGS__)
#endif

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
**************    Private Definitions
************************************************************************************************************/

// none

/************************************************************************************************************
**************    Private Variables
************************************************************************************************************/

// none

/************************************************************************************************************
**************    Private Functions
************************************************************************************************************/

void              ATC_Delay(uint32_t Delay);
void*             ATC_Malloc(size_t Size);
void              ATC_Free(void *pMem);
void              ATC_RxFlush(ATC_HandleTypeDef *hAtc);
bool              ATC_TxRaw(ATC_HandleTypeDef *hAtc, const uint8_t *pData, uint16_t Len);
bool              ATC_TxBusy(ATC_HandleTypeDef *hAtc);
bool              ATC_TxWait(ATC_HandleTypeDef *hAtc, uint32_t Timeout);
void              ATC_CheckEvents(ATC_HandleTypeDef *hAtc);
uint8_t           ATC_CheckResponse(ATC_HandleTypeDef *hAtc, char *pFound);
void              ATC_CheckErrors(ATC_HandleTypeDef *hAtc);

/***********************************************************************************************************/

void ATC_Delay(uint32_t Delay)
{
#if ATC_RTOS == ATC_RTOS_DISABLE
  HAL_Delay(Delay);
#elif (ATC_RTOS == ATC_RTOS_CMSIS_V1) || (ATC_RTOS == ATC_RTOS_CMSIS_V2)
  uint32_t d = (configTICK_RATE_HZ * Delay) / 1000;
  if (d == 0)
      d = 1;
  osDelay(d);
#elif ATC_RTOS == ATC_RTOS_THREADX
  uint32_t d = (TX_TIMER_TICKS_PER_SECOND * Delay) / 1000;
  if (d == 0)
    d = 1;
  tx_thread_sleep(d);
#endif
}

/***********************************************************************************************************/

void* ATC_Malloc(size_t size)
{
  void *ptr = NULL;
#if RTOS_CONFIG == ATC_RTOS_DISABLE
  ptr = malloc(size);
#elif (ATC_RTOS == ATC_RTOS_CMSIS_V1) || (ATC_RTOS == ATC_RTOS_CMSIS_V2)
  ptr = pvPortMalloc(size);
#elif ATC_RTOS == ATC_RTOS_THREADX
  ??
#endif
  return ptr;
}

/***********************************************************************************************************/

void ATC_Free(void *ptr)
{
  if (ptr != NULL)
  {
#if RTOS_CONFIG == ATC_RTOS_DISABLE
    free(ptr);
#elif (ATC_RTOS == ATC_RTOS_CMSIS_V1) || (ATC_RTOS == ATC_RTOS_CMSIS_V2)
    vPortFree(ptr);
#elif ATC_RTOS == ATC_RTOS_THREADX
    ??
#endif
     ptr = NULL;
  }
}

/***********************************************************************************************************/

void ATC_RxFlush(ATC_HandleTypeDef *hAtc)
{
  hAtc->RxIndex = 0;
  memset(hAtc->pReadBuff, 0, hAtc->Size);
}

/***********************************************************************************************************/

bool ATC_TxRaw(ATC_HandleTypeDef *hAtc, const uint8_t *Data, uint16_t Len)
{
  bool answer = false;
  do
  {
    hAtc->TxLen = Len;
    if (HAL_UART_Transmit_DMA(hAtc->hUart, Data, Len) != HAL_OK)
    {
      break;
    }
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

bool ATC_TxBusy(ATC_HandleTypeDef *hAtc)
{
  if ((HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_BUSY_TX) || (HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_BUSY_TX_RX))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/***********************************************************************************************************/

bool ATC_TxWait(ATC_HandleTypeDef *hAtc, uint32_t Timeout)
{
  bool answer = false;
  uint32_t start_time = HAL_GetTick();
  while (1)
  {
    ATC_Delay(1);
    if ((HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_BUSY_RX) || (HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_READY))
    {
      answer = true;
      break;
    }
    if (HAL_GetTick() - start_time >= Timeout)
    {
      HAL_UART_AbortTransmit(hAtc->hUart);
      break;
    }
  }

  return answer;
}

/***********************************************************************************************************/

void ATC_CheckEvents(ATC_HandleTypeDef *hAtc)
{
  if (hAtc->RxIndex > 0)
  {
    uint16_t ev = 0;
    while ((hAtc->sEvents[ev].Event != NULL) && (hAtc->sEvents[ev].EventCallback != NULL))
    {
      char *found = strstr((char*)hAtc->pReadBuff, hAtc->sEvents[ev].Event);
      if (found != NULL)
      {
        hAtc->sEvents[ev].EventCallback(found);
      }
      ev++;
    }
    ATC_RxFlush(hAtc);
  }
}

/***********************************************************************************************************/

uint8_t ATC_CheckResponse(ATC_HandleTypeDef *hAtc, char *FoundPtr)
{
  uint8_t index = 0;
  if (hAtc->RxIndex > 0)
  {
    for (uint16_t i = 0; i < hAtc->RespCount; i++)
    {
      char *found = strstr((char*)hAtc->pReadBuff, (char*)hAtc->ppResp[i]);
      if (found != NULL)
      {
        if (FoundPtr != NULL)
        {
          FoundPtr = found;
        }
        index = i + 1;
        break;
      }
    }
  }
  return index;
}

/***********************************************************************************************************/

void ATC_CheckErrors(ATC_HandleTypeDef *hAtc)
{
  if (HAL_UART_GetError(hAtc->hUart) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    HAL_UART_AbortReceive(hAtc->hUart);
    HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size);
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
  }
}

/************************************************************************************************************
**************    Public Functions
************************************************************************************************************/

/**
  * @brief  Initializes the ATC handle structure.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  hUart: Pointer to the UART handle.
  * @param  BufferSize: Size of the RX buffer. It needs 2X memory.
  * @param  pName: Name identifier for the ATC.
  * @retval true if initialization is successful, false otherwise.
  */
bool ATC_Init(ATC_HandleTypeDef *hAtc, UART_HandleTypeDef *hUart, uint16_t BufferSize, char *pName)
{
  bool answer = false;
  do
  {
    if (hAtc == NULL || hUart == NULL)
    {
      break;
    }
    if (hAtc->hUart != NULL)
    {
      break;
    }
    memset(hAtc, 0, sizeof(ATC_HandleTypeDef));
    if (pName != NULL)
    {
      strncpy(hAtc->Name, pName, sizeof(hAtc->Name) - 1);
    }
    hAtc->hUart = hUart;
    hAtc->pRxBuff = ATC_Malloc(BufferSize);
    if (hAtc->pRxBuff != NULL)
    {
      memset(hAtc->pRxBuff, 0, BufferSize);
    }
    else
    {
      dprintf("[%s] ERROR MALLOC 1\r\n", hAtc->Name);
      break;
    }
    hAtc->pReadBuff = ATC_Malloc(BufferSize);
    if (hAtc->pReadBuff != NULL)
    {
      memset(hAtc->pReadBuff, 0, BufferSize);
    }
    else
    {
      dprintf("[%s] ERROR MALLOC 2\r\n", hAtc->Name);
      break;
    }
    hAtc->Size = BufferSize;
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    if (HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size) != HAL_OK)
    {
      dprintf("[%s] ERROR ENABLE RX DMA\r\n", hAtc->Name);
      break;
    }
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
    answer = true;

  } while (0);

  if (answer == false)
  {
    if (hAtc->pRxBuff != NULL)
    {
      ATC_Free(hAtc->pRxBuff);
    }
    if (hAtc->pReadBuff != NULL)
    {
      ATC_Free(hAtc->pReadBuff);
    }
    memset(hAtc, 0, sizeof(ATC_HandleTypeDef));
  }
  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  DeInitializes the ATC handle structure.
  * @param  hAtc: Pointer to the ATC handle.
  * @retval None.
  */
void ATC_DeInit(ATC_HandleTypeDef *hAtc)
{
  do
  {
    if (hAtc == NULL)
    {
      break;
    }
    if (hAtc->hUart == NULL)
    {
      break;
    }
    ATC_Free(hAtc->pRxBuff);
    ATC_Free(hAtc->pReadBuff);
    memset(hAtc, 0, sizeof(ATC_HandleTypeDef));

  } while (0);
}

/***********************************************************************************************************/

/**
  * @brief  Sets the ATC event handlers.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  sEvents: Pointer to the event handler structure.
  * @retval true if events are set successfully, false otherwise.
  */
bool ATC_SetEvents(ATC_HandleTypeDef *hAtc, const ATC_EventTypeDef *sEvents)
{
  bool answer = false;
  do
  {
    if (hAtc == NULL)
    {
      break;
    }
    if (sEvents == NULL)
    {
      break;
    }
    hAtc->sEvents = (ATC_EventTypeDef*)sEvents;
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Main loop for processing ATC events and errors.
  * @param  hAtc: Pointer to the ATC handle.
  * @retval None.
  */
void ATC_Loop(ATC_HandleTypeDef *hAtc)
{
  ATC_CheckErrors(hAtc);
  ATC_CheckEvents(hAtc);
}

/***********************************************************************************************************/

/**
  * @brief  Sends a command and waits for a response.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  pCommand: Pointer to the command string.
  * @param  TxTimeout: Timeout for sending the command.
  * @param  pResp: Pointer to the response buffer. It Can be NULL.
  * @param  RxTimeout: Timeout for receiving the response.
  * @param  ...: Variable arguments for expected responses.
  * @retval Response index if found, error code otherwise.
  */
int ATC_SendWaitReceive(ATC_HandleTypeDef *hAtc, const char *pCommand, uint32_t TxTimeout, char *pResp, uint32_t RxTimeout, ...)
{
  int answer = ATC_RESP_NOT_FOUND;
  int count = 0;
  va_list args;

  va_start(args, RxTimeout);
  const char *arg;
  while ((arg = va_arg(args, const char*)) != NULL)
  {
    count++;
  }
  va_end(args);

  hAtc->ppResp = (uint8_t**) ATC_Malloc(count * sizeof(uint8_t*));
  if (hAtc->ppResp == NULL)
  {
    return ATC_RESP_MEM_ERROR;
  }

  va_start(args, RxTimeout);
  for (int i = 0; i < count; i++)
  {
    arg = va_arg(args, const char*);
    hAtc->ppResp[i] = (uint8_t*) ATC_Malloc(strlen(arg) + 1);
    if (hAtc->ppResp[i] == NULL)
    {
      answer = ATC_RESP_MEM_ERROR;
      break;
    }
    strcpy((char*) hAtc->ppResp[i], arg);
  }
  va_end(args);

  do
  {
    if (answer != ATC_RESP_NOT_FOUND)
    {
      break;
    }
    ATC_RxFlush(hAtc);
    if (ATC_TxRaw(hAtc, (const uint8_t*)pCommand, strlen((char*)pCommand)) == false)
    {
      answer = ATC_RESP_SENDING_ERROR;
      break;
    }
    if (ATC_TxWait(hAtc, TxTimeout) == false)
    {
      answer = ATC_RESP_SENDING_ERROR;
      break;
    }

  } while (0);

  if ((count > 0) && (answer == ATC_RESP_NOT_FOUND))
  {
    uint32_t start_time = HAL_GetTick();
    hAtc->RespCount = count;
    while (HAL_GetTick() - start_time < RxTimeout)
    {
      ATC_Delay(1);
      uint8_t found_index = ATC_CheckResponse(hAtc, pResp);
      if (found_index > 0)
      {
        answer = found_index;
        break;
      }
    }
    hAtc->RespCount = 0;
    for (uint8_t i = 0; i < count; i++)
    {
      ATC_Free(hAtc->ppResp[i]);
    }
  }
  ATC_Free(hAtc->ppResp);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Callback for handling UART idle line detection.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  Len: Length of received data.
  * @retval None.
  */
inline void ATC_IdleLineCallback(ATC_HandleTypeDef *hAtc, uint16_t Len)
{
  if (Len > hAtc->Size - hAtc->RxIndex)
  {
    Len = hAtc->Size - hAtc->RxIndex;
  }
  memcpy(&hAtc->pReadBuff[hAtc->RxIndex], hAtc->pRxBuff, Len);
  hAtc->RxIndex += Len;
  if (HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size) == HAL_OK)
  {
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
  }
  else
  {
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    HAL_UART_AbortReceive(hAtc->hUart);
    HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size);
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
    dprintf("[%s] ERROR ENABLE RX DMA\r\n", hAtc->Name);
  }
}

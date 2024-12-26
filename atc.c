
/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "atc.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

#if ATC_DEBUG == ATC_DEBUG_ENABLE
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
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
void              ATC_Free(void** pMem);
void              ATC_RxFlush(ATC_HandleTypeDef* hAtc);
bool              ATC_TxRaw(ATC_HandleTypeDef* hAtc, const uint8_t* pData, uint16_t Len);
bool              ATC_TxBusy(ATC_HandleTypeDef* hAtc);
bool              ATC_TxWait(ATC_HandleTypeDef* hAtc, uint32_t Timeout);
void              ATC_CheckEvents(ATC_HandleTypeDef* hAtc);
uint8_t           ATC_CheckResponse(ATC_HandleTypeDef* hAtc,char** ppFound);
void              ATC_CheckErrors(ATC_HandleTypeDef* hAtc);
void              ATC_TempCallback(const char *str);

/***********************************************************************************************************/

void* ATC_Malloc(size_t size)
{
  void *ptr = NULL;
#if ATC_RTOS == ATC_RTOS_DISABLE
  ptr = malloc(size);
#elif (ATC_RTOS == ATC_RTOS_CMSIS_V1) || (ATC_RTOS == ATC_RTOS_CMSIS_V2)
  ptr = pvPortMalloc(size);
#elif ATC_RTOS == ATC_RTOS_THREADX
  ??
#endif
  return ptr;
}

/***********************************************************************************************************/

void ATC_Free(void** ptr)
{
  if (ptr != NULL && *ptr != NULL)
  {
#if ATC_RTOS == ATC_RTOS_DISABLE
    free(*ptr);
#elif (ATC_RTOS == ATC_RTOS_CMSIS_V1) || (ATC_RTOS == ATC_RTOS_CMSIS_V2)
    vPortFree(*ptr);
#elif ATC_RTOS == ATC_RTOS_THREADX
    ??
#endif
     *ptr = NULL;
  }
}

/***********************************************************************************************************/

void ATC_RxFlush(ATC_HandleTypeDef* hAtc)
{
  hAtc->RxIndex = 0;
  memset(hAtc->pReadBuff, 0, hAtc->Size);
}

/***********************************************************************************************************/

bool ATC_TxRaw(ATC_HandleTypeDef* hAtc, const uint8_t* Data, uint16_t Len)
{
  bool answer = false;
  do
  {
#if ATC_DEBUG == ATC_DEBUG_ENABLE
    dprintf("ATC<%s> - TX: ", hAtc->Name);
    for (uint16_t i = 0 ; i < Len; i++)
    {
      dprintf("%c", Data[i]);
    }
    dprintf("\r\n");
#endif
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

bool ATC_TxBusy(ATC_HandleTypeDef* hAtc)
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

bool ATC_TxWait(ATC_HandleTypeDef* hAtc, uint32_t Timeout)
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
    if ((HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_ERROR) || (HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_TIMEOUT))
    {
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

void ATC_CheckEvents(ATC_HandleTypeDef* hAtc)
{
  if (hAtc->RxIndex > 0)
  {
    for (uint32_t ev = 0; ev < hAtc->Events; ev++)
    {
      char *found = strstr((char*)hAtc->pReadBuff, hAtc->psEvents[ev].Event);
      if (found != NULL)
      {
        hAtc->psEvents[ev].EventCallback(found);
      }
    }
    ATC_RxFlush(hAtc);
  }
}

/***********************************************************************************************************/

uint8_t ATC_CheckResponse(ATC_HandleTypeDef* hAtc, char** ppFound)
{
  uint8_t index = 0;
  if (hAtc->RxIndex > 0)
  {
    for (uint16_t i = 0; i < hAtc->RespCount; i++)
    {
      char *found = strstr((char*)hAtc->pReadBuff, (char*)hAtc->ppResp[i]);
      if (found != NULL)
      {
        if (ppFound != NULL)
        {
          *ppFound = found;
        }
        index = i + 1;
        break;
      }
    }
  }
  return index;
}

/***********************************************************************************************************/

void ATC_CheckErrors(ATC_HandleTypeDef* hAtc)
{
  if (HAL_UART_GetError(hAtc->hUart) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    HAL_UART_AbortReceive(hAtc->hUart);
    HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size);
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
  }
  if (!((HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_BUSY_RX) ||
      (HAL_UART_GetState(hAtc->hUart) == HAL_UART_STATE_BUSY_TX_RX)))
  {
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    HAL_UART_AbortReceive(hAtc->hUart);
    HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size);
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
  }
}

/***********************************************************************************************************/

void ATC_TempCallback(const char *str)
{
  UNUSED(str);
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
bool ATC_Init(ATC_HandleTypeDef* hAtc, UART_HandleTypeDef* hUart, uint16_t BufferSize, const char* pName)
{
  bool answer = false;
  do
  {
    if (hAtc == NULL || hUart == NULL)
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
      dprintf("ATC<%s> - ERROR MALLOC RX BUFF\r\n", hAtc->Name);
      break;
    }
    hAtc->pTxBuff = ATC_Malloc(BufferSize);
    if (hAtc->pTxBuff != NULL)
    {
      memset(hAtc->pTxBuff, 0, BufferSize);
    }
    else
    {
      dprintf("ATC<%s> - ERROR MALLOC TX BUFF\r\n", hAtc->Name);
      break;
    }
    hAtc->pReadBuff = ATC_Malloc(BufferSize);
    if (hAtc->pReadBuff != NULL)
    {
      memset(hAtc->pReadBuff, 0, BufferSize);
    }
    else
    {
      dprintf("ATC<%s> - ERROR MALLOC READ BUFF\r\n", hAtc->Name);
      break;
    }
    hAtc->Size = BufferSize;
    __HAL_UART_CLEAR_FLAG(hAtc->hUart, 0xFFFFFFFF);
    if (HAL_UARTEx_ReceiveToIdle_DMA(hAtc->hUart, hAtc->pRxBuff, hAtc->Size) != HAL_OK)
    {
      dprintf("ATC<%s> - ERROR ENABLE RX DMA\r\n", hAtc->Name);
      break;
    }
    __HAL_DMA_DISABLE_IT(hAtc->hUart->hdmarx, DMA_IT_HT);
    answer = true;

  } while (0);

  if (answer == false)
  {
    if (hAtc->pRxBuff != NULL)
    {
      ATC_Free((void**)&hAtc->pRxBuff);
    }
    if (hAtc->pReadBuff != NULL)
    {
      ATC_Free((void**)&hAtc->pReadBuff);
    }
    memset(hAtc, 0, sizeof(ATC_HandleTypeDef));
  }
  else
  {
    dprintf("ATC<%s> - INIT DONE\r\n", hAtc->Name);
  }
  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  DeInitializes the ATC handle structure.
  * @param  hAtc: Pointer to the ATC handle.
  * @retval None.
  */
void ATC_DeInit(ATC_HandleTypeDef* hAtc)
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
    ATC_Free((void**)&hAtc->pRxBuff);
    ATC_Free((void**)&hAtc->pReadBuff);
    memset(hAtc, 0, sizeof(ATC_HandleTypeDef));

  } while (0);
}

/***********************************************************************************************************/

/**
  * @brief  Sets the ATC event handlers.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  psEvents: Pointer to the event handler structure.
  * @retval true if events are set successfully, false otherwise.
  */
bool ATC_SetEvents(ATC_HandleTypeDef* hAtc, const ATC_EventTypeDef* psEvents)
{
  bool answer = false;
  uint32_t ev = 0;
  do
  {
    if (hAtc == NULL)
    {
      break;
    }
    if (psEvents == NULL)
    {
      break;
    }
    while ((psEvents[ev].Event != NULL) && (psEvents[ev].EventCallback != NULL))
    {
      ev++;
    }
    hAtc->psEvents = (ATC_EventTypeDef*)psEvents;
    hAtc->Events = ev;
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
void ATC_Loop(ATC_HandleTypeDef* hAtc)
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
  * @param  ppResp: Pointer to the response buffer. It Can be NULL.
  * @param  RxTimeout: Timeout for receiving the response.
  * @param  Items: Number of String for Searching
  * @param  ...: Variable arguments for expected responses.
  * @retval Response index if found, error code otherwise.
  */
int ATC_SendReceive(ATC_HandleTypeDef* hAtc, const char* pCommand, uint32_t TxTimeout, char** ppResp, uint32_t RxTimeout, uint8_t Items, ...)
{
  int answer = ATC_RESP_NOT_FOUND;
  if (ATC_TxBusy(hAtc) == true)
  {
    return ATC_RESP_TX_BUSY;
  }
  if (Items > ATC_RESP_MAX)
  {
    return ATC_RESP_ITEMS;
  }
  ATC_CheckErrors(hAtc);
  va_list args;
  va_start(args, Items);
  for (int i = 0; i < Items; i++)
  {
    char *arg = va_arg(args, char*);
    hAtc->ppResp[i] = (uint8_t*) ATC_Malloc(strlen(arg) + 1);
    if (hAtc->ppResp[i] == NULL)
    {
      for (uint8_t j = 0; j < i; j++)
      {
        ATC_Free((void**)&hAtc->ppResp[j]);
      }
      return ATC_RESP_MEM_ERROR;
    }
    strcpy((char*) hAtc->ppResp[i], arg);
    hAtc->ppResp[i][strlen(arg)] = 0;
  }
  va_end(args);

  do
  {
    ATC_RxFlush(hAtc);
    if (ATC_TxRaw(hAtc, (const uint8_t*)pCommand, strlen((char*)pCommand)) == false)
    {
      answer = ATC_RESP_SENDING_ERROR;
      break;
    }
    if (ATC_TxWait(hAtc, TxTimeout) == false)
    {
      answer = ATC_RESP_SENDING_TIMEOUT;
      break;
    }

  } while (0);

  if ((Items > 0) && (answer == ATC_RESP_NOT_FOUND))
  {
    uint32_t start_time = HAL_GetTick();
    hAtc->RespCount = Items;
    while (HAL_GetTick() - start_time < RxTimeout)
    {
      ATC_Delay(1);
      uint8_t found_index = ATC_CheckResponse(hAtc, ppResp);
      if (found_index > 0)
      {
        answer = found_index;
        break;
      }
    }
  }
  hAtc->RespCount = 0;
  for (uint8_t i = 0; i < Items; i++)
  {
    ATC_Free((void**)&hAtc->ppResp[i]);
  }
  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Send a command.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  pCommand: Pointer to the command string, it can use like printf format
  * @param  TxTimeout: Timeout for sending the command.
  * @param  ... , it can use like printf format
  * @retval Response true or false.
  */
bool ATC_Send(ATC_HandleTypeDef *hAtc, const char *pCommand, uint32_t TxTimeout, ...)
{
  bool answer = false;
  do
  {
    if (ATC_TxBusy(hAtc) == true)
    {
      break;
    }
    ATC_CheckErrors(hAtc);
    va_list args;
    va_start(args, TxTimeout);
    int len = vsnprintf((char*)hAtc->pTxBuff, hAtc->Size, pCommand, args);
    va_end(args);
    if ((len < 0) || (len > hAtc->Size))
    {
      break;
    }
    ATC_RxFlush(hAtc);
    if (ATC_TxRaw(hAtc, (const uint8_t*)hAtc->pTxBuff, strlen((char*)hAtc->pTxBuff)) == false)
    {
      break;
    }
    if (ATC_TxWait(hAtc, TxTimeout) == false)
    {
      break;
    }
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  waiting for a response.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  ppResp: Pointer to the response buffer. It Can be NULL.
  * @param  RxTimeout: Timeout for sending the command.
  * @param  Items: Number of searching strings
  * @param  ...: Variable arguments for expected responses.
  * @retval Response index if found, error code otherwise.
  */
int ATC_Receive(ATC_HandleTypeDef *hAtc, char **ppResp, uint32_t RxTimeout, uint8_t Items, ...)
{
  int answer = ATC_RESP_NOT_FOUND;
  if (Items > ATC_RESP_MAX)
  {
    return ATC_RESP_ITEMS;
  }
  ATC_CheckErrors(hAtc);
  va_list args;
  va_start(args, Items);
  for (int i = 0; i < Items; i++)
  {
    char *arg = va_arg(args, char*);
    hAtc->ppResp[i] = (uint8_t*) ATC_Malloc(strlen(arg) + 1);
    if (hAtc->ppResp[i] == NULL)
    {
      for (uint8_t j = 0; j < i; j++)
      {
        ATC_Free((void**)&hAtc->ppResp[j]);
      }
      return ATC_RESP_MEM_ERROR;
    }
    strcpy((char*) hAtc->ppResp[i], arg);
    hAtc->ppResp[i][strlen(arg)] = 0;
  }
  va_end(args);

  if (Items > 0)
  {
    uint32_t start_time = HAL_GetTick();
    hAtc->RespCount = Items;
    while (HAL_GetTick() - start_time < RxTimeout)
    {
      ATC_Delay(1);
      uint8_t found_index = ATC_CheckResponse(hAtc, ppResp);
      if (found_index > 0)
      {
        answer = found_index;
        break;
      }
    }
  }
  hAtc->RespCount = 0;
  for (uint8_t i = 0; i < Items; i++)
  {
    ATC_Free((void**)&hAtc->ppResp[i]);
  }
  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Callback for handling UART idle line detection.
  * @param  hAtc: Pointer to the ATC handle.
  * @param  Len: Length of received data.
  * @retval None.
  */
inline void ATC_IdleLineCallback(ATC_HandleTypeDef* hAtc, uint16_t Len)
{
  if (Len > hAtc->Size - hAtc->RxIndex)
  {
    Len = hAtc->Size - hAtc->RxIndex;
  }
  memcpy(&hAtc->pReadBuff[hAtc->RxIndex], hAtc->pRxBuff, Len);
#if ATC_DEBUG == ATC_DEBUG_ENABLE
  dprintf("ATC<%s> - RX: ", hAtc->Name);
  for (int i = 0; i < Len; i++)
  {
    dprintf("%c", hAtc->pRxBuff[i]);
  }
  dprintf("\r\n");
#endif
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
  }
}

/***********************************************************************************************************/

/**
  * @brief  Delay function.
  * @param  Delay: delay in milisecond..
  * @retval None.
  */
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

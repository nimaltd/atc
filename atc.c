#include "atc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if (_ATC_DEBUG == 1)
#define	atc_printf(...)     printf(__VA_ARGS__)
#else
#define	atc_printf(...)     {};
#endif
//####################################################################################################
void* atc_alloc(size_t size)
{
#if (_ATC_RTOS == 0)
  return malloc(size);
#else
  return pvPortMalloc(size);
#endif
}
//####################################################################################################
void atc_free(void *ptr)
{
  if (ptr != NULL)
#if (_ATC_RTOS == 0)
    free(ptr);
#else
    vPortFree(ptr);
#endif
}
//####################################################################################################
void atc_init(atc_t *atc, const char *name, USART_TypeDef *USARTx, void (*found)(char*))
{
  if (atc->inited == true)
    return;
  memset(atc, 0, sizeof(atc_t));
  strncpy(atc->name, name, sizeof(atc->name) - 1);
  atc->usart = USARTx;
  atc->found = found;
  LL_USART_EnableIT_RXNE(atc->usart);
  atc->inited = true;
  atc_printf("\r\n[%s] inited.\r\n", atc->name);
}
//####################################################################################################
bool atc_lock(atc_t *atc, uint32_t wait_ms)
{
  if (atc->lock == false)
  {
    atc->lock = true;
    return true;
  }
  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < wait_ms)
  {
    atc_delay(1);
    if (atc->lock == 0)
    {
      atc->lock = true;
      return true;
    }
  }
  return false;
}
//####################################################################################################
void atc_unlock(atc_t *atc)
{
  atc->lock = false;
}
//####################################################################################################
void atc_transmit(atc_t *atc, uint8_t *data, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    while (!LL_USART_IsActiveFlag_TXE(atc->usart))
      atc_delay(1);
    LL_USART_TransmitData8(atc->usart, data[i]);
  }
  while (!LL_USART_IsActiveFlag_TC(atc->usart))
    atc_delay(1);
}
//####################################################################################################
void atc_rxCallback(atc_t *atc)
{
  if (LL_USART_IsActiveFlag_RXNE(atc->usart))
  {
    uint8_t tmp = LL_USART_ReceiveData8(atc->usart);
    if (atc->rxIndex < _ATC_RXSIZE - 1)
    {
      atc->rxBuffer[atc->rxIndex] = tmp;
      atc->rxIndex++;
    }
    atc->rxTime = HAL_GetTick();
    return;
  }
//  if (LL_USART_IsActiveFlag_PE(atc->usart))
//    LL_USART_ClearFlag_PE(atc->usart);
//  if (LL_USART_IsActiveFlag_FE(atc->usart))
//    LL_USART_ClearFlag_FE(atc->usart);
//  if (LL_USART_IsActiveFlag_ORE(atc->usart))
//    LL_USART_ClearFlag_ORE(atc->usart);
//  if (LL_USART_IsActiveFlag_NE(atc->usart))
//    LL_USART_ClearFlag_NE(atc->usart);
}
//####################################################################################################
void atc_search(atc_t *atc)
{
  for (uint8_t search = 0; search < _ATC_SEARCH_MAX; search++)
  {
    if (atc->search[search] == NULL)
      break;
    char *str = strstr((char*) atc->rxBuffer, atc->search[search]);
    if (str != NULL)
    {
      if (atc->found != NULL)
        atc->found(str);
    }
  }
}
//####################################################################################################
char* atc_searchAnswer(atc_t *atc, uint8_t items, uint8_t *foundIndex)
{
  *foundIndex = 0;
  if (items >= _ATC_SEARCH_CMD_MAX)
    items = _ATC_SEARCH_CMD_MAX;
  for (uint8_t search = 0; search < items; search++)
  {
    if (atc->searchCmd[search] == NULL)
      break;
    char *str = strstr((char*) atc->rxBuffer, atc->searchCmd[search]);
    if (str != NULL)
    {
      *foundIndex = search + 1;
      return str;
    }
  }
  return NULL;
}
//####################################################################################################
void atc_empty(atc_t *atc)
{
  memset(atc->rxBuffer, 0, _ATC_RXSIZE);
  atc->rxIndex = 0;
}
//####################################################################################################
bool atc_available(atc_t *atc)
{
  if ((atc->rxIndex > 0) && (HAL_GetTick() - atc->rxTime) > _ATC_RXTIMEOUT_MS)
    return true;
  return false;
}
//####################################################################################################
bool atc_addSearch(atc_t *atc, const char *str)
{
  if (atc->searchIndex == _ATC_SEARCH_MAX - 1)
    return false;
  atc->search[atc->searchIndex] = (char*) atc_alloc(strlen(str) + 1);
  if (atc->search[atc->searchIndex] != NULL)
  {
    strncpy(atc->search[atc->searchIndex], str, strlen(str));
    atc->search[atc->searchIndex][strlen(str)] = 0;
    atc->searchIndex++;
    return true;
  }
  else
    return false;
}
//####################################################################################################
int8_t atc_command(atc_t *atc, const char *command, uint32_t timeout_ms, char *answer, uint16_t answer_size,
    int items, ...)
{
  if (atc->inited == false)
    return -1;
  if (atc_lock(atc, timeout_ms) == false)
    return -1;
  if (answer != NULL)
    memset(answer, 0, answer_size);
  uint8_t foundIndex = 0;
  va_list tag;
  va_start(tag, items);
  for (uint8_t i = 0; i < items; i++)
  {
    char *str = va_arg(tag, char*);
    atc->searchCmd[i] = (char*) atc_alloc(strlen(str) + 1);
    if (atc->searchCmd[i] != NULL)
    {
      strcpy(atc->searchCmd[i], str);
      atc->searchCmd[i][strlen(str)] = 0;
    }
    if (items >= _ATC_SEARCH_CMD_MAX)
      break;
  }
  va_end(tag);
  atc_transmit(atc, (uint8_t*) command, strlen(command));
  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < timeout_ms)
  {
    atc_delay(1);
    if (atc_available(atc))
    {
      atc_printf("[%s] %s", atc->name, (char* )atc->rxBuffer);
      atc_search(atc);
      char *found = atc_searchAnswer(atc, items, &foundIndex);
      if (found != NULL && answer != NULL)
        strncpy(answer, found, answer_size);
      atc_empty(atc);
      if (found != NULL)
        break;
    }
  }
  for (uint8_t i = 0; i < items; i++)
    atc_free(atc->searchCmd[i]);
  atc_unlock(atc);
  return foundIndex;
}
//####################################################################################################
void atc_loop(atc_t *atc)
{
  if (atc->inited == false)
  {
#if (_ATC_RTOS > 0)
    atc_delay(1);
#endif
    return;
  }
  if (HAL_GetTick() - atc->loopTime < _ATC_RXTIMEOUT_MS)
  {
#if (_ATC_RTOS > 0)
    atc_delay(1);
#endif
    return;
  }
  atc->loopTime = HAL_GetTick();
  if (atc_lock(atc, 10) == false)
    return;
  if (atc_available(atc))
  {
    atc_printf("[%s] %s", atc->name, (char* )atc->rxBuffer);
    atc_search(atc);
    atc_empty(atc);
  }
  atc_unlock(atc);
}
//####################################################################################################

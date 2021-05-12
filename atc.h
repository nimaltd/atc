#ifndef _ATC_H_
#define _ATC_H_

/*
 *	Author:     Nima Askari
 *	WebSite:    https://www.github.com/NimaLTD
 *	Instagram:  https://www.instagram.com/github.NimaLTD
 *	LinkedIn:   https://www.linkedin.com/in/NimaLTD
 *	Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
 */

/*
 * Version:	3.0.3
 *
 * History:
 *
 * (3.0.3): Remove warning.
 * (3.0.2):	Clear answer buffer before use.
 * (3.0.1):	Change some defines.
 * (3.0.0):	Rewrite again. Support NONE-RTOS, RTOS V1 and RTOS V2.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdarg.h>
#include "main.h"
#include "atcConfig.h"

#if (_ATC_RTOS == 0)
#define atc_delay(x)  HAL_Delay(x)
#else
#include "cmsis_os.h"
#define atc_delay(x)  osDelay(x)
#endif

typedef struct
{
  bool inited;
  uint8_t rxBuffer[_ATC_RXSIZE];
  uint16_t rxIndex;
  uint32_t rxTime;
  uint32_t loopTime;
  char *search[_ATC_SEARCH_MAX];
  char *searchCmd[_ATC_SEARCH_CMD_MAX];
  uint8_t searchIndex;
  char name[8];

  bool lock;
  USART_TypeDef *usart;
  void (*found)(char *foundStr);

} atc_t;

//###############################################################################################################
/*
 *  atc: atc struct
 *  name: name of atc
 *  USARTx: selected USART
 *  found: atc found function. auto called after found strings you added before. do not use atc_command function into this
 */
void atc_init(atc_t *atc, const char *name, USART_TypeDef *USARTx, void (*found)(char*));
//###############################################################################################################
/*
 * put in usart rx interrupt
 * atc: atc struct
 */
void atc_rxCallback(atc_t *atc);
//###############################################################################################################
/*
 * if rtos is disabled, put in infinite loop. if rtos is enabled , do not need.
 * atc: atc struct
 */
void atc_loop(atc_t *atc);
//###############################################################################################################
/*
 * add a string to always search strings
 * atc:	atc struct
 * str: add always search string
 * return
 * false: out of memory
 * true: succeed
 */
bool atc_addSearch(atc_t *atc, const char *str);
//###############################################################################################################
/*
 * transmit data
 * atc:	atc struct
 * data: send pointer
 * len: data length
 */
void atc_transmit(atc_t *atc, uint8_t *data, uint16_t len);
//###############################################################################################################
/*
 * send at-command function
 * atc:	atc struct
 * command:	send at-command
 * timeout_ms: timeout for get answer
 * answer: answer pointer. NULL if do not use
 * answer_size: maximum answer size
 * items: number of answer items
 * ...: string of answer
 * 	return:
 * 	-1:	lock
 * 	0:	not found
 * 	>0:	found answer index
 */
int8_t atc_command(atc_t *atc, const char *command, uint32_t timeout_ms, char *answer, uint16_t answer_size,
    int items, ...);
//###############################################################################################################

#ifdef __cplusplus
}
#endif

#endif /* _ATC_H_ */

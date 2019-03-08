#ifndef _ATC_H
#define _ATC_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "ATCConfig.h"
#include "usart.h"
#include "cmsis_os.h"

//###################################################################################
typedef struct
{
	uint8_t		*RxData;
	uint8_t		RxTmp;
	uint16_t	RxIndex;
	uint16_t	RxSize;
	uint32_t	RxTime;
	uint8_t		Timeout;
	uint8_t		RxBusy;
	
}ATC_Buff_t;
//########
typedef struct
{
	uint8_t               ID;
	uint8_t               Busy;
	char                  Name[16];
	UART_HandleTypeDef    uart;
	ATC_Buff_t            Buff;	
	char                  Answer[_ATC_MAX_SEARCH_PARAMETER_FOR_AT_ANSWER][32];
	uint8_t               AnswerFound;
	uint8_t               AutoSearchIndex;
	char                  *AutoSearchString[_ATC_MAX_AUTO_SEARCH_STRING];
  
}ATC_t;
//########


//###################################################################################

//        add to Uart interrupt after HAL_UART_IRQHandler(&huartx);
void      ATC_RxCallBack(ATC_t *atc);

//        osPriority effect only for call first time
bool      ATC_Init(ATC_t *atc,char	*Name,UART_HandleTypeDef SelectUart,uint16_t	RxSize,uint8_t	Timeout_Package,osPriority Priority);

//        tranmit string
void      ATC_TransmitString(ATC_t *atc,char *Buff);

//        send AtCommand and wait for answer. return 0 timeout,return	>0 parameter number found   
uint8_t   ATC_Send(ATC_t *atc,char *AtCommand,uint32_t Wait_ms,uint8_t	ArgCount,...);

//        Add Always search strings
uint16_t  ATC_AddAutoSearchString(ATC_t *atc,char *String);

//        when found a string, call this function automatically. in "ATCUser.c"
void      ATC_User_AutoSearchCallBack(ATC_t *atc,uint16_t	FoundIndex,char *FoundString,char *ATC_rxDataPointer);

//###################################################################################

#endif

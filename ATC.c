
#include "ATC.h"
#include "ATCConfig.h"

int8_t        ATC_ID=-1;
ATC_t         *ATC[_ATC_MAX_DEVICE];

osThreadId    ATCBuffTaskHandle;
void          StartATCBuffTask(void const *argument);
//###################################################################################
void  ATC_RxCallBack(ATC_t *atc)
{
  if((atc->uart.ErrorCode==0) && (atc->Buff.RxBusy==0))
  {
    atc->Buff.RxTime=HAL_GetTick();
    if(atc->Buff.RxIndex < atc->Buff.RxSize-1)
    {
      atc->Buff.RxData[atc->Buff.RxIndex]=atc->Buff.RxTmp;
      atc->Buff.RxIndex++;
    }    
  }
  HAL_UART_Receive_IT(&huart2,&atc->Buff.RxTmp,1);
}
//###################################################################################
void  ATC_TransmitString(ATC_t *atc,char *Buff)
{
  if(atc->RS485_Ctrl_Pin != 0)
  {
    HAL_GPIO_WritePin(atc->RS485_Ctrl_GPIO,atc->RS485_Ctrl_Pin,GPIO_PIN_SET);
    osDelay(1);  
  }
  if(atc->uart.hdmatx!=NULL)
  {
    while(atc->uart.hdmatx->State != HAL_DMA_STATE_READY)
      osDelay(1);    
    HAL_UART_Transmit_DMA(&atc->uart,(uint8_t*)Buff,strlen(Buff));
    while(atc->uart.hdmatx->State != HAL_DMA_STATE_READY)
      osDelay(1);    
  }
  else
  {
    HAL_UART_Transmit(&atc->uart,(uint8_t*)Buff,strlen(Buff),100);    
  }
  if(atc->RS485_Ctrl_Pin != 0)
  {
    HAL_GPIO_WritePin(atc->RS485_Ctrl_GPIO,atc->RS485_Ctrl_Pin,GPIO_PIN_RESET);
    osDelay(1);  
  }
}
//###################################################################################
uint8_t  ATC_Send(ATC_t *atc,char *AtCommand,uint32_t Wait_ms,uint8_t  ArgCount,...)
{
  while(atc->Busy)
    osDelay(1);
  atc->Busy=1;
  atc->AnswerFound=0;
  va_list tag;
  va_start (tag,ArgCount);
  char *arg[_ATC_MAX_SEARCH_PARAMETER_FOR_AT_ANSWER];
  memset(arg,0,sizeof(arg));
  for(uint8_t i=0; i<ArgCount ; i++)
  {
    arg[i] = va_arg (tag, char*);  
  }
  for(uint8_t i=0; i<ArgCount ; i++)
    strncpy(atc->Answer[i],arg[i],sizeof(atc->Answer[i]));
  va_end (tag);
  #if(_ATC_DEBUG==1)
  printf("[%s] Send: %s\r\n",atc->Name,AtCommand);
  #endif
  ATC_TransmitString(atc,AtCommand);
  uint32_t  StartTime=HAL_GetTick();
  while(HAL_GetTick()-StartTime < Wait_ms)
  {
    if(atc->AnswerFound>0)
    {
      memset(atc->Answer,0,sizeof(atc->Answer));
      #if(_ATC_DEBUG==1)
      printf("[%s] Answer: Found (%d) after %d ms\r\n",atc->Name,atc->AnswerFound,HAL_GetTick()-StartTime);
      #endif
      atc->Busy=0;
      return atc->AnswerFound;      
    }
    osDelay(1);  
  }
  memset(atc->Answer,0,sizeof(atc->Answer));
  #if(_ATC_DEBUG==1)
  printf("[%s] Timeout\r\n",atc->Name);
  #endif
  atc->Busy=0;
  return 0;
}
//###################################################################################
uint16_t  ATC_AddAutoSearchString(ATC_t *atc,char *String)
{
  if(String==NULL)
    return 0;
  if(atc->AutoSearchIndex == _ATC_MAX_AUTO_SEARCH_STRING-1)
    return 0;
  atc->AutoSearchString[atc->AutoSearchIndex] = calloc(strlen(String)+1,1);
  if(atc->AutoSearchString[atc->AutoSearchIndex]==NULL)
    return 0;
  strcpy(atc->AutoSearchString[atc->AutoSearchIndex],String);
  atc->AutoSearchIndex++;  
  return atc->AutoSearchIndex;
}
//###################################################################################
void  ATC_AutoSearch(ATC_t *atc)
{
  for(uint16_t  idx=0 ; idx<atc->AutoSearchIndex ; idx++)
  {
    char *str = strstr((char*)atc->Buff.RxData,atc->AutoSearchString[idx]);
    if(str!=NULL)
    {
      ATC_User_AutoSearchCallBack(atc,idx,atc->AutoSearchString[idx],str);
    }    
  }  
}
//###################################################################################
bool  ATC_Init(ATC_t *atc,char  *Name,UART_HandleTypeDef SelectUart,uint16_t  RxSize,uint8_t  Timeout_Package,osPriority Priority)
{
  memset(atc,0,sizeof(&atc));
  atc->uart = SelectUart;
  strncpy(atc->Name,Name,sizeof(atc->Name));
  atc->Buff.RxData = calloc(RxSize,1);
  atc->Buff.RxSize=RxSize;
  atc->Buff.Timeout=Timeout_Package;    
  if(atc->Buff.RxData==NULL)
  {
    #if(_ATC_DEBUG==1)
    printf("[%s] Init Faild, Could not create struct!\r\n",atc->Name);
    #endif
    return false;
  }
  else  
  {
    HAL_UART_Receive_IT(&huart2,&atc->Buff.RxTmp,1);
    if(ATC_ID>=_ATC_MAX_DEVICE)
    {
      #if(_ATC_DEBUG==1)
      printf("[%s] Init Faild, Max Device is %d!\r\n",atc->Name,_ATC_MAX_DEVICE);
      #endif
      memset(atc,0,sizeof(&atc));
      return false;
    }    
    ATC_ID++;
    ATC[ATC_ID] = atc;
    if(ATC_ID==0)
    {
      osThreadDef(ATCBuffTask, StartATCBuffTask, Priority, 0, 256);
      ATCBuffTaskHandle = osThreadCreate(osThread(ATCBuffTask), NULL);
      if(ATCBuffTaskHandle==NULL)
      {
        #if(_ATC_DEBUG==1)
        printf("[%s] Init Faild, Could not create task!\r\n",atc->Name);
        #endif
        memset(atc,0,sizeof(&atc));
        return false;
      }    
    }
    #if(_ATC_DEBUG==1)
    printf("[%s] Init Done\r\n",atc->Name);
    #endif
    return true;
  }
}
//###################################################################################
void ATC_InitRS485(ATC_t *atc,GPIO_TypeDef *RS485_GPIO,uint16_t RS485_PIN)
{
  atc->RS485_Ctrl_GPIO = RS485_GPIO;
  atc->RS485_Ctrl_Pin = RS485_PIN;  
  HAL_GPIO_WritePin(atc->RS485_Ctrl_GPIO,atc->RS485_Ctrl_Pin,GPIO_PIN_RESET);
  #if(_ATC_DEBUG==1)
  printf("[%s] Init ATC RS485 Done\r\n",atc->Name);
  #endif
}
//###################################################################################
void StartATCBuffTask(void const *argument)
{    
  while(1)
  {
    for(uint8_t MX=0 ; MX<_ATC_MAX_DEVICE ; MX++)
    {
      if(ATC[MX] != NULL)
      {
        HAL_UART_Receive_IT(&ATC[MX]->uart,&ATC[MX]->Buff.RxTmp,1);        
        if((ATC[MX]->Buff.RxIndex>0) && (HAL_GetTick()-ATC[MX]->Buff.RxTime>ATC[MX]->Buff.Timeout))
        {
          ATC[MX]->Buff.RxBusy=1;
          //++++++  Search in atcommands answer
          for(uint8_t answ=0 ; answ<_ATC_MAX_SEARCH_PARAMETER_FOR_AT_ANSWER ; answ++)
          {
            if(ATC[MX]->Answer[answ][0]!=0)
            {
              if(strstr((char*)ATC[MX]->Buff.RxData,ATC[MX]->Answer[answ])!=NULL)
              {
                ATC[MX]->AnswerFound=answ+1;
                break;
              }              
            }            
          }          
          //------   Search in atcommands answer
          //++++++  Auto Search String
          ATC_AutoSearch(ATC[MX]);
          //------  Auto Search String
          #if (_ATC_DEBUG==2)
          printf("[%s]\r\n%s\r\n",ATC[MX]->Name,(char*)ATC[MX]->Buff.RxData);
          #endif
          memset(ATC[MX]->Buff.RxData,0,ATC[MX]->Buff.RxSize);
          ATC[MX]->Buff.RxIndex=0;
          ATC[MX]->Buff.RxBusy=0;
        }
      }    
    }
    osDelay(10);
  }
}
//###################################################################################

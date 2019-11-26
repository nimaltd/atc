
#include "ATC.h"
#include "ATCConfig.h"

int8_t        ATC_ID=-1;
ATC_t         ATC[_ATC_MAX_DEVICE];

osThreadId    ATCBuffTaskHandle;
void          StartATCBuffTask(void const *argument);

//###################################################################################
void  ATC_RxCallBack(uint8_t  ID)
{
  if(ID>=_ATC_MAX_DEVICE)
    return;
  if((ATC[ID].uart->ErrorCode==0) && (ATC[ID].Buff.RxBusy==0))
  {
    ATC[ID].Buff.RxTime=HAL_GetTick();
    if(ATC[ID].Buff.RxIndex < ATC[ID].Buff.RxSize-1)
    {
      ATC[ID].Buff.RxData[ATC[ID].Buff.RxIndex]=ATC[ID].Buff.RxTmp;
      ATC[ID].Buff.RxIndex++;
    }    
  }
  HAL_UART_Receive_IT(ATC[ID].uart,&ATC[ID].Buff.RxTmp,1);
}
//###################################################################################
void  ATC_TransmitString(uint8_t  ID,char *Buff)
{
  if(ID>=_ATC_MAX_DEVICE)
    return;
  if(ATC[ID].RS485_Ctrl_Pin != 0)
  {
    HAL_GPIO_WritePin(ATC[ID].RS485_Ctrl_GPIO,ATC[ID].RS485_Ctrl_Pin,GPIO_PIN_SET);
    osDelay(1);  
  }
  HAL_UART_Receive_IT(ATC[ID].uart,&ATC[ID].Buff.RxTmp,1);
  if(ATC[0].uart->hdmatx!=NULL)
  {
    while(ATC[0].uart->hdmatx->State != HAL_DMA_STATE_READY)
      osDelay(1);    
    HAL_UART_Transmit_DMA(ATC[0].uart,(uint8_t*)Buff,strlen(Buff));
    while(ATC[0].uart->hdmatx->State != HAL_DMA_STATE_READY)
      osDelay(1);    
  }
  else
  {
    HAL_UART_Transmit(ATC[0].uart,(uint8_t*)Buff,strlen(Buff),100);    
  }
  if(ATC[ID].RS485_Ctrl_Pin != 0)
  {
    HAL_GPIO_WritePin(ATC[ID].RS485_Ctrl_GPIO,ATC[ID].RS485_Ctrl_Pin,GPIO_PIN_RESET);
    osDelay(1);  
  }
}
//###################################################################################
uint8_t  ATC_Send(uint8_t  ID,char *AtCommand,uint32_t Wait_ms,uint8_t  ArgCount,...)
{
  if(ID>=_ATC_MAX_DEVICE)
    return 0;
  while(ATC[ID].Busy)
    osDelay(1);
  ATC[ID].Busy=1;
  ATC[ID].AnswerFound=0;
  va_list tag;
  va_start (tag,ArgCount);
  char *arg[_ATC_MAX_SEARCH_PARAMETER_FOR_AT_ANSWER];
  memset(arg,0,sizeof(arg));
  for(uint8_t i=0; i<ArgCount ; i++)
  {
    arg[i] = va_arg (tag, char*);  
  }
  for(uint8_t i=0; i<ArgCount ; i++)
    strncpy(ATC[ID].Answer[i],arg[i],sizeof(ATC[ID].Answer[i]));
  va_end (tag);
  #if(_ATC_DEBUG==1)
  printf("[%s] Send: %s\r\n",ATC[ID].Name,AtCommand);
  #endif
  ATC_TransmitString(ID,AtCommand);
  uint32_t  StartTime=HAL_GetTick();
  while(HAL_GetTick()-StartTime < Wait_ms)
  {
    if(ATC[ID].AnswerFound>0)
    {
      #if(_ATC_DEBUG==1)
      printf("[%s] Answer: Found (%d) after %d ms\r\n",ATC[ID].Name,ATC[ID].AnswerFound,HAL_GetTick()-StartTime);
      #endif
      memset(ATC[ID].Answer,0,sizeof(ATC[ID].Answer));
      ATC[ID].Busy=0;
      return ATC[ID].AnswerFound;      
    }
    osDelay(1);  
  }
  memset(ATC[ID].Answer,0,sizeof(ATC[ID].Answer));
  #if(_ATC_DEBUG==1)
  printf("[%s] Timeout\r\n",ATC[ID].Name);
  #endif
  ATC[ID].Busy=0;
  return 0;
}
//###################################################################################
char *     ATC_GetAnswer(uint8_t ID)
{
  return (char*)ATC[ID].Buff.RxDataBackup;  
}
//###################################################################################
uint16_t  ATC_AddAutoSearchString(uint8_t  ID,char *String)
{
  if(ID>=_ATC_MAX_DEVICE)
    return 0;
  if(String==NULL)
    return 0;
  if(ATC[ID].AutoSearchIndex == _ATC_MAX_AUTO_SEARCH_STRING-1)
    return 0;
  ATC[ID].AutoSearchString[ATC[ID].AutoSearchIndex] = calloc(strlen(String)+1,1);
  if(ATC[ID].AutoSearchString[ATC[ID].AutoSearchIndex]==NULL)
    return 0;
  strcpy(ATC[ID].AutoSearchString[ATC[ID].AutoSearchIndex],String);
  ATC[ID].AutoSearchIndex++;  
  return ATC[ID].AutoSearchIndex;
}
//###################################################################################
void  ATC_AutoSearch(uint8_t  ID)
{
  if(ID>=_ATC_MAX_DEVICE)
    return;
  for(uint16_t  idx=0 ; idx<ATC[ID].AutoSearchIndex ; idx++)
  {
    char *str = strstr((char*)ATC[ID].Buff.RxData,ATC[ID].AutoSearchString[idx]);
    if(str!=NULL)
    {
      #if(_ATC_DEBUG==1)
      printf("[%s] Found Auto index: %d: String:%s\r\n",ATC[ID].Name,idx,ATC[ID].AutoSearchString[idx]);
      #endif
      ATC_User_AutoSearchCallBack(ID,idx,ATC[ID].AutoSearchString[idx],str);
    }    
  }  
}
//###################################################################################
bool  ATC_Init(uint8_t  ID,char  *Name,UART_HandleTypeDef *SelectUart,uint16_t  RxSize,uint8_t  Timeout_Package,osPriority Priority)
{
  if(ID>=_ATC_MAX_DEVICE)
  {
    #if(_ATC_DEBUG==1)
    printf("[%s] Init Faild, Max Device is %d!\r\n",ATC[ID].Name,_ATC_MAX_DEVICE);
    #endif
    return false;
  }
  memset(&ATC[ID],0,sizeof(ATC[ID]));
  ATC[ID].uart = SelectUart;
  strncpy(ATC[ID].Name,Name,sizeof(ATC[ID].Name));
  ATC[ID].Buff.RxData = pvPortMalloc(RxSize);
  ATC[ID].Buff.RxDataBackup = pvPortMalloc(RxSize);
  ATC[ID].Buff.RxSize=RxSize;
  ATC[ID].Buff.Timeout=Timeout_Package;    
  if((ATC[ID].Buff.RxData==NULL) || (ATC[ID].Buff.RxDataBackup==NULL))
  {
    #if(_ATC_DEBUG==1)
    printf("[%s] Init Faild, Could not create struct!\r\n",ATC[ID].Name);
    #endif
    return false;
  }
  else  
  {
    HAL_UART_Receive_IT(ATC[ID].uart,&ATC[ID].Buff.RxTmp,1);
    ATC_ID++;
    if(ATC_ID==0)
    {
      osThreadDef(ATCBuffTask, StartATCBuffTask, Priority, 0, 256);
      ATCBuffTaskHandle = osThreadCreate(osThread(ATCBuffTask), NULL);
      if(ATCBuffTaskHandle==NULL)
      {
        #if(_ATC_DEBUG==1)
        printf("[%s] Init Faild, Could not create task!\r\n",ATC[ID].Name);
        #endif
        memset(&ATC[ID],0,sizeof(ATC[ID]));
        return false;
      }    
    }
    #if(_ATC_DEBUG==1)
    printf("[%s] Init Done\r\n",ATC[ID].Name);
    #endif
    return true;
  }
}
//###################################################################################
void ATC_InitRS485(uint8_t  ID,GPIO_TypeDef *RS485_GPIO,uint16_t RS485_PIN)
{
  if(ID>=_ATC_MAX_DEVICE)
    return;
  ATC[ID].RS485_Ctrl_GPIO = RS485_GPIO;
  ATC[ID].RS485_Ctrl_Pin = RS485_PIN;  
  HAL_GPIO_WritePin(ATC[ID].RS485_Ctrl_GPIO,ATC[ID].RS485_Ctrl_Pin,GPIO_PIN_RESET);
  #if(_ATC_DEBUG==1)
  printf("[%s] Init ATC RS485 Done\r\n",ATC[ID].Name);
  #endif
}
//###################################################################################
void StartATCBuffTask(void const *argument)
{    
  while(1)
  {
    for(uint8_t MX=0 ; MX<_ATC_MAX_DEVICE ; MX++)
    {
      if(&ATC[MX] != NULL)
      {      
        if((ATC[MX].Buff.RxIndex>0) && ((HAL_GetTick()-ATC[MX].Buff.RxTime)>ATC[MX].Buff.Timeout))
        {
          ATC[MX].Buff.RxBusy=1;
          //++++++  Search in atcommands answer
          for(uint8_t answ=0 ; answ<_ATC_MAX_SEARCH_PARAMETER_FOR_AT_ANSWER ; answ++)
          {
            if(ATC[MX].Answer[answ][0]!=0)
            {
              if(strstr((char*)ATC[MX].Buff.RxData,ATC[MX].Answer[answ])!=NULL)
              {
                ATC[MX].AnswerFound=answ+1;
                memset(ATC[MX].Buff.RxDataBackup,0,ATC[MX].Buff.RxSize);
                strcpy((char*)ATC[MX].Buff.RxDataBackup,(char*)ATC[MX].Buff.RxData);
                break;
              }              
            }            
          }          
          //------   Search in atcommands answer
          //++++++  Auto Search String
          ATC_AutoSearch(MX);
          //------  Auto Search String
          #if (_ATC_DEBUG==2)
          printf("[%s]\r\n%s\r\n",ATC[MX].Name,(char*)ATC[MX].Buff.RxData);
          #endif
          memset(ATC[MX].Buff.RxData,0,ATC[MX].Buff.RxSize);
          ATC[MX].Buff.RxIndex=0;
          ATC[MX].Buff.RxBusy=0;
        }
      }    
    }
    osDelay(10);
  }
}
//###################################################################################

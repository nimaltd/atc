# ATC  AT Command
<br />
I use Stm32f407vg and Keil Compiler and Stm32CubeMX wizard.
 <br />
Please Do This ...
<br />
<br />
1) Enable FreeRTOS.  
<br />
2) Config your usart and enable RX interrupt (TX DMA is optional) on CubeMX.
<br />
3) Select "General peripheral Initalizion as a pair of '.c/.h' file per peripheral" on project settings.
<br />
4) Config your ATCConfig.h file.
<br />
5) Add  ATC_RxCallBack(YourID) on usart interrupt routin. 
<br />
7) call  ATC_Init(YourID,"name",huart2,512,10,osPriorityLow) on your task.
<br />

example :
```
#include "ATC.h"
.
.
.
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	
	ATC_Init(0,"Bluetooth",huart2,512,10,osPriorityLow);
	ATC_AddAutoSearchString(0,"Always search this string1");
	ATC_AddAutoSearchString(0,"Always search this string2");
	ATC_AddAutoSearchString(0,"Always search this string3");
	osDelay(3000);
	if(ATC_Send(0,"AT\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n") == 1)
	{
	   // find "\r\nOK\r\n	
	}
  	for(;;)
  	{
		osDelay(3000);
	
	  }
  /* USER CODE END StartDefaultTask */
}


```

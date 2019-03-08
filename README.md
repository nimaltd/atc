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
5) Add  ATC_RxCallBack(ATC_t *atc) on usart interrupt routin. 
<br />
7) call  ATC_Init(&ATC_handle,"name",huart2,512,10,osPriorityLow) on your task.
<br />

example :
```
#include "ATC.h"
ATC_t	ATC_Bluetooth;
.
.
.
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	
	ATC_Init(&ATC_Bluetooth,"Bluetooth",huart2,512,10,osPriorityLow);
	ATC_AddAutoSearchString(&ATC_Bluetooth,"Always search this string1");
	ATC_AddAutoSearchString(&ATC_Bluetooth,"Always search this string2");
	ATC_AddAutoSearchString(&ATC_Bluetooth,"Always search this string3");
	osDelay(3000);
	ATC_Send(&ATC_Bluetooth,"AT\r\n",1000,2,"OK\r\n","ERROR\r\n");
  for(;;)
  {
	osDelay(3000);
	
  }
  /* USER CODE END StartDefaultTask */
}


```

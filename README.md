# ATC  AT Command

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
	ATC_Send(&ATC_Bluetooth,"AT\r\n",1000,"OK\r\n");
  for(;;)
  {
	osDelay(3000);
	
  }
  /* USER CODE END StartDefaultTask */
}


```

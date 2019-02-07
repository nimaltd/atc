
#include "ATC.h"
#include "Bluetooth.h"


void		ATC_User_AutoSearchCallBack(ATC_t *atc,uint16_t	FoundIndex,char *FoundString,char *ATC_rxDataPointer)
{
	if(strcmp(FoundString,"+BTCONNECT:")==0)
	{
		printf("Conected......................\r\n");
		Bluetooth.ConnectionStatus=1;
		Bluetooth.ConnectionStatusChanged=1;	
	}
	else if(strcmp(FoundString,"+BTDISCONN:")==0)
	{
		printf("DisConected......................\r\n");
		Bluetooth.ConnectionStatus=0;
		Bluetooth.ConnectionStatusChanged=1;
	}
	else if(strcmp(FoundString,"+BTCONNECTING:")==0)
	{
		Bluetooth.ConnectingNewProfile=1;
	}
}



#ifndef _ATCCONFIG_H_
#define _ATCCONFIG_H_

#define	_ATC_DEBUG            0       //  use printf debug
#define	_ATC_RTOS             1       //  0: no rtos    1: cmsis_os v1    2: cmsis_os v2
#define	_ATC_RXSIZE           1500    //  at-command rx buffer size
#define	_ATC_SEARCH_CMD_MAX   5       //  maximum of answer in at-command
#define	_ATC_SEARCH_MAX       10      //  maximum	of always search in buffer
#define	_ATC_RXTIMEOUT_MS     50      //  rx timeout to get new packet

#endif /* _ATCCONFIG_H_ */

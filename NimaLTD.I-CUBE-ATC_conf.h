/**
  ******************************************************************************
  * File Name          : NimaLTD.I-CUBE-ATC_conf.h
  * Description        : This file provides code for the configuration
  *                      of the NimaLTD.I-CUBE-ATC_conf.h instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _NIMALTD_I_CUBE_ATC_CONF_H_
#define _NIMALTD_I_CUBE_ATC_CONF_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define ATC_DEBUG_DISABLE                    0
#define ATC_DEBUG_ENABLE                     1

#define ATC_RTOS_DISABLE                     0
#define ATC_RTOS_CMSIS_V1                    1
#define ATC_RTOS_CMSIS_V2                    2
#define ATC_RTOS_THREADX                     3

/**
	MiddleWare name : NimaLTD.I-CUBE-ATC.4.0.0
	MiddleWare fileName : NimaLTD.I-CUBE-ATC_conf.h
*/
/*---------- ATC_DEBUG  -----------*/
#define ATC_DEBUG      ATC_DEBUG_ENABLE

/*---------- ATC_RTOS  -----------*/
#define ATC_RTOS      ATC_RTOS_DISABLE

#ifdef __cplusplus
}
#endif
#endif /* _NIMALTD_I_CUBE_ATC_CONF_H_ */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


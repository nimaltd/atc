## at-command library 
 *	Author:     Nima Askari
 *	WebSite:    https://www.github.com/NimaLTD
 *	Instagram:  https://www.instagram.com/github.NimaLTD
 *	LinkedIn:   https://www.linkedin.com/in/NimaLTD
 *	Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw 
--------------------------------------------------------------------------------
* [x] NONE RTOS Supported.
* [x] RTOS V1 Supported.
* [x] RTOS V2 Supported.
--------------------------------------------------------------------------------
* [ ] F0 tested.
* [ ] L0 tested.
* [x] F1 tested.
* [ ] L1 tested.
* [ ] F2 tested.
* [ ] F3 tested.
* [ ] F4 tested.
* [ ] L4 tested.
* [ ] F7 tested.
* [ ] H7 tested.
--------------------------------------------------------------------------------   
* Enable USART (LL Library) and RX interrupt.
* Add library to your project.
* Configure `atcCongig.h` file.
* Create a struct as global. ex: atc_t atc; . 
* Select `General peripheral Initalizion as a pair of '.c/.h' file per peripheral` on project settings.
* Config `gsmConfig.h`.
* Call `gsm_init(osPriotary_low)` in your task. 


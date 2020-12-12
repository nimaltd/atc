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
* Create a struct as global.
* Create found callback function if you need it.
* Call `atc_init()`.
* Call `atc_loop()` in infinit loop.
```
#include "atc.h"
atc_t  atc;

void  atc_found(char *foundStr)
{

}

int main()
{
  atc_init(&atc, "MY_ATC", USART1, atc_found);
  while (1)
  {
    atc_loop(&atc);
  }  
}
```



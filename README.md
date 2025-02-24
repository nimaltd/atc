# AT Command Library for STM32  
---  
## Please Do not Forget to get STAR, DONATE and support me on social networks. Thank you. :sparkling_heart:  
---   
- **Author**: Nima Askari  
- **Github**: [https://www.github.com/NimaLTD](https://www.github.com/NimaLTD)  
- **Youtube**: [https://www.youtube.com/@nimaltd](https://www.youtube.com/@nimaltd)  
- **LinkedIn**: [https://www.linkedin.com/in/nimaltd](https://www.linkedin.com/in/nimaltd)  
- **Instagram**: [https://instagram.com/github.NimaLTD](https://instagram.com/github.NimaLTD)  
---
## Overview
The AT Command Library (ATC) simplifies UART communication for STM32 microcontrollers with an event-driven approach and debug capabilities. This updated version introduces AT command slave functionality, allowing the STM32 to process commands like `AT+LED=ON` and respond (e.g., `+OK`), making it ideal for IoT and embedded applications.

### Key Features
- Event-driven UART handling with DMA support.
- Optional RTOS compatibility (FreeRTOS, ThreadX).
- Debug logging when enabled.
- AT command slave mode with custom handlers.

---

## Installation
1. Download the library: [NimaLTD.I-CUBE-ATC.pdsc](https://github.com/nimaltd/STM32-PACK/raw/main/ATC/NimaLTD.I-CUBE-ATC.pdsc).
2. Import it into STM32CubeMX and enable it.
3. Configure UART:
   - Enable UART interrupt.
   - Enable TX/RX DMA in Normal Mode.
4. In the Code Generator tab, select "Generate peripheral initialization as a pair of .c/.h files per peripheral."
5. Generate the project code.

---

## Getting Started
1. Declare an `ATC_HandleTypeDef` structure in your code.
2. Add `ATC_IdleLineCallback()` to the UART idle line callback (e.g., in `HAL_UARTEx_RxEventCallback`).
3. Initialize the library with `ATC_Init()`.
4. Optionally, set up events with `ATC_SetEvents()` or AT commands with `ATC_SetCommands()`.
5. Call `ATC_Loop()` in your main loop to process incoming data.

---

## Using AT Command Handlers
This library now supports AT command handling, enabling the STM32 to act as a command slave. Send commands like `AT+LED=ON` from an external device, and the STM32 will execute them and respond (e.g., `+OK` or `+LED:ON`).

### Setup Steps
1. **Define Command Handlers**: Create functions to process commands and generate responses.
2. **Create a Command Table**: Use `ATC_CmdTypeDef` to map command prefixes (e.g., `AT+LED=`) to handlers.
3. **Register Commands**: Call `ATC_SetCommands()` to link the table to your ATC handle.
4. **Run the Loop**: Use `ATC_Loop()` to handle incoming commands and events.

### Example: LED Control
Control an LED and query its state via AT commands:

```c
#include "atc.h"
#include "stm32f1xx_hal.h" // Adjust for your STM32 series

// ATC handle
ATC_HandleTypeDef hAtc;

// Command handler for setting LED state
void Handle_LED(const char* args, char* response) {
  if (strcmp(args, "ON") == 0) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // Adjust port/pin
    strcpy(response, "+OK");
  } else if (strcmp(args, "OFF") == 0) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    strcpy(response, "+OK");
  } else {
    strcpy(response, "+ERROR");
  }
}

// Command handler for querying LED state
void Handle_GetLED(const char* args, char* response) {
  GPIO_PinState led_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
  strcpy(response, (led_state == GPIO_PIN_SET) ? "+LED:ON" : "+LED:OFF");
}

// Command table
ATC_CmdTypeDef at_commands[] = {
  {"AT+LED?", Handle_GetLED},  // Query LED state: "AT+LED?" -> "+LED:ON"
  {"AT+LED=", Handle_LED},     // Set LED state: "AT+LED=ON" -> "+OK"
  {NULL, NULL}                 // Terminator
};

// UART handle (from STM32CubeMX)
extern UART_HandleTypeDef huart1;

int main(void) {
  // HAL initialization (from STM32CubeMX)
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();

  // Initialize ATC
  ATC_Init(&hAtc, &huart1, 256, "Slave1");

  // Register AT commands
  ATC_SetCommands(&hAtc, at_commands);

  while (1) {
    ATC_Loop(&hAtc); // Process commands and events
  }
}

// Add to your UART IRQ handler (e.g., stm32f1xx_it.c)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance == USART1) { // Adjust for your UART instance
    ATC_IdleLineCallback(&hAtc, Size);
  }
}
```

## Command Responses
- `AT+LED=ON` → `+OK` (turns LED on)
- `AT+LED=OFF` → `+OK` (turns LED off)
- `AT+LED?` → `+LED:ON` or `+LED:OFF` (queries state)
- Unknown command → `+ERROR`

---

# Watch the Video:

<div align="center">
  <a href="https://www.youtube.com/watch?v=Wz_oWqmAEo8"><img src="https://img.youtube.com/vi/Wz_oWqmAEo8/0.jpg" alt="Video"></a>
</div>

---
The old Version: https://github.com/nimaltd/ATC/archive/refs/tags/3.0.3.zip 



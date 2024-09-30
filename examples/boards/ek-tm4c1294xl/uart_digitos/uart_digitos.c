#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>  // Incluye stdlib.h para atoi

//*****************************************************************************
// System clock rate in Hz.
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
// The error routine that is called if the driver library encounters an error.
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
// The UART interrupt handler.
//*****************************************************************************
#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
uint32_t bufferIndex = 0;
char receivedChar;
uint32_t contador = 0;

void UARTIntHandler(void) {
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ui32Status);

    // Reiniciar el índice del buffer
    bufferIndex = 0;
    memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    contador = 0;
    while (UARTCharsAvail(UART0_BASE)) {
        receivedChar = UARTCharGetNonBlocking(UART0_BASE);
        
        // Solo almacenar si hay espacio en el buffer
        if (bufferIndex < BUFFER_SIZE - 1) {
            buffer[bufferIndex++] = receivedChar; // Almacena el carácter
            buffer[bufferIndex] = '\0'; // Asegúrate de que el string esté terminado
            
            // Si se recibe un salto de línea, convertir a entero
            if (isdigit(receivedChar)) {
                UARTCharPutNonBlocking(UART0_BASE, contador);
                // Convertir la cadena a entero usando atoi
                contador = contador * 10 + (receivedChar - '0'); 
            }
        }
        
        UARTCharPutNonBlocking(UART0_BASE, receivedChar);
    }
}

//*****************************************************************************
// Send a string to the UART.
//*****************************************************************************
void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count) {
    // Loop while there are more characters to send.
    while(ui32Count--) {
        // Write the next character to the UART.
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

//*****************************************************************************
// This example demonstrates how to send a string of data to the UART.
//*****************************************************************************
int main(void) {
    // Run from the PLL at 120 MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);
                                             
    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Enable the GPIO pins for the LED (PN0).
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    // Enable the peripherals used by this example.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable processor interrupts.
    IntMasterEnable();

    // Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    // Prompt for text to be entered.
    UARTSend((uint8_t *)"\033[2JEnter text: ", 16);

    // Loop forever echoing data through the UART.
    while(1) {
        // Control del LED según el valor del contador
        if (contador == 123) {
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0); // Encender LED
        } else {
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0); // Apagar LED
        }
    }
}

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
#include <stdlib.h>

///*****************************************************************************
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





uint32_t tiempo = 1;
uint32_t g_ui32Flags;

//*****************************************************************************
//
// The interrupt handler for the first timer interrupt.
//
//*****************************************************************************
void Timer0IntHandler(void)
{
    char cOne, cTwo;

    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    HWREGBITW(&g_ui32Flags, 0) ^= 1;
    // Encender o apagar los LEDs según el valor del contador usando desplazamiento de bits
    if(contador > 20) {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0 ); // Bit 2
    }
    else if (contador <= 20 && contador > 15)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0 ); // Bit 2
    }
    else if (contador <= 15)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1 ); // Bit 2
    }
    //
    // Update the interrupt status.
    //
    IntMasterDisable();
    cOne = HWREGBITW(&g_ui32Flags, 0) ? '1' : '0';
    cTwo = HWREGBITW(&g_ui32Flags, 1) ? '1' : '0';
    // Agrega un debug para ver el estado de las variables
    UARTprintf("cOne: %d, cTwo: %d, time: %d, contador: %d\n", cOne, cTwo, tiempo, contador);


    TimerLoadSet(TIMER0_BASE, TIMER_A, (uint32_t)(g_ui32SysClock * tiempo));
    IntMasterEnable();
}

//*****************************************************************************
//
// The interrupt handler for the second timer interrupt.
//
//*****************************************************************************
void Timer1IntHandler(void)
{
    char cOne, cTwo;
    //
    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    HWREGBITW(&g_ui32Flags, 1) ^= 1;
    //
    // Toggle the flag for the second timer.
    //

    if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0)
         {
             tiempo++;
         }
    //
    if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0)
         {
             tiempo--;
             if (tiempo<=0){tiempo=1;}
         }
    // Update the interrupt status.
    // 
    IntMasterDisable();
    cOne = HWREGBITW(&g_ui32Flags, 0) ? '1' : '0';
    cTwo = HWREGBITW(&g_ui32Flags, 1) ? '1' : '0';
    // Agrega un debug para ver el estado de las variables
    UARTprintf("cOne: %d, cTwo: %d\n", cOne, cTwo);
    IntMasterEnable();
}




//////////////////////////////////////
//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}
void interrupcio_UART(void){
    // Habilitar interrupciones UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}
void boton(void){
    // Habilitar el periférico GPIOJ
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    // Configurar PJ0 y PJ1 como entradas con resistencias pull-up
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
void user_led_N(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1|GPIO_PIN_0);
}
void user_led_F(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
}
//*****************************************************************************
//
// This example application demonstrates the use of the timers to generate
// periodic interrupts.
//
//*****************************************************************************
int main(void)
{
    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    //
    //
    // Enable processor interrupts.
    //
    IntMasterEnable();
    // Initialize the UART and write status.
    //
    ConfigureUART();
    interrupcio_UART();
    boton();
    user_led_F();
    user_led_N();
    
    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    

    //
    // Configure the two 32-bit periodic timers.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock *0.2);
    TimerLoadSet(TIMER1_BASE, TIMER_A, g_ui32SysClock *0.2);

    //
    // Setup the interrupts for the timer timeouts.
    //
    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Enable the timers.
    //
    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);

    //
    // Loop forever while the timers run.
    //
    while(1)
    {
    }
}

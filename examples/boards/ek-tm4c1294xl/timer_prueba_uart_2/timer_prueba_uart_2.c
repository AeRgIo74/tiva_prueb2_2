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


// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Flags that contain the current value of the interrupt indicator as displayed
// on the UART.
//
//*****************************************************************************

uint32_t tiempo = 1;
uint32_t contador = 0;
uint32_t g_ui32Flags;
char receivedData[7] = {0};  // Para almacenar la palabra "buzzer"
static uint8_t index = 0;  // Índice para almacenar los caracteres recibidos

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The interrupt handler for the first timer interrupt.
//
//*****************************************************************************
void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while (ui32Count--)
    {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}
void Timer0IntHandler(void)
{
    char cOne, cTwo;

    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    HWREGBITW(&g_ui32Flags, 0) ^= 1;
    // Encender o apagar los LEDs según el valor del contador usando desplazamiento de bits
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, (contador & 0x1) ? GPIO_PIN_1 : 0); // Bit 0
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, (contador & 0x2) ? GPIO_PIN_0 : 0); // Bit 1
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, (contador & 0x4) ? GPIO_PIN_4 : 0); // Bit 2
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, (contador & 0x8) ? GPIO_PIN_0 : 0); // Bit 3

    contador++;
    if(contador > 15) {
        contador = 0;
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

//////////////////////////
void UARTIntHandler(void)
{
    uint32_t ui32Status;

    // Obtener el estado de la interrupción de UART
    ui32Status = UARTIntStatus(UART0_BASE, true);
    // Limpiar la interrupción
    UARTIntClear(UART0_BASE, ui32Status);

    // Mientras haya datos disponibles en UART
    while (UARTCharsAvail(UART0_BASE))
    {
        char receivedChar = UARTCharGetNonBlocking(UART0_BASE);  // Leer carácter recibido

        // Ignorar caracteres de nueva línea y retorno de carro
        if (receivedChar == '\n' || receivedChar == '\r')
        {
            continue;
        }

        // Guardar el carácter recibido en el buffer
        if (index < sizeof(receivedData) - 1)  // Dejar espacio para el terminador nulo
        {
            receivedData[index] = receivedChar;
            index++;
            receivedData[index] = '\0';  // Añadir terminador nulo
        }

        // Manejar caracteres numéricos
        if (isdigit(receivedChar))
        {
            tiempo = receivedChar - '0';  // Convertir carácter a entero
            index = 0;  // Reiniciar el índice para la siguiente entrada
            memset(receivedData, 0, sizeof(receivedData));  // Limpiar el buffer
        }
        
        // Si se ha recibido la palabra "buzzer"
        if (index == 6 && strncmp(receivedData, "buzzer", 6) == 0)
        {
            tiempo = 1;  // Reiniciar tiempo a 1
            index = 0;  // Reiniciar el índice para recibir la siguiente palabra
            memset(receivedData, 0, sizeof(receivedData));  // Limpiar el buffer
        }
        // Opcional: Imprimir el buffer para depuración
        UARTprintf("Recibido: %s\n", receivedData);
    }
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
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Configure the two 32-bit periodic timers.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock);
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

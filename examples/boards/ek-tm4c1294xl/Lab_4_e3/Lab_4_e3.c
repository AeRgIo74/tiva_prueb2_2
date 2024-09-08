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

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Counter for the timer load value.
//
//*****************************************************************************
uint32_t ui32TimerSeconds = 1;  // Inicialmente 1 segundo

//*****************************************************************************
//
// The interrupt handler for the first timer interrupt.
//
//*****************************************************************************
uint32_t contador = 0;

void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    contador++;
    
    if(contador > 15) {
        contador = 0;
    }

    // Encender o apagar los LEDs según el valor del contador usando desplazamiento de bits
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, (contador & 0x1) ? GPIO_PIN_1 : 0); // Bit 0
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, (contador & 0x2) ? GPIO_PIN_0 : 0); // Bit 1
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, (contador & 0x4) ? GPIO_PIN_4 : 0); // Bit 2
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, (contador & 0x8) ? GPIO_PIN_0 : 0); // Bit 3
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
    // Enable the GPIO port that is used for the on-board LEDs.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable the GPIO pins for the LEDs (PN0 & PN1).
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    
    // Enable processor interrupts.
    IntMasterEnable();

    //
    // Configure the 32-bit periodic timer.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    
    // Configurar el temporizador inicialmente para 1 segundo
    TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock);

    //
    // Setup the interrupts for the timer timeouts.
    //
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Enable the timers.
    //
    TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Loop forever while the timers run.
    //
    while(1)
    {
    }
}

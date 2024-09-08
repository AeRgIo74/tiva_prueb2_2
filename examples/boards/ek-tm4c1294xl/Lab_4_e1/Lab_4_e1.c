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
void Timer0IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Leer el estado actual del pin PN1.
    uint8_t ui8LEDState = GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1);

    // Alternar el estado del LED (si está encendido, apagarlo; si está apagado, encenderlo).
    if(ui8LEDState)
    {
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);  // Apagar LED
        if (ui32TimerSeconds < 5)
        {
            ui32TimerSeconds = ui32TimerSeconds + 1;  // Aumentar el contador
        }
        else
        {
            ui32TimerSeconds = 1;  // Reiniciar el contador a 1 segundo
        }
    }
    else
    {
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);  // Encender LED
    }

    // Incrementar el tiempo de carga hasta un máximo de 5 segundos

    // Reconfigurar el tiempo de carga del temporizador con el nuevo valor
    TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock * ui32TimerSeconds);
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

    //
    // Enable the GPIO pins for the LEDs (PN0 & PN1).
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Configure the 32-bit periodic timer.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    
    // Configurar el temporizador inicialmente para 1 segundo
    TimerLoadSet(TIMER0_BASE, TIMER_A, g_ui32SysClock * ui32TimerSeconds);

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

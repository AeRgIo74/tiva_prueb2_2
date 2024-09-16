#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Blinky (blinky)</h1>
//!
//! A very simple example that blinks the on-board LED using direct register
//! access.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1);
}
#endif

//*****************************************************************************
//
// Blink the on-board LED.
//
//*****************************************************************************
int
main(void)
{
    volatile uint32_t ui32Loop;

    //
    // Enable the GPIO port that is used for the on-board LED.
    //ON para los puertos N y OF para los puertos F
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1 | GPIO_PIN_0);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
        //
        // Delay for a bit.
        //
        for(ui32Loop = 0; ui32Loop < 1000000; ui32Loop++)
        {
        }

        //
        // Turn off the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for a bit.
        //
        for(ui32Loop = 0; ui32Loop < 1000000; ui32Loop++)
        {
        }
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
        for(ui32Loop = 0; ui32Loop < 1000000; ui32Loop++)
        {
        }
    }
}

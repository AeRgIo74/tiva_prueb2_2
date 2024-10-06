#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
// The variable g_ui32SysClock contains the system clock frequency in Hz.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

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
// Configure the UART and its pins. This must be called before UARTprintf.
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

//*****************************************************************************
//
// Configure PWM for a 25% duty cycle signal running at 250Hz.
//
//*****************************************************************************
void ConfigurePWM(void)
{
    uint32_t ui32PWMClockRate;

    //
    // The PWM peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // Enable the GPIO port that is used for the PWM output.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Configure the PWM function for this pin.
    //
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);

    //
    // Set the PWM clock to be SysClk / 8.
    //
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_8);

    //
    // Use a local variable to store the PWM clock rate which will be
    // 120 MHz / 8 = 15 MHz. This variable will be used to set the
    // PWM generator period.
    //
    ui32PWMClockRate = g_ui32SysClock / 8;

    //
    // Configure PWM2 to count up/down without synchronization.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, (ui32PWMClockRate / 250));

    //
    // Set initial duty cycle to 0%.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);

    //
    // Enable PWM Out Bit 2 (PF2) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}

//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int main(void)
{
    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    //
    // Initialize the UART.
    //
    ConfigureUART();

    //
    // Display the setup on the console.
    //
    UARTprintf("PWM ->\n");
    UARTprintf("  Module: PWM2\n");
    UARTprintf("  Pin: PF2\n");
    UARTprintf("  Initial Duty Cycle: 0%%\n");
    UARTprintf("  Increasing Duty Cycle from 0%% to 100%%\n\n");
    UARTprintf("Generating PWM on PWM2 (PF2) -> State = ");

    //
    // Configure the PWM.
    //
    ConfigurePWM();

    //
    // Loop forever while the PWM signals are generated.
    //
    while (1)
    {
        // Increase duty cycle from 0% to 100%
        for (uint32_t dutyCycle = 0; dutyCycle <= 100; dutyCycle++)
        {
            // Set the PWM pulse width based on the duty cycle.
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle) / 100);
            
            // Print out the current duty cycle.
            UARTprintf("PWM Output Duty Cycle: %d%%\n", dutyCycle);

            // Delay for a short period to see the change in duty cycle
            SysCtlDelay((g_ui32SysClock / 3)*0.1); // Adjust delay as needed
        }

        // Decrease duty cycle from 100% to 0%
        for (uint32_t dutyCycle = 100; dutyCycle > 0; dutyCycle--)
        {
            // Set the PWM pulse width based on the duty cycle.
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle) / 100);

            // Print out the current duty cycle.
            UARTprintf("PWM Output Duty Cycle: %d%%\n", dutyCycle);

            // Delay for a short period to see the change in duty cycle
            SysCtlDelay((g_ui32SysClock / 3)*0.1); // Adjust delay as needed
        }
    }
}

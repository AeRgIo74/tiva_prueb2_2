


#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include <string.h>

uint32_t g_ui32SysClock;
volatile bool activateBuzzer = false; // Variable para activar el zumbador

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

void
UARTIntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ui32Status);

    while(UARTCharsAvail(UART0_BASE))
    {
        char receivedChar = UARTCharGetNonBlocking(UART0_BASE);
        UARTCharPutNonBlocking(UART0_BASE, receivedChar);
        
        // Verificar si el mensaje recibido es "BUZZER"
        if (receivedChar == 'B') // Comienza a buscar "BUZZER"
        {
            char buffer[6];
            buffer[0] = receivedChar;
            for (int i = 1; i < 6; i++)
            {
                buffer[i] = UARTCharGetNonBlocking(UART0_BASE);
            }
            buffer[6] = '\0'; // Terminar la cadena

            if (strcmp(buffer, "UZZER") == 0)
            {
                activateBuzzer = true; // Activar el zumbador
            }
        }
    }
}

void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

int
main(void)
{
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
                                             
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0); // LED en PN0

    // Habilitar el puerto C para el zumbador
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6); // Configurar PC6 como salida para el zumbador

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    IntMasterEnable();

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    UARTSend((uint8_t *)"¿Cómo estás???\r\n", 18);

    // Habilitar el puerto J para los botones
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    // Configurar PJ0 y PJ1 como entradas con resistencias pull-up
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    while(1)
    {
        // Si el mensaje "BUZZER" fue recibido, activar el zumbador por 2 segundos
        if (activateBuzzer)
        {
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6); // Encender el zumbador
            SysCtlDelay(g_ui32SysClock / 3); // 2 segundos de retardo
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0); // Apagar el zumbador
            activateBuzzer = false; // Resetear la bandera
        }

        // Si el botón en PJ0 está presionado (estado bajo), encender el zumbador manualmente
        if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0)
        {
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6); // Encender el zumbador
        }
        else
        {
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0); // Apagar el zumbador
        }

        // Si el botón en PJ1 está presionado (estado bajo), enviar "motor2"
        if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0)
        {
            UARTSend((uint8_t *)"motor2\r\n", 7);
            SysCtlDelay(g_ui32SysClock / 10);  // Retardo para evitar múltiples lecturas
        }
    }
}

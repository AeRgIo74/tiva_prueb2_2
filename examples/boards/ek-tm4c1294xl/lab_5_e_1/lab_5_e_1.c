#include <stdint.h>
#include <stdbool.h>
#include <string.h>
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

uint32_t g_ui32SysClock;
char receivedData[7] = {0};  // Para almacenar la palabra "buzzer"
int contador = 0;
static uint8_t index = 0;  // Índice para almacenar los caracteres recibidos

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

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

        UARTCharPutNonBlocking(UART0_BASE, receivedChar);  // Eco del carácter recibido

        // Guardar el carácter recibido en el buffer
        if (index < 6)  // Asegurarse de no superar el tamaño del buffer
        {
            receivedData[index] = receivedChar;
            index++;
        }

        // Si se ha recibido la palabra "buzzer"
        if (index == 6 && strncmp(receivedData, "buzzer", 6) == 0)
        {
            contador = 1;
            index = 0;  // Reiniciar el índice para recibir la siguiente palabra
            memset(receivedData, 0, sizeof(receivedData));  // Limpiar el buffer
        }

        // Reiniciar el índice si se sobrepasa el tamaño del buffer
        if (index >= 6)
        {
            index = 0;
            memset(receivedData, 0, sizeof(receivedData));  // Limpiar el buffer
        }
    }
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while (ui32Count--)
    {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

int main(void)
{
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);

    // Habilitar el puerto C para el buzzer (PC6)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);  // Asegurarse de que el buzzer esté apagado inicialmente

    // Habilitar el puerto N para el LED (PN0)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    // Habilitar UART0 y el puerto A para UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    IntMasterEnable();

    // Configurar los pines para UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configurar UART para 115200 baudios, 8 bits de datos, 1 bit de parada y sin paridad
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Habilitar interrupciones UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    // Enviar mensaje inicial a través de UART
    UARTSend((uint8_t *)"¿Cómo estás???\r\n", 18);

    // Habilitar el puerto J para los botones
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    // Configurar PJ0 y PJ1 como entradas con resistencias pull-up
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    while (1)
    {
        if (contador == 1)
        {
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);  // Encender el buzzer
            SysCtlDelay((g_ui32SysClock / 3) * 5);  // Esperar 5 segundos
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);  // Apagar el buzzer
            contador = 0;
        }

        // Si el botón en PJ0 está presionado (estado bajo), enviar "motor1"
        if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0)
        {
            UARTSend((uint8_t *)"motor1\r\n", 7);
            SysCtlDelay(g_ui32SysClock / 10);  // Retardo para evitar múltiples lecturas
        }

        // Si el botón en PJ1 está presionado (estado bajo), enviar "motor2"
        if (GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0)
        {
            UARTSend((uint8_t *)"motor2\r\n", 7);
            SysCtlDelay(g_ui32SysClock / 10);  // Retardo para evitar múltiples lecturas
        }
    }
}

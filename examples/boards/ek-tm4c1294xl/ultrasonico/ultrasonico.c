#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"

uint32_t ui32SysClock;
uint32_t distance;
uint32_t start_time, end_time;
volatile bool echo_received = false;


// ISR para capturar los eventos de la señal Echo
void EchoIntHandler(void)
{
    // Limpiar la interrupción
    GPIOIntClear(GPIO_PORTB_BASE, GPIO_PIN_4);

    if (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_4))
    {
        // Señal de Echo sube: inicio del pulso
        TimerEnable(TIMER0_BASE, TIMER_A);
        start_time = TimerValueGet(TIMER0_BASE, TIMER_A);
    }
    else
    {
        // Señal de Echo baja: fin del pulso
        end_time = TimerValueGet(TIMER0_BASE, TIMER_A);
        echo_received = true;
        TimerDisable(TIMER0_BASE, TIMER_A);
    }
}

/// configuracion de pines
void config_ultrasonic(void){
    // Configurar Trigger y Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);  // Trigger pin
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);   // Echo pin

    // Configurar interrupciones para el pin Echo
    GPIOIntRegister(GPIO_PORTB_BASE, EchoIntHandler);
    GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_BOTH_EDGES);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_4);

    // Configurar Timer0 para medir el tiempo del Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT_UP);
}
void distancia(void){
    // Generar pulso de Trigger
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5);
    SysCtlDelay(ui32SysClock / (1000000 * 3));  // Delay 10 us
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);

        // Esperar hasta que se reciba la señal Echo
    while (!echo_received);

        // Calcular la distancia en cm
    uint32_t time_diff = end_time - start_time;
    distance = ((time_diff/120) * 0.034) / 2;  // Usar la fórmula de la velocidad del sonido
    echo_received = false;
    return distance;
}
int main(void)
{
    
    
    // Configurar el sistema a 120 MHz
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                       SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL |
                                       SYSCTL_CFG_VCO_240), 120000000);

    // Configurar UART para mostrar los datos en la consola
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, ui32SysClock);

    config_ultrasonic();

    while (1)
    {
        
        distancia();
        // Imprimir la distancia
        UARTprintf("Distance: %u cm\n", distance);

        // Delay para la próxima medición
        SysCtlDelay(ui32SysClock / 3);  // Delay de 1 segundo
    }
}

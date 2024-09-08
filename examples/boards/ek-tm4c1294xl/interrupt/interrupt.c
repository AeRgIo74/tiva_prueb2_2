#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"

// Manejadores de interrupciones vacíos
void IntGPIOa(void) { }
void IntGPIOb(void) { }
void IntGPIOc(void) { }

// Prototipo del manejador de interrupción
void GPIOJIntHandler(void);

int main(void)
{
    // Habilita los puertos N y F
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Espera a que los periféricos estén listos
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}

    // Configura los pines PN0, PN1 y PF4 como salida (LEDs)
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);

    // Configura PJ0 como entrada para el botón
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);

    // Habilita la resistencia pull-up en PJ0 para evitar fluctuaciones
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configura interrupción para el pin PJ0 (detectar flanco de bajada)
    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);

    // Limpia y habilita la interrupción en PJ0
    GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);
    GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_0);

    // Habilita las interrupciones globalmente
    IntEnable(INT_GPIOJ);

    // Bucle principal vacío
    while(1)
    {
        // No hay ninguna acción en el bucle principal, todo está controlado por la interrupción
    }
}

// Manejador de interrupción para el puerto J
void GPIOJIntHandler(void)
{
    // Limpia la interrupción en PJ0
    GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);

    // Enciende los LEDs en respuesta a la interrupción
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1); // PN1 ON
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0); // PN0 ON
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4); // PF4 ON
}

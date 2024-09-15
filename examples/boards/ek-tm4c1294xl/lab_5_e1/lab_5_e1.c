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

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
    while(1);
}
#endif

int contador = 0;

void Button1IntHandler(void) {
    GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0); // Limpiar la interrupción
    contador++;
    if (contador >= 15) {
        contador = 15; 
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0); // Indicador botón 1
}

void Button2IntHandler(void) {
    GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_1); // Limpiar la interrupción
    contador--;
    if (contador < 0) {
        contador = 0; // Limitar a 0
    }
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4); // Indicador botón 2
}

void GPIOJIntHandler(void) {
    if (GPIOIntStatus(GPIO_PORTJ_BASE, true) & GPIO_PIN_1) {
        GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_1); // Limpiar la interrupción
        Button1IntHandler(); // Llama al manejador del botón 1
    }
    if (GPIOIntStatus(GPIO_PORTJ_BASE, true) & GPIO_PIN_0) {
        GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0); // Limpiar la interrupción
        Button2IntHandler(); // Llama al manejador del botón 2
    }
}

void decimalToBinaryArray(int num, int binaryArray[], int *size) {
    *size = 0; // Inicializar el tamaño del array
    while (num > 0) {
        binaryArray[(*size)++] = num % 2; // Almacena el bit en el array
        num /= 2; // Divide el número por 2
    }
    while (*size < 4) {
        binaryArray[(*size)++] = 0; // Rellenar con ceros
    }
    for (int i = 0; i < *size / 2; i++) {
        int temp = binaryArray[i];
        binaryArray[i] = binaryArray[*size - 1 - i];
        binaryArray[*size - 1 - i] = temp;
    }
}

int main(void) {
    // Habilitar los periféricos para los puertos N, F y J
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    // Asegurarse de que los periféricos estén listos
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)) {}

    // Configurar como salida (LED)
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1 | GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0);

    // Configurar PJ como entrada (interruptores)
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_1);
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);

    // Habilitar resistencia pull-up en PJ1 y PJ0
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configurar interrupciones para los botones
    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE); // Interruptor 1
    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE); // Interruptor 2

    // Habilitar las interrupciones para los botones
    GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_1 | GPIO_PIN_0);

    // Habilitar las interrupciones globales
    IntMasterEnable();

    // Asociar los manejadores de interrupción
    IntRegister(INT_GPIOJ, GPIOJIntHandler); // Registra el manejador de interrupción para PJ

    while(1) {
        int binaryArray[4]; // Array para almacenar hasta 4 bits
        int size;

        // Actualizar el array binario después de cambiar el contador
        decimalToBinaryArray(contador, binaryArray, &size);

        // Encender o apagar los LEDs según el array binario
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, (binaryArray[0] == 1) ? GPIO_PIN_1 : 0);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, (binaryArray[1] == 1) ? GPIO_PIN_0 : 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, (binaryArray[2] == 1) ? GPIO_PIN_4 : 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, (binaryArray[3] == 1) ? GPIO_PIN_0 : 0);

        SysCtlDelay(200000); // Esperar un momento para permitir el cambio de estado visible
    }
}

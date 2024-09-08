#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include <stdio.h>


#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
    while(1);
}
#endif

void decimalToBinaryArray(int num, int binaryArray[], int *size) {
    *size = 0; // Inicializar el tamaño del array
    // Convertir el número a binario y almacenarlo en el array
    while (num > 0) {
        binaryArray[(*size)++] = num % 2; // Almacena el bit en el array
        num /= 2; // Divide el número por 2
    }
    // Rellenar con ceros si el número es menor de 4 bits
    while (*size < 4) {
        binaryArray[(*size)++] = 0; // Rellenar con ceros
    }
    // Invertir el array para que los bits estén en el orden correcto
    for (int i = 0; i < *size / 2; i++) {
        int temp = binaryArray[i];
        binaryArray[i] = binaryArray[*size - 1 - i];
        binaryArray[*size - 1 - i] = temp;
    }
}

int contador = 0;

int main(void) {
    volatile uint32_t ui32Loop;

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

    while(1) {
        int binaryArray[4]; // Array para almacenar hasta 4 bits
        int size;

        // Leer el estado del interruptor PJ1 y PJ0
        uint8_t switchState = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1);
        uint8_t switchState1 = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0);

        if (switchState1 == 0) { // Interruptor PJ0 está presionado
            for(ui32Loop = 0; ui32Loop < 500000; ui32Loop++){} // Esperar para evitar el rebote
            contador++;
            if (contador >= 15) {
                contador = 15; 
            }
        }

        if (switchState == 0) { // Interruptor PJ1 está presionado
            for(ui32Loop = 0; ui32Loop < 500000; ui32Loop++){} // Esperar para evitar el rebote
            contador--;
            if (contador < 0) {
                contador = 0; // Limitar a 0
            }
        }

        // Actualizar el array binario después de cambiar el contador
        decimalToBinaryArray(contador, binaryArray, &size);

        // Encender o apagar los LEDs según el array binario
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, (binaryArray[0] == 1) ? GPIO_PIN_1 : 0);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, (binaryArray[1] == 1) ? GPIO_PIN_0 : 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, (binaryArray[2] == 1) ? GPIO_PIN_4 : 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, (binaryArray[3] == 1) ? GPIO_PIN_0 : 0);
    }
}

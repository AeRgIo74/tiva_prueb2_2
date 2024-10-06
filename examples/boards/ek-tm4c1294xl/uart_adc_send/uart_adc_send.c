#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_adc.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include <string.h> // Para strlen


// Variable global para el reloj del sistema
uint32_t g_ui32SysClock;

// Configuración del UART y sus pines
void ConfigureUART(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // Habilitar GPIOA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // Habilitar UART0
    GPIOPinConfigure(GPIO_PA0_U0RX); // Configurar PA0 como RX
    GPIOPinConfigure(GPIO_PA1_U0TX); // Configurar PA1 como TX
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); // Configurar pines como UART
    UARTStdioConfig(0, 115200, g_ui32SysClock); // Configurar UART a 115200 bps
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count) {
    // Enviar los caracteres por UART
    while (ui32Count--) {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

// Configuración del ADC y sus pines
void ConfigureADC(void) {
    // Habilitar periféricos
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Habilitar ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // Habilitar GPIOE

    // Configurar el pin PE3 como entrada ADC (Canal 0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Configura PE3 como entrada del ADC
    // Configurar el ADC 
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0); // Secuencia 0
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // Canal 0
    ADCSequenceEnable(ADC0_BASE, 0); // Habilitar secuencia 0
    ADCIntClear(ADC0_BASE, 0); // Limpiar interrupción
}

// Función para convertir un número entero en una cadena de caracteres
void IntToChar(uint32_t value, char *buffer) {
    int i = 0;
    int j;
    char temp;

    // Manejar el caso de valor 0
    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0'; // Terminar cadena
        return;
    }

    // Extraer cada dígito del número
    while (value > 0) {
        buffer[i++] = (value % 10) + '0';  // Convertir el dígito a su valor ASCII
        value /= 10;
    }

    // Terminar cadena
    buffer[i] = '\0';

    // Invertir la cadena porque los dígitos están al revés
    for (j = 0; j < i / 2; j++) {
        temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
}

// Variable para almacenar el valor leído del ADC
uint32_t ui32ADCValue;

// Función principal
int main(void) {
    // Configuración del reloj del sistema
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                         SYSCTL_OSC_MAIN |
                                         SYSCTL_USE_PLL |
                                         SYSCTL_CFG_VCO_240), 120000000);

    // Habilitar interrupciones globales
    IntMasterEnable();

    ConfigureADC();

    // Configurar UART
    ConfigureUART();
    UARTprintf("Iniciando la lectura del ADC con timer...\n\n");

    

    char buffer[10];  // Buffer para almacenar la conversión del número a cadena

    // Bucle principal
    while (1) {
        ADCProcessorTrigger(ADC0_BASE, 0);
        // Esperar hasta que se complete la conversión ADC
        while (!ADCIntStatus(ADC0_BASE, 0, false)) {}

        // Limpiar la bandera de interrupción del ADC
        ADCIntClear(ADC0_BASE, 0);

        // Obtener el valor del ADC
        ADCSequenceDataGet(ADC0_BASE, 0, &ui32ADCValue);

        // Ajustar el valor leído al límite de 10 bits
        ui32ADCValue = ui32ADCValue >> 2; // Desplazamiento a la derecha para obtener solo 10 bits

        // Convertir el valor a cadena de caracteres
        IntToChar(ui32ADCValue, buffer);

        // Enviar la cadena por UART
        UARTSend((uint8_t *)buffer, strlen(buffer));
        UARTSend((uint8_t *)"\n", 1);  // Enviar salto de línea para formatear la salida
        // Introducir un retardo de 1 segundo
        SysCtlDelay((g_ui32SysClock / 3)*0.5); // Aproximadamente 1 segundo
    }
}

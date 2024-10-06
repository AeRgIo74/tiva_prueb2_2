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

    // Habilitar periféricos
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Habilitar ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // Habilitar Timer0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // Habilitar GPIOE

    // Configurar el pin PE3 como entrada ADC (Canal 0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Configura PE3 como entrada del ADC

    // Configurar UART
    ConfigureUART();
    UARTprintf("Iniciando la lectura del ADC con timer...\n\n");

    // Configurar el ADC con disparo por Timer
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0); // Secuencia 0
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0); // Canal 0
    ADCSequenceEnable(ADC0_BASE, 0); // Habilitar secuencia 0
    ADCIntClear(ADC0_BASE, 0); // Limpiar interrupción


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

        // Mostrar el valor leído por UART
        UARTprintf("Valor ADC ajustado (10 bits): %d\n", ui32ADCValue);

        // Introducir un retardo de 2 segundos
        SysCtlDelay(g_ui32SysClock / 3); // Aproximadamente 1 segundo
    }
}

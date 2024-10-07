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
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include <string.h> // Para strlen


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
void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count) {
    // Enviar los caracteres por UART
    while (ui32Count--) {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
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

// Función para convertir un número flotante en una cadena de caracteres
void FloatToChar(float value, char *buffer, int decimalPlaces) {
    int i = 0;

    // Manejar el caso de valor 0
    if (value == 0.0f) {
        buffer[i++] = '0';
        buffer[i] = '\0'; // Terminar cadena
        return;
    }

    // Manejar números negativos
    if (value < 0.0f) {
        buffer[i++] = '-';
        value = -value; // Hacer el valor positivo
    }

    // Obtener la parte entera
    uint32_t intPart = (uint32_t)value;
    float fracPart = value - intPart;

    // Extraer cada dígito de la parte entera
    char intBuffer[12]; // Buffer temporal para la parte entera
    int intIndex = 0;

    while (intPart > 0) {
        intBuffer[intIndex++] = (intPart % 10) + '0';  // Convertir el dígito a su valor ASCII
        intPart /= 10;
    }

    // Invertir la parte entera en el buffer
    for (int j = 0; j < intIndex / 2; j++) {
        char temp = intBuffer[j];
        intBuffer[j] = intBuffer[intIndex - j - 1];
        intBuffer[intIndex - j - 1] = temp;
    }

    // Agregar la parte entera al buffer principal
    for (int j = 0; j < intIndex; j++) {
        buffer[i++] = intBuffer[j];
    }

    // Agregar la parte decimal
    buffer[i++] = '.'; // Agregar el punto decimal

    // Extraer y agregar los dígitos de la parte decimal
    for (int j = 0; j < decimalPlaces; j++) {
        fracPart *= 10.0f;
        int digit = (int)fracPart;
        buffer[i++] = digit + '0'; // Convertir el dígito a su valor ASCII
        fracPart -= digit; // Mantener la parte decimal restante
    }

    // Terminar cadena
    buffer[i] = '\0';
}
// Variable para almacenar el valor leído del ADC
uint32_t ui32ADCValue;
float dutyCycle;
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
    ConfigureADC();
    //
    // Loop forever while the PWM signals are generated.
    //
    char buffer[10];  // Buffer para almacenar la conversión del número a cadena
    char buffer2[20];
    while (1)
    {
        ADCProcessorTrigger(ADC0_BASE, 0);
        // Esperar hasta que se complete la conversión ADC
        while (!ADCIntStatus(ADC0_BASE, 0, false)) {}

        // Limpiar la bandera de interrupción del ADC
        ADCIntClear(ADC0_BASE, 0);

        // Obtener el valor del ADC
        ADCSequenceDataGet(ADC0_BASE, 0, &ui32ADCValue);

        // Ajustar el valor leído al límite de 10 bits
        dutyCycle = (float)ui32ADCValue/4096;
        // if (dutyCycle <= 0.003){
        //     dutyCycle = 0;
        // }
        // Set the PWM pulse width based on the duty cycle.
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));

        IntToChar(ui32ADCValue, buffer);
        FloatToChar(dutyCycle, buffer2, 4); // Convierte a string con 2 decimales
        
        // Enviar la cadena por UART
        UARTSend((uint8_t *)buffer, strlen(buffer));
        UARTSend((uint8_t *)"   ", 3);  // Enviar salto de línea para formatear la salida
        UARTSend((uint8_t *)buffer2, strlen(buffer2));
        UARTSend((uint8_t *)"\n", 1);  // Enviar salto de línea para formatear la salida
        // Introducir un retardo de 1 segundo
        SysCtlDelay((g_ui32SysClock / 3)*0.5); // Aproximadamente 1 segundo
    }
}

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
#include "driverlib/fpu.h"
#include "driverlib/timer.h" 
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
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


////// ultrasonico
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
    SysCtlDelay(g_ui32SysClock / (1000000 * 3));  // Delay 10 us
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);

        // Esperar hasta que se reciba la señal Echo
    while (!echo_received);

        // Calcular la distancia en cm
    uint32_t time_diff = end_time - start_time;
    distance = ((time_diff/120) * 0.034) / 2;  // Usar la fórmula de la velocidad del sonido
    echo_received = false;
}
// Variable para almacenar el estado del LED
volatile bool ledState = false;
void Timer1IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    if (distance <= 7){
        // Alternar el estado del LED
        ledState = !ledState;
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, ledState ? GPIO_PIN_5 : 0);
    }
    else{
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
    }
    IntMasterEnable();
}


//*****************************************************************************
//
// Configure the UART and its pins. This must be called before UARTprintf.
//
//*****************************************************************************
//*****************************************************************************
// The UART interrupt handler.
//*****************************************************************************
#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
uint32_t bufferIndex = 0;
char receivedChar;
uint32_t contador = 0;
uint32_t direccion;

void UARTIntHandler(void) {
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ui32Status);

    // Reiniciar el índice del buffer
    bufferIndex = 0;
    memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    contador = 0;
    UARTprintf("Buffer recibido\n");
    while (UARTCharsAvail(UART0_BASE)) {
        receivedChar = UARTCharGetNonBlocking(UART0_BASE);
        
        // Solo almacenar si hay espacio en el buffer
        if (bufferIndex < BUFFER_SIZE - 1) {
            buffer[bufferIndex++] = receivedChar; // Almacena el carácter
            buffer[bufferIndex] = '\0'; // Asegúrate de que el string esté terminado
            // Si recibimos un salto de línea o un espacio (indicador de fin de palabra)
                // Comprobar si la palabra recibida es "adelante"
            if (strcmp(buffer, "adelante") == 0) {
                direccion = 1; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "atras") == 0) {
                direccion = 2; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "quieto") == 0) {
                direccion = 0; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "derecha") == 0) {
                direccion = 3; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "izquier") == 0) {
                direccion = 4; // Asignar 1 si la palabra es "adelante"
            }
            // Si se recibe un salto de línea, convertir a entero
            else if (isdigit(receivedChar)) {
                // Convertir la cadena a entero usando atoi
                contador = contador * 10 + (receivedChar - '0'); 
            }
        }
        
        UARTCharPutNonBlocking(UART0_BASE, receivedChar);
    }
}
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
void interrupcio_UART(void){
    // Habilitar interrupciones UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}
void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count) {
    // Enviar los caracteres por UART
    while (ui32Count--) {
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}


void UART7IntHandler(void) {
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ui32Status);

    // Reiniciar el índice del buffer
    bufferIndex = 0;
    memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    contador = 0;
    UARTprintf("Buffer recibido\n");
    while (UARTCharsAvail(UART0_BASE)) {
        receivedChar = UARTCharGetNonBlocking(UART0_BASE);
        
        // Solo almacenar si hay espacio en el buffer
        if (bufferIndex < BUFFER_SIZE - 1) {
            buffer[bufferIndex++] = receivedChar; // Almacena el carácter
            buffer[bufferIndex] = '\0'; // Asegúrate de que el string esté terminado
            // Si recibimos un salto de línea o un espacio (indicador de fin de palabra)
                // Comprobar si la palabra recibida es "adelante"
            if (strcmp(buffer, "adelante") == 0) {
                direccion = 1; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "atras") == 0) {
                direccion = 2; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "quieto") == 0) {
                direccion = 0; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "derecha") == 0) {
                direccion = 3; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "izquier") == 0) {
                direccion = 4; // Asignar 1 si la palabra es "adelante"
            }
            // Si se recibe un salto de línea, convertir a entero
            else if (isdigit(receivedChar)) {
                // Convertir la cadena a entero usando atoi
                contador = contador * 10 + (receivedChar - '0'); 
            }
        }
        
        UARTCharPutNonBlocking(UART0_BASE, receivedChar);
    }
}

void ConfigureUART7(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    //
    // Enable UART0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);

    //
    // Configure GPIO Pins for UART mode.
    //
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}

void interrupcio_UART7(void){
    // Habilitar interrupciones UART
    IntEnable(INT_UART7);
    UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);
}

void UART7Send(const uint8_t *pui8Buffer, uint32_t ui32Count) {
    // Enviar los caracteres por UART
    while (ui32Count--) {
        UARTCharPutNonBlocking(UART7_BASE, *pui8Buffer++);
    }
}
//*****************************************************************************
//

void GPIO_K(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}
void GPIO_L(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}
void user_led_N(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1|GPIO_PIN_0);
}
void user_led_F(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
}
void buzzer_E(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
}


void configura_TIMER1(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, g_ui32SysClock * 2);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER1_BASE, TIMER_A);
}
// Configure PWM for a 25% duty cycle signal running at 250Hz.
//
//*****************************************************************************

void ConfigurePWM_G0(void)
{
    uint32_t ui32PWMClockRate;

    //
    // The PWM peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // Enable the GPIO port that is used for the PWM output.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

    //
    // Configure the PWM function for this pin.
    //
    GPIOPinConfigure(GPIO_PG0_M0PWM4);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

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
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, (ui32PWMClockRate / 250));

    //
    // Set initial duty cycle to 0%.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);

    //
    // Enable PWM Out Bit 2 (PF2) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
}
void ConfigurePWM_F3(void)
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
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);

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
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0);

    //
    // Enable PWM Out Bit 2 (PF2) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}
void ConfigurePWM_F2(void)
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
void ConfigurePWM_F1(void)
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
    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);

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
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, (ui32PWMClockRate / 250));

    //
    // Set initial duty cycle to 0%.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);

    //
    // Enable PWM Out Bit 2 (PF2) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
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
     //
    // Enable processor interrupts.
    //
    IntMasterEnable();
    // Initialize the UART.
    //
    ConfigureUART();
    interrupcio_UART();
    ConfigureUART7();
    interrupcio_UART7();
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
    ConfigurePWM_F1();
    ConfigurePWM_F2();
    ConfigurePWM_F3();
    ConfigurePWM_G0();
    ConfigureADC();
    GPIO_L();
    GPIO_K();
    config_ultrasonic();
    user_led_F();
    user_led_N();
    buzzer_E();
    configura_TIMER1();
    //
    // Loop forever while the PWM signals are generated.
    //
    char buffer[10];  // Buffer para almacenar la conversión del número a cadena
    char buffer2[20];
    char buffer3[10];
    char buffer4[10];
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
        distancia();
        if(distance <= 7){
            direccion = 0;
        }
        // Encender o apagar los LEDs según el valor del contador usando desplazamiento de bits
        if(distance > 20) {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0 ); // Bit 2
        }
        else if (distance <= 20 && distance > 15)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0 ); // Bit 2
        }
        else if (distance <= 15)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // Bit 2
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // Bit 2
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1 ); // Bit 2
        }
        if (direccion == 0){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|0|0 );
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|0|0 );
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle * 0));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle * 0));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle * 0));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle * 0));

        }
        else if (direccion == 1){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|GPIO_PIN_2|0 );
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|GPIO_PIN_2|0 );
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        }
        else if (direccion == 2){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|GPIO_PIN_3 );
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|GPIO_PIN_3 );
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));
        }
        else if (direccion == 3){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|GPIO_PIN_3 );
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|GPIO_PIN_2|0 );
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle * 0.2));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle * 0.2));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));
        }
        else if (direccion == 4){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|GPIO_PIN_2|0 );
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|GPIO_PIN_3 );
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle * 0.2));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle * 0.2));
        }
        

        IntToChar(ui32ADCValue, buffer);
        FloatToChar(dutyCycle, buffer2, 4); // Convierte a string con 2 decimales
        IntToChar(direccion, buffer3);
        IntToChar(distance, buffer4);
        // Enviar la cadena por UART
        // UARTSend((uint8_t *)buffer, strlen(buffer));
        // UARTSend((uint8_t *)"   ", 3);  // Enviar salto de línea para formatear la salida
        // UARTSend((uint8_t *)buffer2, strlen(buffer2));
        // UARTSend((uint8_t *)"   ", 3);  // Enviar salto de línea para formatear la salida
        // UARTSend((uint8_t *)buffer3, strlen(buffer3));
        // UARTSend((uint8_t *)"\n", 1);  // Enviar salto de línea para formatear la salida
        UARTSend((uint8_t *)buffer4, strlen(buffer4));
        UARTSend((uint8_t *)"\n", 1);  // Enviar salto de línea para formatear la salida
        
        // Introducir un retardo de 1 segundo
        SysCtlDelay((g_ui32SysClock / 3)*0.5); // Aproximadamente 1 segundo
    }
}

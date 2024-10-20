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


// System clock rate in Hz.
uint32_t g_ui32SysClock;

// The error routine that is called if the driver library encounters an error.
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


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

/// Configuración de pines
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
    distance = ((time_diff / 120) * 0.034) / 2;  // Usar la fórmula de la velocidad del sonido
    echo_received = false;
}



void configura_TIMER2(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER2_BASE, TIMER_A, g_ui32SysClock*2);
    IntEnable(INT_TIMER2A);
    TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER2_BASE, TIMER_A);
}
volatile bool ledState1 = false;
uint32_t send = 0;
uint32_t marcador;
uint32_t estado = 1;
void Timer2IntHandler(void)
{
 
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    if (estado == 0){
        if (distance <= 7){
            send++;
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2)*0.5));
            UARTSend((uint8_t *)"quieto", 6);
            UARTSend((uint8_t *)"\n", 1);
            if (send == 1){
                UARTSend((uint8_t *)"alerta", 6);
                UARTSend((uint8_t *)"\n", 1);
            }
            // Alternar el estado del LED
            ledState1 = !ledState1;
            //UARTSend((uint8_t *)"alerta", 6);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, ledState1 ? GPIO_PIN_5 : 0);
        }
        else{
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
            send =0;
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2)*0.0));
            if (marcador == 0)
            {
                UARTSend((uint8_t *)"derecha", 7);
                UARTSend((uint8_t *)"\n", 1);
            }
            else if (marcador == 1)
            {
                UARTSend((uint8_t *)"adelante", 8);
                UARTSend((uint8_t *)"\n", 1);
            }
            else if (marcador == 2)
            {
                UARTSend((uint8_t *)"quieto", 7);
                UARTSend((uint8_t *)"\n", 1);
            }
            
        }
    }
    else{
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2)*0.0));    
    }
    IntMasterEnable();
}
void configura_TIMER1(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, g_ui32SysClock);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER1_BASE, TIMER_A);
}
volatile bool ledState = false;
void Timer1IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    if (estado == 0){
        if (distance >= 20){
            // Alternar el estado del LED
            ledState = !ledState;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, ledState ? GPIO_PIN_1 : 0);
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ledState ? 0 : GPIO_PIN_0);
        }
        else{
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
        }
    }
    else{
        if (distance <= 7){
            // Alternar el estado del LED
            ledState = !ledState;
            UARTSend((uint8_t *)"alerta", 6);
            UARTSend((uint8_t *)"\n", 1);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, ledState ? GPIO_PIN_5 : 0);
        }
        else{
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
        }
    }

    IntMasterEnable();
}


//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void UARTConfig(void){
    // Configurar UART para mostrar los datos en la consola
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    GPIOPinConfigure(GPIO_PP0_U6RX);
    GPIOPinConfigure(GPIO_PP1_U6TX);
    GPIOPinTypeUART(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART6_BASE, g_ui32SysClock, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    
    
}
void interrupcio_UART(void){
    // Habilitar interrupciones UART
    IntEnable(INT_UART6);
    UARTIntEnable(UART6_BASE, UART_INT_RX | UART_INT_RT);
}
#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
uint32_t bufferIndex = 0;
char receivedChar;
uint32_t contador = 0;
uint32_t direccion;
uint32_t valor;
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    int numLength = 0;     // Variable para almacenar el número de dígitos del número recibido
    int contadorLength = 0; // Variable para almacenar el número de dígitos del contador

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART6_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART6_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    // Reiniciar el índice del buffer
    bufferIndex = 0;
    memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    contador = 0;
    while(UARTCharsAvail(UART6_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        receivedChar = UARTCharGetNonBlocking(UART6_BASE);
        // Solo almacenar si hay espacio en el buffer
        if (bufferIndex < BUFFER_SIZE - 1) {
            buffer[bufferIndex++] = receivedChar; // Almacena el carácter
            buffer[bufferIndex] = '\0'; // Asegúrate de que el string esté terminado
            // Si recibimos un salto de línea o un espacio (indicador de fin de palabra)
                // Comprobar si la palabra recibida es "adelante"
            if (isdigit(receivedChar)) {
                // Convertir la cadena a entero usando atoi
                contador = contador * 10 + (receivedChar - '0'); 
                numLength++;  // Increment the count of numeric characters
            }
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
            else if (strcmp(buffer, "dder") == 0) {
                direccion = 5; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "iizq") == 0) {
                direccion = 6; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "ader") == 0) {
                direccion = 7; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "aizq") == 0) {
                direccion = 8; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "bder") == 0) {
                direccion = 9; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "bizq") == 0) {
                direccion = 10; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "gder") == 0) {
                direccion = 11; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "gizq") == 0) {
                direccion = 12; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "auto") == 0) {
                estado = 0;
            }
            else if (strcmp(buffer, "manu") == 0) {
                estado = 1;
            }
            // Si se recibe un salto de línea, convertir a entero
            
        }
    }
    int tempContador = contador;
    do
    {
        contadorLength++;       // Incrementa la longitud del contador
        tempContador /= 10;     // Divide el número entre 10 para remover el último dígito
    } while (tempContador != 0);
    if (numLength == contadorLength)
    {
        // Si las longitudes coinciden, puedes tomar alguna acción
        valor = contador;
    }
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPutNonBlocking(UART6_BASE, *pui8Buffer++);
    }
}

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
///GPIOS
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
void GPIO_E(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4|GPIO_PIN_5);
}

///PWM
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
void ConfigurePWM_G1(void)
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
    GPIOPinConfigure(GPIO_PG1_M0PWM5);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_1);

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
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 0);

    //
    // Enable PWM Out Bit 2 (PF2) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
}

// Definición de la estructura Motor
typedef struct Motor {
    uint32_t puerto; // El puerto GPIO (por ejemplo, GPIO_PORTL_BASE)
    uint8_t pinA;    // Primer pin del puente H
    uint8_t pinB;    // Segundo pin del puente H
} Motor;

// Función para inicializar el motor
void Motor_init(Motor* motor, uint32_t p, uint8_t pA, uint8_t pB) {
    motor->puerto = p;
    motor->pinA = pA;
    motor->pinB = pB;
}
// Movimiento hacia adelante
void motorAdelante(Motor* motor) {
    GPIOPinWrite(motor->puerto, motor->pinA | motor->pinB, motor->pinA); // Activa pinA, desactiva pinB
}

// Movimiento hacia atrás
void motorAtras(Motor* motor) {
    GPIOPinWrite(motor->puerto, motor->pinA | motor->pinB, motor->pinB); // Activa pinB, desactiva pinA
}

// Detener motor
void motorDetener(Motor* motor) {
    GPIOPinWrite(motor->puerto, motor->pinA | motor->pinB, 0); // Desactiva ambos pines
}
// Estructura para controlar múltiples motores
typedef struct ControlMotores {
    Motor motores[4]; // Array para almacenar 4 motores
} ControlMotores;
// Inicializar control de motores
void ControlMotores_init(ControlMotores* control) {
    // Inicializar motores, cada uno con un puerto y dos pines
    Motor_init(&control->motores[0], GPIO_PORTL_BASE, GPIO_PIN_0, GPIO_PIN_1); // Motor 1
    Motor_init(&control->motores[1], GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_3); // Motor 2
    Motor_init(&control->motores[2], GPIO_PORTK_BASE, GPIO_PIN_0, GPIO_PIN_1); // Motor 3
    Motor_init(&control->motores[3], GPIO_PORTK_BASE, GPIO_PIN_2, GPIO_PIN_3); // Motor 4
}
// Movimiento hacia adelante de un motor específico
void ControlMotores_moverAdelante(ControlMotores* control, int motorId) {
    motorAdelante(&control->motores[motorId]);
}

// Movimiento hacia atrás de un motor específico
void ControlMotores_moverAtras(ControlMotores* control, int motorId) {
    motorAtras(&control->motores[motorId]);
}

// Detener un motor específico
void ControlMotores_detenerMotor(ControlMotores* control, int motorId) {
    motorDetener(&control->motores[motorId]);
}

// Definición de la estructura PWM
typedef struct PWM {
    uint32_t base; // Base del PWM
    uint32_t output; // Salida del PWM
    uint32_t gen; // Generador del PWM
} PWM;

// Función para inicializar un PWM
void PWM_init(PWM* pwm, uint32_t base, uint32_t output, uint32_t gen) {
    pwm->base = base;
    pwm->output = output;
    pwm->gen = gen;
}

// Función para configurar el ancho de pulso
void PWM_setDutyCycle(PWM* pwm, float dutyCycle) {
    uint32_t period = PWMGenPeriodGet(pwm->base, pwm->gen);
    PWMPulseWidthSet(pwm->base, pwm->output, (period * dutyCycle));
}

// Función para inicializar y configurar los PWMs
void PWM_configurar(PWM* pwms) {
    PWM_init(&pwms[0], PWM0_BASE, PWM_OUT_1, PWM_GEN_0);
    PWM_init(&pwms[1], PWM0_BASE, PWM_OUT_2, PWM_GEN_1);
    PWM_init(&pwms[2], PWM0_BASE, PWM_OUT_3, PWM_GEN_1);
    PWM_init(&pwms[3], PWM0_BASE, PWM_OUT_4, PWM_GEN_2);
}
void autonomo(void);
void manual(void);
float dutyCycle;
//*****************************************************************************
//
// This example demonstrates how to send a string of data to the UART.
//
//*****************************************************************************

int
main(void)
{
    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);


    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    UARTConfig(),

    interrupcio_UART();
    //
    // Prompt for text to be entered.
    //
    UARTSend((uint8_t *)"\033[2JEnter text: ", 16);

    //
    // Loop forever echoing data through the UART.
    //
    //ultrasonic
    config_ultrasonic();
    ///pwm
    ConfigurePWM_F1();
    ConfigurePWM_F2();
    ConfigurePWM_F3();
    ConfigurePWM_G0();
    ConfigurePWM_G1();
    ///gpios
    GPIO_L();
    GPIO_K();
    user_led_F();
    user_led_N();
    GPIO_E();
    configura_TIMER1();
    configura_TIMER2();
    
    while(1)
    {
        if (estado == 0){
            autonomo();
        }
        else {
            manual();
        }
    }
}
void autonomo(void){
    // Crear objeto de control de motores
    ControlMotores controlMotores;
    ControlMotores_init(&controlMotores);

    PWM pwms[4];
    // Inicializar y configurar PWMs
    PWM_configurar(pwms);
    dutyCycle = (float)20/100;
        distancia();
        if (distance >= 20)
        {
            marcador =0;
            ControlMotores_moverAtras(&controlMotores, 3);
            ControlMotores_moverAtras(&controlMotores, 2);
            ControlMotores_moverAdelante(&controlMotores, 1);
            ControlMotores_moverAdelante(&controlMotores, 0);
            PWM_setDutyCycle(&pwms[3], dutyCycle);
            PWM_setDutyCycle(&pwms[2], dutyCycle*0.2);
            PWM_setDutyCycle(&pwms[1], dutyCycle);
            PWM_setDutyCycle(&pwms[0], dutyCycle*0.2);
        }
        else{
            marcador =1;
            ControlMotores_moverAdelante(&controlMotores, 0);
            ControlMotores_moverAdelante(&controlMotores, 1);
            ControlMotores_moverAdelante(&controlMotores, 2);
            ControlMotores_moverAdelante(&controlMotores, 3);
            PWM_setDutyCycle(&pwms[3], dutyCycle);
            PWM_setDutyCycle(&pwms[2], dutyCycle);
            PWM_setDutyCycle(&pwms[1], dutyCycle);
            PWM_setDutyCycle(&pwms[0], dutyCycle);
            if (distance <= 7){
                marcador =2;
                ControlMotores_detenerMotor(&controlMotores, 0);
                ControlMotores_detenerMotor(&controlMotores, 1);
                ControlMotores_detenerMotor(&controlMotores, 2);
                ControlMotores_detenerMotor(&controlMotores, 3);
                PWM_setDutyCycle(&pwms[3], dutyCycle*0.0);
                PWM_setDutyCycle(&pwms[2], dutyCycle*0.0);
                PWM_setDutyCycle(&pwms[1], dutyCycle*0.0);
                PWM_setDutyCycle(&pwms[0], dutyCycle*0.0);
            }
        }
}
void manual(void){
        dutyCycle = (float)valor/100;
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
        else if (direccion == 5){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|GPIO_PIN_2|0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|GPIO_PIN_3);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        }
        else if (direccion == 6){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|GPIO_PIN_3);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|GPIO_PIN_2|0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        }
        else if (direccion == 7){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|GPIO_PIN_2|0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        }     
        else if (direccion == 8){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|GPIO_PIN_2|0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        } 
        else if (direccion == 9){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|0|GPIO_PIN_3);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        } 
        else if (direccion == 10){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|0|0|GPIO_PIN_3);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|0|0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        } 
        else if (direccion == 11){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|GPIO_PIN_2|0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0|GPIO_PIN_1|GPIO_PIN_2|0);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        } 
        else if (direccion == 12){
            GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|GPIO_PIN_3);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_0|0|0|GPIO_PIN_3);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle));
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * dutyCycle));

        } 
}
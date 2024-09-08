#include <stdio.h>

void decimalToBinaryArray(int num, int binaryArray[], int *size) {
    *size = 0; // Inicializar el tamaño del array

    // Convertir el número a binario y almacenarlo en el array
    while (num > 0) {
        binaryArray[(*size)++] = num % 2; // Almacena el bit en el array
        num /= 2; // Divide el número por 2
    }

    // Invertir el array para que los bits estén en el orden correcto
    for (int i = 0; i < *size / 2; i++) {
        int temp = binaryArray[i];
        binaryArray[i] = binaryArray[*size - 1 - i];
        binaryArray[*size - 1 - i] = temp;
    }
    
}

void printBinaryArray(int binaryArray[], int size) {
    printf("Valores de cada columna:\n");
    for (int i = 0; i < size; i++) {
        printf("Columna %d: %d\n", i, binaryArray[i]);
    }
}

int main() {
    int number = 1;
    int binaryArray[4]; // Array para almacenar hasta 32 bits
    int size;

    // Convertir el número a binario
    decimalToBinaryArray(number, binaryArray, &size);
    
    // Imprimir los valores de cada columna
    printBinaryArray(binaryArray, size);

    return 0;
}

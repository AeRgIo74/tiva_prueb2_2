#include <stdio.h>
#include <stdlib.h>  // Necesario para usar atoi

int main() {
    char input[100];  // Buffer para almacenar la entrada del usuario
    int number;       // Variable para almacenar el número convertido

    // Solicitar al usuario que ingrese un número
    printf("Ingrese un número: ");
    fgets(input, sizeof(input), stdin); // Leer la entrada del usuario

    // Convertir la cadena a entero
    number = atoi(input);

    // Imprimir el resultado
    printf("El número ingresado es: %d\n", number);

    return 0;
}

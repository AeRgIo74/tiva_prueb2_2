# 1. Agregue n números,


def sumar_numeros(n):
    suma = 0
    for i in range(n):
        numero = float(input(f"Ingrese el número : "))
        suma += numero
    print(f"Suma : {suma}")

# Solicita al usuario el número de elementos
n = int(input("¿Cuántos números deseas ingresar? "))
sumar_numeros(n)











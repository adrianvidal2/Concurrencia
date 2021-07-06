# Resolución de diferentes problemas de carreras críticas

Se aportan implementaciones para distintos problemas clásicos de sincronización como puede ser el problema de los **filósofos** o **productor-consumidor** entre otros.
Se utilizan como mecanismos: semáforos, mutexes y variables de condición. Estas implementaciones se han **escrito en C**.


## Ficheros disponibles
- **mutex-variables**: Implementaciones realizadas con el uso de un mutex y variables de condición. En ella se encuentran el problema de los **filósofos** y del
**productor-consumidor**.
- **semaphore**: Implementaciones realizadas mediante el uso de semáforos. Se encuentran: problema del **barbero**, **productor-consumidor**, **fumadores y estanco**.

## ¿Cómo compilar y ejecutar?

Para la compilación de las implementaciones será necesario:
``` 
cd <ubicacion-implementacion>
gcc -Wall [implementacion] -pthread -o [ejecutable]
```
Para la ejecución de la implementación del problema del **barbero** se necesitará:
```
cd <ubicacion-implementacion>
./barbero [numero-clientes]
```
Para poder realizar la ejecución del problema del **productor-consumidor** concurrente. En primer lugar se lanzará el productor:
```
cd <ubicacion-productor>
./productor [numero-productores]
```
A continuación, se lanza el consumidor:
```
cd <ubicacion-consumidor>
./consumidor [numero-consumidores]
```
El resto de las ejecuciones se realizará de la siguiente forma:
```
cd <ubicacion-implementacion>
./implementacion
```
## Autores:
[Adrián Vidal Lorenzo](https://github.com/adrianvidal2)

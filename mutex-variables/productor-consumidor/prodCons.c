#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define N 5
#define P 5
#define C 4
#define IT 5
#define ITEM_BY_P 10

#define VERDE "\x1b[32m"
#define ROJO "\x1b[31m"
#define RESET "\x1b[0m"

/*
 * escribir: Posición donde el productor deberá escribir el producto
 * leer:	 Posición donde el consumidor leerá el producto
 * buffer:	 Buffer para almacenar los diversos productos creados por el productor
 */

typedef struct{
	int posicion;
}hilos;

int escribir = 0;
int leer = 0;
int buffer[N];
int elementos = 0;
int aConsumir;

pthread_mutex_t the_mutex;
pthread_cond_t condc,condp;

/*Función encargada de generar un número aleatorio que será el producido*/
int produce_item();
/*Función encargada de las tareas correspondientes a un productor*/
void *producir(void *arg);
/*Función encargada de las tareas correspondientes a un consumidor*/
void *consumir(void *arg);


int main(int argc, char **argv) {
	pthread_t productores[P];
	pthread_t consumidores[C];
	hilos prod[P];
	hilos cons[C];
	srand(time(NULL));
	pthread_mutex_init(&the_mutex,0);
	pthread_cond_init(&condc, 0);
	pthread_cond_init(&condp, 0);

	aConsumir  = P*ITEM_BY_P;
	for(int i = 0; i < C; i++){
		cons[i].posicion = i;
		pthread_create(&consumidores[i], NULL, consumir, (void *)&cons[i]);
	}

	for(int i = 0; i < P; i++){
		prod[i].posicion = i;
		pthread_create(&productores[i], NULL, producir, (void *)&prod[i]);
	}

	for(int i = 0; i < P; i++){
		pthread_join(productores[i], 0);
	}

	for(int i = 0; i < C; i++){
		pthread_join(consumidores[i],0);
	}
	pthread_cond_destroy(&condc);
	pthread_cond_destroy(&condp);
	pthread_mutex_destroy(&the_mutex);

	return EXIT_SUCCESS;

}

int produce_item(){
	return rand()%(122-48+1)+48;
}

void *producir(void *arg){
	int item;
	hilos *prod = (hilos *)arg;

	for(int i = 0; i < ITEM_BY_P; i++){
		item = produce_item();
		sleep(rand()%5);
		pthread_mutex_lock(&the_mutex);
		/*Si el número de elementos es igual al máximo posible, se bloquea la variable de condición*/
			while(elementos == IT)pthread_cond_wait(&condp,&the_mutex);
			buffer[escribir] = item;
			elementos++;
			printf(ROJO"Producido: %d\tPosición: %d\tProductor: %d\n"RESET,item,escribir,prod->posicion);
			escribir++;
			/*Si se llega al final de la cola se vuelve al principio*/
			if(escribir >= IT) escribir = escribir % IT;
			
		pthread_cond_signal(&condc);
		pthread_mutex_unlock(&the_mutex);
	}
	pthread_cond_broadcast(&condp);
	pthread_exit(0);
}

void *consumir(void *arg){
	int item;
	hilos *con = (hilos *)arg;

	while(aConsumir != 0){
		pthread_mutex_lock(&the_mutex);
		/*Si no quedan elementos o no existen se bloquea la variable de condición*/
		while(elementos == 0 && aConsumir != 0)pthread_cond_wait(&condc, &the_mutex);
		if(aConsumir != 0){
			item = buffer[leer];
			elementos--;
			aConsumir--;
			printf("\t\t\t"VERDE "Consumido: %d\tposicion: %d\tConsumidor: %d\tFaltan: %d\n"RESET,item,leer,con->posicion,aConsumir);
			leer++;			
			/*Si se llega al final de la cola se vuelve al principio*/
			if(leer >= IT) leer = leer % IT;			
			pthread_cond_signal(&condp);
		}
		pthread_mutex_unlock(&the_mutex);
		sleep(rand()%5);
	}
	pthread_cond_broadcast(&condc);
	pthread_exit(0);
}

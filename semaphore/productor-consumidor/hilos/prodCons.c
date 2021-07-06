#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#define N 8
#define P 2
#define C 2
#define IT 50
/*
 * Se usan códigos de colores para una clara distinción del productor y el consumidor.
 */
#define VERDE "\x1b[32m"
#define ROJO "\x1b[31m"
#define RESET "\x1b[0m"


int cuenta = 0;
int buffer[N];	//Variable global en donde se almacenará los items producidos

/*
 * accesos: Controla los accesos a la región crítica del programa.
 * vacias: Cuenta cuantas posiciones se encuentran libres para la introducción de items.
 * full: Cuenta las posiciones ocupadas en el buffer, contienen items.
 */
sem_t *accesos;
sem_t *vacias;
sem_t *full;

//Estructura en la que se almacenará quién es el productor y el consumidor.
typedef struct{
	int posicion;
}hilo;
/*
 * CABECERAS DE FUNCIONES
 */
// Función encargada de la creación de un número aleatorio entre 40 y 122. Retorna dicho número
int produce_item();
//Función encargada de crear los semáforos.
void crearSemaforo();
/*
 * Función encargada del cierre y eliminación del kernel de los semáforos.
 * Esta función se ejecutará al comienzo del programa por si algunos de los semáforos no ha
 * sido eliminado correctamente.
 */
void destruirSemaforos();
//Acción del hilo de productor.
void *producir();
//Acción del hilo consumidor
void *consumir();
//Se inicializan todas las posiciones del buffer a -1.
void inicializarBuffer();

int main(int argc, char **argv) {
	pthread_t consumidores[C];
	pthread_t productores[P];
	hilo prod[P];
	hilo cons[C];
	srand(time(NULL));
	//Se destruyen los semáforos por si estos no habían sido eliminados correctamente en alguna ejecución anterior
	destruirSemaforos();
	//Se crean los semáforos, con los valores iniciales correspondientes
	crearSemaforo();
	//Se inicializan todas las posiciones del buffer a -1, valor que se considera vacío en este caso
	inicializarBuffer();
	//Bucles de creación de hilos (productor y consumidor respectivamente), se almacena su posición y se determina su comportamiento.
	for(int i = 0; i < P; i++){
		prod[i].posicion = i;
		pthread_create(&productores[i], NULL, producir, (void *)&prod[i]);
	}
	for(int i = 0; i < C; i++){
		cons[i].posicion = i;
		pthread_create(&consumidores[i], NULL, consumir, (void *)&cons[i]);
	}
	//Bucles de espera de hilos (productor y consumidor respectivamente).
	for(int i = 0; i < P; i++){
		pthread_join(productores[i], NULL);
	}

	for(int i = 0; i < C; i++){
		pthread_join(consumidores[i],NULL);
	}
	//Se destruyen los semáforos.
	destruirSemaforos();
	return EXIT_SUCCESS;
}

/*
 * FUNCIONES
 */
int produce_item(){
	return rand()%(122-48+1)+48;
}

void crearSemaforo(){
	if((vacias = sem_open("VACIAS", O_CREAT,0700,N)) == SEM_FAILED){
		perror("Error creando vacias: ");
		exit(EXIT_FAILURE);
	}

	if((full = sem_open("FULL", O_CREAT,0700,0)) == SEM_FAILED){
		perror("Error creando full: ");
		exit(EXIT_FAILURE);
	}

	if((accesos = sem_open("ACCESOS", O_CREAT,0700,1)) == SEM_FAILED){
		perror("Error creando los semáforos: ");
		exit(EXIT_FAILURE);
	}
	printf("\nSemáforos creados correctamente\n\n");
}

void destruirSemaforos(){
	//Cierre de los semáforos
	sem_close(vacias);
	sem_close(full);
	sem_close(accesos);
	
	//Eliminación de los semáforos
	sem_unlink("VACIAS");
	sem_unlink("FULL") ;
	sem_unlink("ACCESOS");
}

void *producir(void *arg){
	int i = 0;
	hilo *prod;
	int item;
	prod = (hilo *)arg;
	while(i < (IT/P)){
		item = produce_item();
		sleep(rand()%4);
		sem_wait(vacias);
		sem_wait(accesos);
		buffer[cuenta] = item;
		cuenta++;
		printf(ROJO"producido: %d\t posición: %d\tproductor: %d\n"RESET,item,cuenta-1,prod->posicion);
		sem_post(accesos);
		sem_post(full);
		i++;
	}
	pthread_exit(NULL);
}

void *consumir(void *arg){
	int i = 0,item;
	hilo *cons;
	cons = (hilo *)arg;
	while(i < (IT/C)){
		sem_wait(full);
		sem_wait(accesos);
		item = buffer[cuenta-1];
		cuenta--;
		sem_post(accesos);
		sem_post(vacias);
		printf(VERDE"consumido: %d\t posicion: %d\t consumidor: %d\n"RESET,item,cuenta,cons->posicion);
		sleep(rand()%4);
		i++;
	}
	pthread_exit(NULL);
}

void inicializarBuffer(){
	for(int i = 0;  i < N; i++){
		buffer[i] = -1;
	}
}

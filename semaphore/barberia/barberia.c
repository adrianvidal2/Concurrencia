#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

	/*CONSTANTES*/
/*
 * LARGO: el cliente tiene el pelo largo
 * CORTO: el cliente tiene el pelo corto
 * SILLAS: número de sillas en la barbería
 */
#define LARGO 'l'
#define CORTO 'c'
#define SILLAS 4

	/*COLORES CONSTANTES*/
#define RESET   "\x1b[0m"
#define CYAN     "\x1b[36m"
#define VERDE  "\x1b[32m"

	/*VARIABLES GLOBALES*/
/*
 * *pelo: array que almacenará el estado de la cabellera de los clientes
 *  isAtendidos: variable que determinará si todos los clientes han sido atendidos
 *  sillasLibres: número de sillas disponibles en un momento
 *  numClient: almacena el número de clientes que recibira la peluquería
 */
char *pelo;
int isAtendidos;
int sillasLibres = SILLAS;
int numClient = 0;

	/*SEMAPHORE*/
/*
 * *barberoDesp: determina si el barbero se encuentra disponible
 * *clientes: estará activo mientras haya clientes en la barbería
 * *accesos: controla el acceso a la región crítica (0 y 1, mutex)
 * */
sem_t *barberoDesp, *clientes, *accesos;

	/*HEADERS*/
//Funciones de comportamiento de los semáforos

/*
 * Función que creará los diferentes semáforos para la ejecución
 * del programa.
 */
void initSem();

/*
 * Función encargada del cierre y destrucción de los semáforos.
 */
void destroySem();

/*
 * Función encargada de la inicialización del array pelo.
 *
 * int numClientes: número de clientes que recibirá la
 * 					barbería.
 */
void initHair(int numClientes);

/*Función asociada al hilo del barbero*/
void *barbero(void *args);

//Funciones para los clientes

/*
 * Función que dormirá al cliente entre 1 y 30 segundos en caso
 * de no poder sentarse en la barbería
 *
 * int clien: cliente que no puede sentarse en las sillas.
 */
void pasear(int clien);

/*
 * Función que proporcionará un cambio en el array pelo, pasará
 * de LARGO a CORTO.
 *
 * int clien: cliente al que se le corta el pelo
 */
void cortar(int clien);

/*Función que determina el comportamiento del cliente*/
void *cliente(void *args);

int main(int argc, char **argv) {
	if(argc == 2){	//El programa debe leer 2 parámetros por línea de comandos

		//Determinación de número de clientes de la barbería
		sscanf(argv[1],"%d",&numClient);

		//Hilo para el barbero y array para los clientes. Se utiliza un array
		//para almacenar el número del cliente.
		pthread_t barber;
		pthread_t clients[numClient];
		int cli[numClient];

		/*
		 * Destrucción de los semáforos ante una mala eliminación previa.
		 * Posteriormente se realiza la apertura.
		 */
		destroySem();
		initSem();

		//Inicialización del array pelo
		initHair(numClient);

		//Creación de hilos y determinación del comportamiento.
		pthread_create(&barber, NULL, barbero, NULL);

		for(int i = 0; i < numClient; i++){
			cli[i] = i;
			pthread_create(&clients[i], NULL, cliente, (void*)&cli[i]);
		}

		//Espera de hilos
		for(int i = 0; i < numClient; i++){
			pthread_join(clients[i], NULL);
		}

		/*
		 * Se realiza un cambio a la variable globar y se desbloquea el
		 * semáforo clientes ante un posible bloqueo del barbero.
		 */
		isAtendidos = 1;
		sem_post(clientes);

		pthread_join(barber, NULL);

		//Destrucción de los semáforos y liberación de memoria.
		destroySem();
		free(pelo);

	}else{	//Si el número de argumentos es incorrecto se informa de un error.
		fprintf(stderr,"ejecutable [NUM_CLIENTES]\n");
	}
}

void initSem(){
	//Apertura del semáforo del barbero
	if((barberoDesp = sem_open("BARBERO", O_CREAT, 0700, 0)) == SEM_FAILED){
		perror("Error sem_open barbero: ");
		exit(EXIT_FAILURE);
	}

	//Apertura del semáforo de clientes
	if((clientes = sem_open("CLIENTS", O_CREAT, 0700, 0)) == SEM_FAILED){
		perror("Error sem_open clientes: ");
		exit(EXIT_FAILURE);
	}

	/*
	 * Apertura del semáforo de acceso a la región crítica.
	 * Inicialmente se permite el acceso.
	 */
	if((accesos = sem_open("ACCESOS", O_CREAT, 0700, 1)) == SEM_FAILED){
		perror("Error sem_open accesos");
		exit(EXIT_FAILURE);
	}
}

void destroySem(){
	//Cierre de los semáforos
	sem_close(barberoDesp);
	sem_close(clientes);
	sem_close(accesos);

	//Eliminación de semáforos
	sem_unlink("BARBERO");
	sem_unlink("CLIENTS");
	sem_unlink("ACCESOS");
}

void initHair(int numClientes){

	isAtendidos = 0;

	//Reserva de memoria e inicialización del array pelo.
	pelo = (char *)malloc(numClientes * sizeof(char));

	for(int i = 0; i < numClientes; i++){
		pelo[i] = 'l';

	}
}

void pasear(int clien){
	printf("CLIENTE %d: ¡Vaya, parece que no hay sillas! Iré a pasear\n\n",clien);
	sleep(rand()%(30-1+1)+1);
}

void cortar(int clien){
	sleep(4);
	printf(VERDE"CLIENTE %d: El barbero me ha dejado listo\n\n"RESET, clien);

	pelo[clien] = CORTO;
}

void *barbero(void *args){

	while(!isAtendidos){
		//Espera señal del cliente
		sem_wait(clientes);

		if(!isAtendidos){
			printf(CYAN"\nBarbero: hay clientes para atender\n"RESET);
			//Se intenta acceder a región crítica
			sem_wait(accesos);
			//Se libera una silla de la sala de espera
			sillasLibres += 1;
			//Envía señal de barbero dispuesto a cortar pelo
			sem_post(barberoDesp);
			//Se libera región crítica
			sem_post(accesos);
		}
	}

	pthread_exit(NULL);
}

void *cliente(void *args){
	int *cita;
	cita = (int *)args;

	while(pelo[*cita] == 'l'){
		printf("Soy el cliente: %d\n", *cita);
		//Se intenta acceder a región crítica
		sem_wait(accesos);

		if(sillasLibres > 0){
			printf("\nVoy a sentarme en una silla. Cliente: %d!\n",*cita);
			//El cliente se sienta en la silla
			sillasLibres -= 1;
			//Aviso a barbero de llegada de clientes
			sem_post(clientes);
			//Liberación de región crítica
			sem_post(accesos);
			//Espera para poder cortar el pelo
			sem_wait(barberoDesp);
			cortar(*cita);
		}else{
			sem_post(accesos);
			pasear(*cita);
		}

	}

	pthread_exit(NULL);
}

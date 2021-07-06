#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

/*
 * N: número máximo de elementos que puede contener el buffer
 * C: número de consumidores que se crearán
 * IT: número de items que se podrán crear
 */
#define N 8
#define C 5
#define IT 50

int *cuenta;
/*
 * Creación de los semáforos que se usarán en el programa
 * accesos: Controla los accesos a la región crítica del programa.
 * vacias: Cuenta cuantas posiciones se encuentran libres para la introducción de items.
 * full: Cuenta las posiciones ocupadas en el buffer, contienen items.
 */
sem_t *accesos;
sem_t *vacias;
sem_t *full;
/*CABECERAS DE FUNCIONES*/
//Función encargada de la apertura de los semáforos creados en el productor
void abrirSemaforo();
/*
 * Función encargada del cierre y eliminación del kernel de los semáforos.
 * Esta función se ejecutará al comienzo del programa por si algunos de los semáforos no ha
 * sido eliminado correctamente.
 */
void destruirSemaforos();



int main(int argc, char **argv) {
	int item,buffer,consumCreado, wstatus;
	pid_t pid;

	srand(time(NULL));
	//Impresión del código del consumidor
	printf("\t\t--- Código: %d ---\n",getpid());

	//Apertura del buffer y acceso a la pryección de memoria en la que se encuentra el productor con el archivo de respaldo.
	if((buffer = open("buffer.txt",O_RDWR | O_CREAT | O_APPEND,S_IRWXU | S_IRWXG | S_IRWXO)) < 0){
			perror("Error al abrir el buffer: ");
			exit(EXIT_FAILURE);
		}

	if((cuenta = (int *)mmap(NULL,(N+1)*sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED,buffer,0)) == MAP_FAILED){
			perror("Error al abrir buffer: ");
			exit(EXIT_FAILURE);
	}

	//Apertura de los semáforos.
	abrirSemaforo();

	/*
	 * El siguiente bucle for, crea el número definido de consumidores, al igual que como
	 * con los productores, se sale del bucle cuando se trate del hijo el que esta
	 * en ejecución.
	 */
	for(consumCreado = 0; consumCreado < C; consumCreado++){
		pid = fork();
		if(pid == 0){break;}
		else if(pid == -1){perror("Error al crear el consumidor: ");exit(EXIT_FAILURE);}
	}

	/*
	 * Si el proceso actual es un consumidor creado en el bucle, se empieza a trabajar
	 * con el bucle de consumición, este lazo es igual al de un consumidor con semáforos.
	 * Por cada elemento consumido se imprime el pid del consumidor, así sabremos quien
	 * es el consumidor de consume el producto en cada momento.
	 */
	if(pid == 0){
		int i = 0;
		while(i < (IT/C)){
			sem_wait(full);
			sem_wait(accesos);
			item = cuenta[cuenta[N]-1];
			cuenta[N]--;
			printf("Posición: %d\t",cuenta[N]);
			sem_post(accesos);
			sem_post(vacias);
			printf("consumido: %d\tconsumidor: %d\n",item,getpid());
			sleep(rand()%4);
			i++;
		}
		exit(0);
	}else{
		/*
		 * El padre debe esperar a que todos los procesos creados acaben. Cuando estos hayan
		 * finalizado su trabajo, el padre cierra la proyección, el fichero de respaldo y
		 * finalmente elimina los semáforos del kernel
		 */
		 for(int i = 0; i < C; i++){
		 	wait(&wstatus);
		 }

		if(munmap((void *)cuenta,(N+1)*sizeof(int)) == -1){
			perror("Error en munmap productor: ");
			exit(EXIT_FAILURE);
		}

		close(buffer);
		destruirSemaforos();
		printf("Ya es hora de cerrar. Me despido %d\n",getpid());
	}
	return EXIT_SUCCESS;
}

/*
 * FUNCIONES
 */
void abrirSemaforo(){
	if((vacias = sem_open("VACIAS",0)) == SEM_FAILED){
		perror("Error al abrir vacias: ");
		exit(EXIT_FAILURE);
	}
	if((full = sem_open("FULL",0)) == SEM_FAILED){
		perror("Error al abrir full");
		exit(EXIT_FAILURE);
	}

	if((accesos = sem_open("ACCESOS",0)) == SEM_FAILED){
		perror("Error al abrir accesos: ");
		exit(EXIT_FAILURE);
	}
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

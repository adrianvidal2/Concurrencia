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
#define IT 50

int *cuenta;

/*
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
	int item;
	int buffer,i = 0;

	srand(time(NULL));
	//Impresión del código del consumidor
	printf("\t\t--- Soy el consumidor, mi código es: %d ---\n\n",getpid());

	 /*
	  * Apertura del buffer y acceso a la proyección de memoria en la que se encuentra el
	  * productor con el archivo de respaldo.
	  */
	if((buffer = open("buffer.txt",O_RDWR | O_CREAT | O_APPEND,S_IRWXU | S_IRWXG | S_IRWXO)) < 0){
		perror("Error al abrir el buffer: ");
		exit(EXIT_FAILURE);
	}

	if((cuenta = (int *)mmap(NULL,(N+1)*sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED,buffer,0)) == MAP_FAILED){
		perror("Error al abrir buffer: ");
		exit(EXIT_FAILURE);
	}

	 //Apertura de los semáforos, toman el valor actual.
	abrirSemaforo();

	/*
	 * Lazo de consumición, se consume el item de la posición que marca la última posición
	 * del archivo. Anteriormente, se decrementa los semáforos full y accesos, provocando
	 * así un bloqueo de la región crítica, imposibilitando la entrada a esta por parte
	 * de otro proceso. Finalmente, dormimos el proceso un tiempo aleatorio entre 0 y 3.
	 */
	while(i < IT){
		sem_wait(full);
		sem_wait(accesos);
		item = cuenta[cuenta[N]-1];
		cuenta[N]--;
		sem_post(accesos);
		sem_post(vacias);
		printf("consumido: %d\tposición: %d\n",item,cuenta[N]);
		sleep(rand()%4);
		i++;
	}

	//Cierre de la proyección de memoria, cierre del archivo y eliminación de los semáforos del kernel.
	if(munmap((void *)cuenta, (N+1)*sizeof(int)) == -1){
		perror("Error en munmap consumidor: ");
		destruirSemaforos();
		exit(EXIT_FAILURE);
	}

	close(buffer);
	destruirSemaforos();
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

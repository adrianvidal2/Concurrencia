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
/*
 * Creación de los semáforos que se usarán en el programa
 * accesos: Controla los accesos a la región crítica del programa.
 * vacias: Cuenta cuantas posiciones se encuentran libres para la introducción de items.
 * full: Cuenta las posiciones ocupadas en el buffer, contienen items.
 */
sem_t *accesos;
sem_t *vacias;
sem_t *full;

int *cuenta;

/*CABECERAS DE FUNCIONES*/
// Función encargada de la creación de un número aleatorio entre 40 y 122. Retorna dicho número
int produce_item();
//Función encargada de crear los semáforos.
void crearSemaforo();
//Función encargada del cierre de los semáforos.
void cerrarSemaforos();
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
	//Impresión del código del productor y eliminación de los semáforos por si estos seguían en ejecución
	printf("\t\t--- Soy el productor, mi código es: %d ---\n",getpid());
	destruirSemaforos();

	//Apertura del archivo que se usará como respaldo en la proyección, a continuación se le atribuyé el tamaño del buffer +1, ya que se
	//almacenará el valor de la cuenta. Finalmente se abre la proyección de memoria.
	if((buffer = open("buffer.txt",O_RDWR | O_CREAT | O_APPEND,S_IRWXU | S_IRWXG | S_IRWXO)) < 0){
		perror("Error creando el buffer: ");
		exit(EXIT_FAILURE);
	}

	if(ftruncate(buffer,(N+1)*sizeof(int)) == -1){
		perror("Error truncando el archivo: ");
		exit(EXIT_FAILURE);
	}

	if((cuenta = (int *)mmap(NULL,(N+1)*sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED,buffer,0)) == MAP_FAILED){
		perror("Error en mmap productor: ");
		exit(EXIT_FAILURE);
	}

	cuenta[N] = 0;
	//Creación de los 3 semáforos con sus valores iniciales.
	crearSemaforo();

	/*
	 * Lazo de producción, se produce un item de manera aleatoria, luego se duerme un tiempo
	 * aleatorio entre 0 y 3. Cuando este se despierta, decrementa el semáforo
	 * vacias, quedando un hueco menos y accesos (si vale 0 se bloquea el proceso).
	 * Posteriormente se entra en la región crítica y se introduce el item e incrementa
	 * el número de elementos, tanto de la cuenta como de los semáforos.
	 */
	while(i < IT){
		item = produce_item();
		sleep(rand()%4);
		sem_wait(vacias);
		sem_wait(accesos);
		cuenta[cuenta[N]] = item;
		cuenta[N]++;
		printf("elemento: %d\t posición: %d\n",item,cuenta[N]-1);
		sem_post(accesos);
		sem_post(full);
		i++;
	}

	//Cierre de la proyección de memoria, del archivo y finalmente de los 3 semáforos utilizados (no se eliminan del kernel)
	if(munmap((void *)cuenta,(N+1)*sizeof(int)) == -1){
		perror("Error en munmap productor: ");
		cerrarSemaforos(),
		exit(EXIT_FAILURE);
	}

	close(buffer);
	cerrarSemaforos();
	return EXIT_SUCCESS;
}

/*
 * Funciones
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

void cerrarSemaforos(){
	sem_close(vacias);
	sem_close(full);
	sem_close(accesos);
}

void destruirSemaforos(){
	sem_close(vacias);
	sem_close(full);
	sem_close(accesos);
	sem_unlink("VACIAS");
	sem_unlink("FULL") ;
	sem_unlink("ACCESOS");
}

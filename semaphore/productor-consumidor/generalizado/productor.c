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
 * P: número de productores que se crearán
 * IT: número de items que se podrán crear
 */
#define N 8
#define P 5
#define IT 50

int *cuenta;
//Definimos los semáforos
sem_t *accesos;
sem_t *vacias;
sem_t *full;


/*CABECERAS DE FUNCIONES*/
/*
 * Función encargada de la creación de un número aleatorio entre 40 y 122
 * Retorna dicho número.
 */
int produce_item();
/*
 * Función encargada de la apertura de los 3 semáforos correspondientes
 * vacias: semáforo que cuenta las posiciones libres del buffer que quedan
 * full: semáforo que cuenta el número de espacios ocupados en el buffer
 * accesos: semáforo que controla el acceso a la región crítica del programa
 */
void crearSemaforo();
/*
 * Función encargada del cierre de los semáforos creados con la función anterior
 */
void cerrarSemaforos();
/*
 * Función encargada del cierre y eliminación del kernel de los semáforos.
 * Esta función se ejecutará al comienzo del programa por si algunos de los semáforos no ha
 * sido eliminado correctamente.
 */
void destruirSemaforos();



int main(int argc, char **argv) {
	int item;
	int buffer,procCreado,wstatus;
	pid_t pid;

	srand(time(NULL));
	//Impresión del código del productor y eliminación de los semáforos por si estos seguían en ejecución
	printf("\t\t--- Soy el encargado, mi código es: %d --- \n",getpid());
	destruirSemaforos();

	//Apertura del archivo que se usará como respaldo en la proyección, a continuación se le atribuyé el tamaño del buffer +1, ya que se
	//almacenará el valor de la cuenta. Finalmente se abre la proyección de memoria.
	if((buffer = open("buffer.txt",O_RDWR | O_CREAT | O_APPEND,S_IRWXU | S_IRWXG | S_IRWXO)) < 0){
		perror("Error creando el buffer: ");
		exit(EXIT_FAILURE);
	}

	if(ftruncate(buffer,(N+1)*sizeof(int)) == -1){
		perror("Error truncando el buffer: ");
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
	 * Con el siguiente bucle for, se crea los procesos hijo que serán los productores
	 * como no queremos que los nuevos procesos creen más procesos, se el pid == 0, salimos
	 * del bucle.
	 */
	for(procCreado = 0; procCreado < P; procCreado++){
		pid = fork();
		if(pid == 0){ break;}
		else if(pid == -1){perror("Error al crear el productor: "); exit(EXIT_FAILURE); break;}
	}

	/*
	 * Si el proceso actual es un proceso creado en el bucle anterior, lanzamos el lazo de
	 * producción, el lazo es igual al lazo de un productor con semáforos. En este caso
	 * repartiremos los items a producir entre los productores. Por cada elemento se
	 * imprime el pid del productor, así sabremos cuantos hace cada proceso.
	 */
	if(pid == 0){
		int i = 0;
		while(i < (IT/P)){
			item = produce_item();
			sleep(rand()%4);
			sem_wait(vacias);
			sem_wait(accesos);
			cuenta[cuenta[N]] = item;
			printf("elemento: %d\t posicion: %d\t productor: %d\n",item,cuenta[N],getpid());
			cuenta[N]++;
			sem_post(accesos);
			sem_post(full);
			i++;
		}
		exit(0);
	}else{
		/*
		 * El padre debe esperar por todos los hijos antes de ejecutarse, una vez estos han
		 * acabado su trabajo, el padre cierra la proyección de memoria, el archivo de
		 * respaldo y cierra los semáforos.
		 */
		 for(int i = 0; i < P; i++){
		 	wait(&wstatus);
		 }

		if(munmap((void *)cuenta,(N+1)*sizeof(int)) == -1){
				perror("Error en munmap productor: ");
				exit(EXIT_FAILURE);
			}

			close(buffer);
			cerrarSemaforos();
			printf("Ya es hora de cerrar. Me despido %d\n",getpid());
	}


	return EXIT_SUCCESS;
}

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
}

void cerrarSemaforos(){
	sem_close(vacias);
	sem_close(full);
	sem_close(accesos);
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

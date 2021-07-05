#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

/*Constantes que determinan los valores de los objetos a utilizar*/
#define USADO 0
#define PAPEL 1
#define TABACO 2
#define CERILLA 3


int mesa[2];		//Ingredientes que se encuentran en la mesa
int disponibles;	//Número de elementos posibles a consumir
int poseer[3];		//Elemento que posee cada fumador

	/*SEMAPHORE*/
/*
 * *fum1, *fum2, *fum3: Semáforos para cada uno de los fumadores.
 *
 *	*accesos: Este semáforo solo tomará valor 0 y 1 (mutex), será
 *			  el encargado del control a la región crítica del
 *			  programa.
 *
 *	*agent: Semáforo para el agente.
 */
sem_t *fum1,*fum2,*fum3,*accesos,*agent;

	/*ESTRUCTURA FUMADOR*/
/*Esta estructura permitirá determinar los elementos para poder así
 * describir en cada momento a un fumador.
 *
 * int ingrediente: indica cual de los posibles ingredientes posee
 * 					el fumador.
 *
 * int num: determina el número del fumador para poder asociarlo a un
 * 			semáforo.
 */
typedef struct{
	int ingrediente;
	int num;
}fumador;

	/*HEADERS*/

//Funciones para los ingredientes

/*
 * Función encargada de ordenar los ingredientes de menor a
 * mayor cuando estos son dispuestos en la mesa
 *
 * int *produce: array de los ingredientes producidos a ordenar.
 */
void ordenar(int *produce);

/*
 * Función encargada del reparto aleatorio de los ingredientes
 * entre los tres fumadores.
 *
 * fumador *fum: colección del tipo fumador que almacenará los
 * 				 fumadores disponibles.
 */
void initFumadores(fumador *fum);

/*
 * Función encarga de la producción de diferentes ingredientes
 *
 * int *produce: array de los ingredientes producidos
 * int numIng: número de elementos a producir
 */
void produce_ingre(int *produce, int numIng);

//Funciones para el comportamiento de los semáforos

/*
 * Función encargada de la creación de los diferentes semáforos
 * necesarios para la ejecución del programa.
 */
void initSem();

/*
 * Función que realiza el cierre y eliminación de los semáforos
 * utilizados en el programa
 */
void destroySem();

//Menu
void menu();

/*Función asociada al hilo del agente*/
void *agente(void *arg);

//Función para los 3 fumadores

/*
 * Función que realiza la eliminación de los ingredientes que se
 * encuentran encima de la mesa para la preparación del cigarrillo
 *
 * fumador fum: variable que determina el fumador que tiene permitido
 * 				seleccionar los ingredientes para liar un cigarro.
 */
void liar(fumador fum);

/*
 * Función que detiene el programa 4 segundos para que el fumador
 * fume el cigarro preparado con anterioridad.
 *
 * fumador fum: variable que determina el fumador que va a fumar.
 */
void fumar(fumador fum);

/*Función asociada a los hilos de los fumadores*/
void *smoker(void *arg);


int main(int argc, char **argv) {
	//Hilos para agente y los tres fumadores
	pthread_t vendedor;
	pthread_t smokers[3];

	disponibles = 0;

	//Lista de estructuras tipo fumador para obtener la información de este.
	fumador fumadores[3];

	//Se fija la semilla para poder generar ingredientes aleatoriamente
	srand(time(NULL));

	//Destrucción de los semáforos ante una posible existencia de una ejecución anterior
	destroySem();

	//Apertura de los semáforos
	initSem();

	//Inicialización de los ingredientes a utilizar y de los 3 fumadores con sus respectivos ingredientes.
	mesa[0] = 0;
	mesa[1] = 0;
	initFumadores(fumadores);

	//Se muestra la información de cada fumador por pantalla
	menu(fumadores);

	//Creación de los hilos y determinación de su comportamiento
	for(int i = 0; i < 3; i++){
		pthread_create(&smokers[i], NULL, smoker, (void*)&fumadores[i]);
	}

	pthread_create(&vendedor,NULL, agente, NULL);

	//Espera de hilos
	pthread_join(vendedor, NULL);

	for(int i = 0; i < 3; i++){
		pthread_join(smokers[i], NULL);
	}


	//Destrucción de los semáforos y liberación de memoria de la lista de ingredientes
	destroySem();

	printf("Se han acabado los ingredientes\n");
	return EXIT_SUCCESS;
}


void initFumadores(fumador *fum){
	//Producción de los ingredientes para cada fumador
	int reparto[4];
	produce_ingre(reparto, 3);

	//inicialización del fumador 1
	fum[0].num = 1;
	fum[0].ingrediente = poseer[0] = reparto[0];

	//inicialización del fumador 2
	fum[1].num = 2;
	fum[1].ingrediente = poseer[1] = reparto[1];

	//inicialización del fumador 3
	fum[2].num = 3;
	fum[2].ingrediente = poseer[2] = reparto[2];
}

void initSem(){
	//Creación semáforo agente
	if((agent = sem_open("AGENTE", O_CREAT, 0700,1)) == SEM_FAILED){
		perror("Error sem_open agent: ");
		exit(EXIT_FAILURE);
	}

	//Creación semáforo acceso a región crítica
	if((accesos = sem_open("ACCESOS", O_CREAT, 0700, 1)) == SEM_FAILED){
		perror("Error sem_open accesos: ");
		exit(EXIT_FAILURE);
	}

	//Creación semáforos para los fumadores
	if((fum1 = sem_open("FUMA1", O_CREAT, 0700, 0)) == SEM_FAILED){
		perror("Error sem_open fum1: ");
		exit(EXIT_FAILURE);
	}

	if((fum2 = sem_open("FUMA2", O_CREAT, 0700,0)) == SEM_FAILED){
		perror("Error sem_open fum2: ");
		exit(EXIT_FAILURE);
	}

	if((fum3 = sem_open("FUMA3", O_CREAT, 0700, 0)) == SEM_FAILED){
		perror("Error sem_open fum3: ");
		exit(EXIT_FAILURE);
	}
}

void destroySem(){
	//Cierre de los semáforos
	sem_close(agent);
	sem_close(accesos);
	sem_close(fum1);
	sem_close(fum2);
	sem_close(fum3);

	//Destrucción de los semáforos
	sem_unlink("AGENTE");
	sem_unlink("ACCESOS");
	sem_unlink("FUMA1");
	sem_unlink("FUMA2");
	sem_unlink("FUMA3");
}

void menu(fumador *fumadores){
	char tiene[8];
	printf("==========================================\n");

	for(int i = 0; i < 3; i++){
		if(fumadores[i].ingrediente == 1) strcpy(tiene, "Papel");
		if(fumadores[i].ingrediente == 2) strcpy(tiene,"Tabaco");
		if(fumadores[i].ingrediente == 3) strcpy(tiene, "Cerilla");

		printf("||\tFumador: %d\tTiene: %s\t||\n",fumadores[i].num, tiene);
	}

	printf("==========================================\n");
}

void produce_ingre(int *produce, int numIng){
	//lista de número usados al generar aleatoria.
	int usado[] = {0,0,0};
	int n = 0, random;

	while(n < numIng){
		//Se genera un número aleatorio entre 0 y 2
		random = rand() % 3;
		//Si la posición de usado es 0 se introduce el número
		if(!usado[random]){
//			printf("RANDOM: %d\n", random+1);
			produce[n] = random+1;
			usado[random] = 1;
			n++;
		}
	}



}

void liar(fumador fum){

	//Se selecciona los ingredientes de la mesa
	if(mesa[0] == TABACO &&
			mesa[1] == CERILLA &&
			fum.ingrediente == PAPEL){
		printf("Fumador %d apoderándose de TABACO y CERILLA\n", fum.num);
	}else if(mesa[0] == PAPEL &&
			mesa[1] == CERILLA &&
			fum.ingrediente == TABACO){
		printf("Fumador %d apoderándose de PAPEL y CERILLA\n", fum.num);
	}else if(mesa[0] == PAPEL &&
			mesa[1] == TABACO &&
			fum.ingrediente == CERILLA){
		printf("Fumador %d apoderándose de PAPEL y TABACO\n",fum.num);
	}

	//El fumador lia su cigarro
	printf("El fumador %d está liando un cigarro\n", fum.num);

	//Eliminación de los ingredientes de encima de la mesa
	mesa[0] = USADO; mesa[1] = USADO;
}

void fumar(fumador fum){
	printf("El fumador %d está fumando\n", fum.num);
	sleep(4);
}

void ordenar(int *produce){
	int auxiliar, ing1, ing2;
	ing1 = produce[0];
	ing2 = produce[1];

	if(ing1 > ing2){
		auxiliar = ing1;
		produce[0] = ing2;
		produce[1] = auxiliar;
	}
}


void *agente(void *arg){ // @suppress("No return")
	int producidos[2];		//Elementos que el agente colocará en la mesa
	int consumidos = 0, ing1, ing2;

	while(consumidos < 10){
		//Bloqueo del semáforo del agente
		sem_wait(agent);

		//Se producen los elementos y se ordenan (PAPEL,TABACO, CERILLA)
		produce_ingre(producidos,2);
		ordenar(producidos);
		ing1 = producidos[0]; ing2 = producidos[1];

		//Elección de fumador que podrá liar y fumar un cigarro
		if(ing1 == 2 && ing2 == 3){
			//Se permite la escritura en la región crítica por parte del agente
			sem_wait(accesos);
			printf("El agente pone sobre la mesa: TABACO y CERILLA\n");
			sem_post(accesos);

			/*Selección del fumador para la consumición del cigarro.
			 * Se comprueba quién posee el ingrediente necesario.
			 */
			if(poseer[0] == PAPEL) sem_post(fum1);
			else if(poseer[0] == PAPEL) sem_post(fum2);
			else sem_post(fum3);

		}else if(ing1 == 1 && ing2 == 3){
			sem_wait(accesos);
			printf("El agente pone obre la mesa: PAPEL y CERILLA\n");
			sem_post(accesos);

			if(poseer[0] == TABACO) sem_post(fum1);
			else if(poseer[1] == TABACO) sem_post(fum2);
			else sem_post(fum3);

		}else if(ing1 == 1 && ing2 == 2){
			sem_wait(accesos);
			printf("El agente pone sobre la mesa: PAPEL y TABACO\n");
			sem_post(accesos);

			if(poseer[0] == CERILLA) sem_post(fum1);
			else if(poseer[1] == CERILLA) sem_post(fum2);
			else sem_post(fum3);
		}
		consumidos = consumidos + 2;
	}

	pthread_exit(NULL);
}

void *smoker(void *arg){ // @suppress("No return")
	fumador *fuma;
	fuma = (fumador *)arg;

	//Para la detención del programa se consumirá 5 veces
	while(disponibles < 10){
		//Bloqueo de los semáforos de cada fumador
		if(fuma->num == 1) sem_wait(fum1);
		if(fuma->num == 2) sem_wait(fum2);
		if(fuma->num == 3) sem_wait(fum3);

		if(disponibles < 10){
			//Se permite el acceso a la región crítica
			sem_wait(accesos);
			//Lia el cigarro y modifica la variable compartida
			liar(*fuma);
			//Se cede el acceso a la región crítica
			sem_post(accesos);
			//Fuma el cigarro
			fumar(*fuma);
			//Actualiza la disponibilidad de los ingredientes
			disponibles = disponibles + 2;
			//Al acabar se liberan todos los semáforos, evitando así bloqueos posteriores
			if(disponibles == 10){ sem_post(fum1); sem_post(fum2); sem_post(fum3);}
			printf("\n\n\n");
			//Aviso al agente de finalización con los ingredientes actuales.
			sem_post(agent);
		}

	}

	pthread_exit(NULL);
}

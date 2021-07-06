#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

int N ;							//Número de filósofos
#define IZQUIERDA	(posFilo+N-1)%N			//Número del vecino izquierdo de i
#define DERECHA (posFilo+1)%N				//Número del vecino derecho de i
#define PENSANDO 0					//El filósofo está pensando
#define HAMBRIENTO 1					//El filósofo está hambriento
#define COMIENDO 2					//El filósofo está comiendo
#define MAX_TIEMPO_PENSAR 4				//Tiempo máximo que un filósofo estará pensando

/*Colores para los diferentes estados de los filósofos*/
#define VERDE "\x1b[32m"
#define ROJO "\x1b[31m"
#define RESET "\x1b[0m"
#define AMARILLO "\x1b[33m"



int *estados;						//Estado de los filósofos

pthread_mutex_t el_mutex;
pthread_cond_t *condFilosofo;

typedef struct{
	int posicion;
}hilos;


void inicializarVariables();
void destruirVariables();
void pensar();
void probar(int posFilo);
void tomar_tenedores(int posFilo);
void poner_tenedores(int posFilo);
void imprimeEstados();
void *filosofo(void *arg);

int main(int argc, char **argv) {
	printf("PROBLEMA DE LOS FILÓSOFOS UTILIZANDO MUTEXES Y VARIABLES DE CONDICIÓN\n");
	printf("Introduzca el número de filósofos: ");
	scanf("%d",&N);

	if(N > 0){
		pthread_t filo[N];
		hilos posicion[N];

		//Reserva de memoria para el número de filósofos introducido
		if(((condFilosofo = (pthread_cond_t *)malloc(N*sizeof(pthread_cond_t))) == NULL) ||
				((estados = (int *)malloc(N*sizeof(int))) == NULL)){
			perror("Error en malloc: ");
			exit(EXIT_FAILURE);
		}

		inicializarVariables();

		//Creación de los diferentes filósofos y su comportamiento
		for(int i = 0; i < N; i++){
			posicion[i].posicion = i;
			if(pthread_create(&filo[i], NULL, filosofo, &posicion[i]) != 0){
				exit(EXIT_FAILURE);
			}
		}

		//Espera por todos los filósofos lanzados con anterioridad
		for(int i = 0; i < N; i++){
			pthread_join(filo[i], 0);
		}



		destruirVariables();

	}else{
		printf("Introduzca un número positivo de filósofos\n");
	}

	return EXIT_SUCCESS;
}

void inicializarVariables(){
	//Inicializamos el mutex
	if(pthread_mutex_init(&el_mutex, 0) != 0){
		perror("Error creando el mutex: ");
		exit(EXIT_FAILURE);
	}

	//Inicializamos las variables de condición y comprobamos los posibles errores
	for(int i = 0; i < N; i++){
		if(pthread_cond_init(&condFilosofo[i], 0) != 0){
			perror("Error creando las variables de condición: ");
			exit(EXIT_FAILURE);
		}
	}

	//Inicialmente los filósofos se encuentran en estado pensativo (0)
	for(int i = 0; i < N; i++)estados[i] = PENSANDO;

	printf("Bien inicializadas\n");
}

void destruirVariables(){
	//Destrucción de las variables de condición
	for(int i = 0; i < N; i++){
		if(pthread_cond_destroy(&condFilosofo[i]) != 0){
			perror("Error al destruír las variables de condición: ");
			exit(EXIT_FAILURE);
		}
	}

	//Destrucción del mutex

	if(pthread_mutex_destroy(&el_mutex) != 0){
		perror("Error al destruir el mutex: ");
		exit(EXIT_FAILURE);
	}

	//liberamos la memoria reservados para los estados y para las variables de condición
	free(condFilosofo);
	free(estados);
	printf("Bien destruídas\n");
}
void pensar(){
	sleep(rand()%MAX_TIEMPO_PENSAR);
}

void imprimeEstados(){
	printf("\t\tEstados: ");
	for(int i = 0; i < N; i++){
		printf("%d\t",estados[i]);
	}
	printf("\n");
}
void probar(int posFilo){
	//Se comprueba si el filósofo puede comer, se asegura que sus vecinos no están ocupando sus tenedores
	if(estados[posFilo] == HAMBRIENTO && estados[IZQUIERDA] != COMIENDO && estados[DERECHA] != COMIENDO){
		printf(VERDE"\t\tSoy el filósofo número %d y me encuentro comiendo\n"RESET,posFilo);

		//Se cambia el estado a comiendo
		estados[posFilo] = COMIENDO;
		if(pthread_cond_signal(&condFilosofo[posFilo]) != 0){
			perror("Error enviando señal: ");
			exit(EXIT_FAILURE);
		}
		imprimeEstados();
	}
}

void tomar_tenedores(int posFilo){
	//Como se entra en la región crítica se bloquea el mutex para evitar carreras críticas
	if(pthread_mutex_lock(&el_mutex) != 0){
		perror("Error al bloquear el mutex: ");
		exit(EXIT_FAILURE);
	}

	estados[posFilo] = HAMBRIENTO;	//Se cambia el estado del filósofo a hambriento

	probar(posFilo);

	while(estados[posFilo] != COMIENDO){
		//Si el filósofo no se encuentra comiendo se bloquea la variable de condición
		if(pthread_cond_wait(&condFilosofo[posFilo], &el_mutex) != 0){
			perror("Error al bloquear la variable de condición: ");
			exit(EXIT_FAILURE);
		}
	}

	//Se libera la región crítica
	if(pthread_mutex_unlock(&el_mutex) != 0){
		perror("Error al desbloquear el mutex: ");
		exit(EXIT_FAILURE);
	}


}

void poner_tenedores(int posFilo){
	//Como se entra en la región crítica se bloquea el mutex para evitar carreras críticas
	if(pthread_mutex_lock(&el_mutex) != 0){
		perror("Error al bloquear el mutex: ");
		exit(EXIT_FAILURE);
	}

	estados[posFilo] = PENSANDO;

	probar(IZQUIERDA);	//Se verifica si el vecino de la izquierda puede comer
	probar(DERECHA);	//Se verifica si el vecino de la derecha puede comer

	//Se libera la región crítica
	if(pthread_mutex_unlock(&el_mutex) != 0){
		perror("Error al desbloquear el mutex: ");
		exit(EXIT_FAILURE);
	}
}

void *filosofo(void *arg){
	hilos *filosofo = (hilos *)arg;

	srand(time(NULL));
	while(1){
		printf(AMARILLO"Soy el filósofo número %d, me encuentro pensando\n"RESET,filosofo->posicion);
		pensar();
		printf("Soy el filósofo número %d y voy a intentar coger los tenedores\n",filosofo->posicion);
		tomar_tenedores(filosofo->posicion);
		sleep(rand()%MAX_TIEMPO_PENSAR);
		printf(ROJO"Soy el filósofo número %d, dejo los tenedores\n"RESET,filosofo->posicion);
		poner_tenedores(filosofo->posicion);
	}
	pthread_exit(0);
}

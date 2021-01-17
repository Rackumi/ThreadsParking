#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define tiempoEsperaParking 5
#define tiempoEsperaFuera 10
#define radio 0.4

//compilar con gcc parking.c -lpthread -o x
//ejecutar con ./x

int * aparcamiento;

int plazasLibres;
int numPlazas;
int numPlantas;
int numCoches;
int numCamiones;

pthread_mutex_t mutex;

pthread_cond_t condicion_coche;
pthread_cond_t condicion_camion;

void imprimirParking(){
    printf("Parking:");
    int plantaActual = 1;
    for (int i = 0; i < numPlazas*numPlantas; i++) {
        printf(" [%03d] ", aparcamiento[i]);
        if ((i + 1)/plantaActual == numPlazas) {
            plantaActual++;
            if(i+1 != numPlantas*numPlazas){
                printf("\n        ");
            }else{
                printf("\n");
            }
        }
    }
    return;
}

int getPlazaLibreCoche(){//Devuelve el indice de la primera plaza de coche libre que encuentra
    for (int i = 0; i < numPlazas*numPlantas; i++) {
        if (aparcamiento[i] == 0) {
            return i; //devuelve i
        }
    }
    return -1;
}

int getPlazaLibreCamion(){//Mismo que getPlazaLibreCoche() pero con 2 plazas contuguas
    for (int i = 0; i < numPlazas*numPlantas - 1; i++) {
        if (aparcamiento[i] == 0 && aparcamiento[i + 1] == 0 && (i+1)%numPlazas!=0 && (i+1 > 0) && (i+1<numPlazas) && (i<numPlazas)) {
            return i; //devuelve i y su contigua
        }
    }
    return -1;
}

float getRatio(){ //devolverá la proporcion de camiones respecto a coches
    float coches = 0.0;
    float camiones = 0.0;
    for(int i= 0;  i < numPlazas*numPlantas; i++) {
        if(aparcamiento[i]!=0){
            if(aparcamiento[i]<=100){
                coches = coches+1;
            }else{
                camiones = camiones+1;
            }
        }
    }
    if(camiones==0.0){
        return -1;
    }
//    return coches/(camiones+0.25);
    return camiones/coches;
}

void *coche(void* num){
    int coche_id = *(int *)num;

    while(1){
        //Seccion critica1
        pthread_mutex_lock(&mutex);

        while(getPlazaLibreCoche()==-1){
            pthread_cond_wait(&condicion_coche,&mutex);
        }

        int free = getPlazaLibreCoche();
        aparcamiento[free] = coche_id; //plaza cogida
        plazasLibres--;
        printf("ENTRADA: Coche %d aparca en %d. Plazas libres: %d \n",coche_id,free,plazasLibres); //imprimir texto
        imprimirParking(); //imprime parking

        pthread_mutex_unlock(&mutex);
        //Fin seccion critica1

        sleep((rand()% 2) * tiempoEsperaParking);// duermes en el parking

        //Seccion critica2
        pthread_mutex_lock(&mutex);

        aparcamiento[free] = 0;
        plazasLibres++;

        if(getRatio()<radio){
            pthread_cond_signal(&condicion_camion);
        }
        else{
            pthread_cond_signal(&condicion_coche);
        }

        printf("SALIDA: Coche %d saliendo. Plazas libres: %d \n",coche_id,plazasLibres);

        pthread_mutex_unlock(&mutex); //suelto plaza
        //Fin seccion critica2

        sleep((rand()% 2)* tiempoEsperaFuera); // Esperamos fuera
    }
}

void *camion(void* num){
    int camion_id = *(int *)num;

    while(1){
        //Seccion critica1
        pthread_mutex_lock(&mutex);

        while(getPlazaLibreCamion()==-1){
            pthread_cond_wait(&condicion_camion,&mutex);
        }

        int free = getPlazaLibreCamion();
        aparcamiento[free] = camion_id; //plaza cogida
        aparcamiento[free+1] = camion_id;
        plazasLibres -= 2;
        printf("ENTRADA: Camion %d aparca en %d. Plazas libres: %d \n",camion_id,free,plazasLibres); //imprimir texto
        imprimirParking(); //imprime parking

        pthread_mutex_unlock(&mutex);
        //Fin seccion critica1

        sleep((rand()% 2) * tiempoEsperaParking);// duermes en el parking

        //Seccion critica2
        pthread_mutex_lock(&mutex);

        aparcamiento[free] = 0;
        aparcamiento[free+1] = 0;
        plazasLibres += 2;

        if(getRatio()<radio){
            pthread_cond_signal(&condicion_camion);
        }
        else{
            pthread_cond_signal(&condicion_coche);
        }

        printf("SALIDA: Camion %d saliendo. Plazas libres: %d \n",camion_id,plazasLibres);

        pthread_mutex_unlock(&mutex); //suelto plaza
        //Fin seccion critica2

        sleep((rand()% 2)* tiempoEsperaFuera); // Esperamos fuera
    }
}

int main(int argc, char *argv[]){

    if(argc<3 || argc>5){//Comprobaciion del numero de argumentos de entrada
        printf("Uso: %s. Error. La entrada del programa tiene que estar entre 2 y 4 argumentos,\n"
               "    siguiendo el esquema de (nº Plazas)(nº Plantas)(nº Coches)(nº Camiones). \n"
               "    En caso de no incluir alguno o ambos de los ultimos parametros, se tomaran como 0. \n", argv[0]);
        return 1;
    }

    numPlazas = atoi(argv[1]);
    numPlantas = atoi(argv[2]);

    if(argc==3){ //2 argumentos -> plazas(x) | plantas(x) | coches(plazas*plantas*2) | camiones(0)
        numCoches = numPlantas*numPlazas*2;
        numCamiones = 0;
    }
    if(argc==4){ //3 argumentos -> plazas(x) | plantas(x) | coches(x) | camiones(0)
        numCoches = atoi(argv[3]);
        numCamiones = 0;
    }
    if(argc==5){ //4 argumentos -> plazas(x) | plantas(x) | coches(x) | camiones(x)
        numCoches = atoi(argv[3]);
        numCamiones = atoi(argv[4]);
    }

    if(numPlazas<1){//Comprobacion de plazas
        printf("Número de plazas no válido\n");
        return 1;
    }
    if(numPlantas<1){//Comprobacion de plantas
        printf("Número de plantas no válido\n");
        return 1;
    }
    if(numCoches<1 || numCoches>100){//Comprobacion de coches para que no se mezclen con camiones
        printf("Número de coches no válido\n");
        return 1;
    }
    if(numCamiones<0){//Comprobacion de camiones
        printf("Número de camiones no válido\n");
        return 1;
    }

    int i;
    aparcamiento = malloc(sizeof(int)*numPlazas*numPlantas);
    plazasLibres = numPlazas*numPlantas;
    int coche_id[numCoches];
    int camion_id[numCamiones];

    for(i=0; i<numPlazas*numPlantas;i++){
        aparcamiento[i] = 0;
    }

    pthread_t th; //creamos thread id

    pthread_mutex_init(&mutex,NULL); //creamos el mutex de las plazas

    for(i=0; i < numCoches; i++) {
        coche_id[i] = i+1;
        pthread_create(&th,NULL,coche,(void*)&coche_id[i]); //creamos thread de los coches
    }

    for(i=0; i < numCamiones; i++) {
        camion_id[i] = i+101;
        pthread_create(&th,NULL,camion,(void*)&camion_id[i]); //creamos thread de los camiones
    }

    while(1); //para que no termine hasta que nosotros queramos
}
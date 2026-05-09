#include <stdio.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <malloc.h>


typedef  struct {
    int rodzaj;
    int gotowe;
} Zamowienie;

//reprezentacja beczek


typedef struct {
    int beczki;                  // początkowo 2
    pthread_mutex_t mutex;
    pthread_cond_t available;    // sygnalizuje: zwolniła się beczka
} wine_t;

typedef struct {
    Zamowienie* queue[100];

    int front;
    int rear;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;

} OrderQueue;
typedef struct {

    int klient_id;
    OrderQueue* queue;

} ClientArgs;

typedef struct {
    OrderQueue* zam;
    wine_t* wines;
} KelnerArgs;

void* klient_fun (void* args){

    ClientArgs* data = (ClientArgs*)args;

    for (int i = 0; i < 3; i++) {

        //wymyślanie zamówienia

        Zamowienie* z = malloc(sizeof(Zamowienie));

        z->rodzaj = rand() % 3;
        z->gotowe = 0;

        printf("Klient %d zamawia wino %d\n",
               data->klient_id,
               z->rodzaj);

        pthread_mutex_lock(&data->queue->mutex);

        data->queue->queue[data->queue->rear] = z;

        data->queue->rear++;

        pthread_cond_signal(&data->queue->not_empty);

        pthread_mutex_unlock(&data->queue->mutex);

        while (z->gotowe == 0);

        printf("Klient %d dostał wino %d\n",
               data->klient_id,
               z->rodzaj);

        //zwalnianie pamięci wykorzystanych zamówień
        free(z);
    }

    printf("Klient %d wychodzi\n", data->klient_id);

    return NULL;
    }


void* kelner_fun(void* arg) {

    KelnerArgs* data = (KelnerArgs*)arg;

    Zamowienie* z = data->zam;
    OrderQueue* queue = data->zam;

    while (1) {

        pthread_mutex_lock(&queue->mutex);

        // czekaj aż pojawi się zamówienie
        while (queue->front == queue->rear) {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }

        // pobranie zamówienia
        Zamowienie* z = queue->queue[queue->front];

        queue->front++;

        pthread_mutex_unlock(&queue->mutex);

        // wybór odpowiedniego wina
        wine_t* wine = &data->wines[z->rodzaj];



        pthread_mutex_lock(&wine->mutex);

        while (wine->beczki == 0) {
            pthread_cond_wait(&wine->available, &wine->mutex);
        }

        wine->beczki--;

        pthread_mutex_unlock(&wine->mutex);



        printf("Kelner nalewa wino rodzaju %d\n", z->rodzaj);

        z->gotowe = 1;



        pthread_mutex_lock(&wine->mutex);

        wine->beczki++;

        pthread_cond_signal(&wine->available);

        pthread_mutex_unlock(&wine->mutex);
    }

    return NULL;
}


int main() {

    wine_t wines[3];

    for (int i = 0; i < 3; i++) {
        wines[i].beczki = 2;
        pthread_mutex_init(&wines[i].mutex, NULL);
        pthread_cond_init(&wines[i].available, NULL);
    }


    pthread_t kelner[3];

    pthread_t klient[10];


    OrderQueue oq;
    oq.front = 0;
    oq.rear = 0;

    ClientArgs client_args[10];

    for(int i = 0; i < 10; i++){
        client_args[i].klient_id = i;
        client_args[i].queue = &oq;

        pthread_create(&klient[i], NULL,
                       klient_fun,
                       &client_args[i]);

    }

    KelnerArgs kelner_args[3];

    for (int i = 0; i < 3; i++){
        kelner_args[i].zam = &oq;
        kelner_args[i].wines = wines;

        pthread_create(&kelner[i], NULL,
                       kelner_fun,
                       &kelner_args[i]);
    }

    pthread_mutex_init(&oq.mutex, NULL);
    pthread_cond_init(&oq.not_empty, NULL);

    for (int i = 0; i < 10; i++) {
    pthread_create(&klient[i], NULL, klient_fun, &client_args[i]);
    }
    for (int i = 0; i < 3; i++) {
        pthread_create(&kelner[i], NULL, kelner_fun, &kelner_args[i]);
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(klient[i], NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(kelner[i], NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&wines[i].mutex);
        pthread_cond_destroy(&wines[i].available);
    }

    return 0;
}
//comment

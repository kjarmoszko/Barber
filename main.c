/*
 * main.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

pthread_cond_t klientDostepny = PTHREAD_COND_INITIALIZER;
pthread_cond_t fryzjerDostepny = PTHREAD_COND_INITIALIZER;
pthread_cond_t fryzjerStrzyze = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutexKrzesla = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexFryzjer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexStrzyzenie = PTHREAD_MUTEX_INITIALIZER;

//zmienne globalne
int oczekujacyKlienci = 0;
int klienci = 0;
int zrezygnowaniKlienci = 0;
int ostrzyzeniKlienci = 0;
int debug = 0;
int obslugiwany = 0;

typedef struct kolejkaStruct {
	int id;
	pthread_cond_t turn;
	struct kolejkaStruct *next;
} kolejka;

kolejka *doFryzjera = NULL;
kolejka *zrezygnowani = NULL;

#define KRZESLA 5

void fryzjer(void);
void klient(void);
void ostrzyz(kolejka *);
void czekajNaStrzyzenie(void);
void drukujKolejke(kolejka *);
kolejka *nastepnyKlient(void);
void drukujStatystyki(void);
void drukujDebug();
kolejka *nowyKlient(kolejka **, int);

int main(int argc, char *argv[]) {
	int status = 0;
	pthread_t id1, id2;
	srand(time(NULL));

	if(argc == 2) {
		if ((strncmp(argv[1], "-debug", 6) == 0)) {
			debug = 1;
		}
	}

	status = pthread_create(&id1, NULL, (void*)fryzjer, NULL);
	if(status != 0) {
		printf("Nie mozna stworzyc fryzjera\n");
		exit(1);
	}

	while(1) {
		sleep(rand() % 4);
		status = pthread_create(&id2, NULL, (void*)klient, NULL);
		if (status != 0) {
			printf("Nie mozna stworzyc klienta\n");
			exit(1);
		}
	}

	pthread_join(id1, NULL);
	return 0;
}

void fryzjer() {
	while(1) {
		pthread_mutex_lock(&mutexKrzesla);
		while (oczekujacyKlienci == 0) {
			ostrzyzeniKlienci = 0;
			pthread_cond_wait(&klientDostepny, &mutexKrzesla);
		}
		oczekujacyKlienci--;
		kolejka *klient = nastepnyKlient();
		ostrzyzeniKlienci = klient->id;
		drukujStatystyki();

		pthread_mutex_unlock(&mutexKrzesla);

		pthread_cond_signal(&klient->turn);

		ostrzyz(klient);
	}
}

void klient() {
	int id;

	pthread_mutex_lock(&mutexKrzesla);

	klienci++;
	id = klienci;

	if (oczekujacyKlienci < KRZESLA) {

		oczekujacyKlienci++;
		kolejka *knt = nowyKlient(&doFryzjera, klienci);
		drukujStatystyki();

		pthread_cond_signal(&klientDostepny);

		pthread_mutex_unlock(&mutexKrzesla);

		pthread_mutex_lock(&mutexFryzjer);
		while (ostrzyzeniKlienci != id) {
			pthread_cond_wait(&knt->turn, &mutexFryzjer);
		}
		pthread_mutex_unlock(&mutexFryzjer);
		czekajNaStrzyzenie();
	} else {

		nowyKlient(&zrezygnowani, klienci);
		zrezygnowaniKlienci++;
		drukujStatystyki();

		pthread_mutex_unlock(&mutexKrzesla);
	}
}

void ostrzyz(kolejka *klient) {
	pthread_mutex_lock(&mutexStrzyzenie);
	obslugiwany = 1;
	sleep(rand() % 5);
	obslugiwany = 0;
	pthread_cond_signal(&fryzjerStrzyze);

	pthread_mutex_unlock(&mutexStrzyzenie);
}

void czekajNaStrzyzenie() {

	pthread_mutex_lock(&mutexStrzyzenie);
	while (obslugiwany) {
		pthread_cond_wait(&fryzjerStrzyze, &mutexStrzyzenie);
	}
	pthread_mutex_unlock(&mutexStrzyzenie);
}

void drukujKolejke(kolejka *queue) {
	kolejka *tmp = NULL;
	if (queue == NULL)
		return;
	else {
		tmp = queue;
		while (tmp != NULL) {
			printf("%d ", tmp->id);
			tmp = tmp->next;
		}
	}
	printf("\n");
}

kolejka *nowyKlient(kolejka **queue, int id) {
	kolejka *new = malloc(sizeof(kolejka));
	new->id = id;
	new->next = NULL;
	pthread_cond_init(&new->turn, NULL);

	if (*queue == NULL)
		(*queue) = new;
	else {
		kolejka *tmp = *queue;
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = new;
	}
	return new;
}

kolejka *nastepnyKlient() {
	kolejka *tmp = doFryzjera;
	doFryzjera = doFryzjera->next;
	return tmp;

}

void drukujStatystyki() {
	if (ostrzyzeniKlienci != 0)
		printf("Res: %d WRoom: %d/%d [in: %d]\n", zrezygnowaniKlienci,
				oczekujacyKlienci, KRZESLA, ostrzyzeniKlienci);
	else
		printf("Res: %d WRoom: %d/%d [in: -]\n", zrezygnowaniKlienci,
				oczekujacyKlienci, KRZESLA);
	if (debug)
		drukujDebug();
}

void drukujDebug() {
	printf("Czekajacy: ");
	drukujKolejke(doFryzjera);
	printf("Zrezygnowani: ");
	drukujKolejke(zrezygnowani);
	printf("\n");
}


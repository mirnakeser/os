#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "slucajni_prosti_broj.h"

#define MASKA(bitova)			(-1 + (1<<bitova) )
#define UZMIBITOVE(broj,prvi,bitova) 	( ( broj >> (64-(prvi)) ) & MASKA(bitova) )

uint64_t MS[10], ULAZ = 0, IZLAZ = 0;
uint64_t velicina = 1;

int brojDretvi = 5;
int broj[10], ulaz[10];

int kraj = 0;

struct gmp_pomocno p;

sem_t KO, PUNI, PRAZNI;

void stavi_u_MS(uint64_t broj)
{
	MS[ULAZ] = broj;
	ULAZ = (ULAZ + 1) % 10;
	/*BROJAC++;
	if (BROJAC > 10)
	{
		BROJAC--;
		IZLAZ = (IZLAZ + 1) % 10;
	}*/
	return;
}

uint64_t uzmi_iz_MS()
{
	uint64_t broj;
	broj = MS[IZLAZ];
    IZLAZ = (IZLAZ + 1) % 10;
	/*if ( BROJAC > 0)
	{
		IZLAZ = (IZLAZ + 1) % 10;
		BROJAC--;
	}*/
	return broj;
}

void udi_u_KO (int id)
{
	int i, j, max = 0;
	for (i = 0; i < brojDretvi * 2; i++)
		if (broj[i] > max)
			max = broj[i];

	ulaz[id] = 1;
	broj[id] = max + 1;
	ulaz[id] = 0;

	for (j = 0; j < brojDretvi * 2; j++)
	{
		while ( ulaz[j] == 1 );
		while ( broj[j] != 0 && ( broj[j] < broj[id] || ( broj[j] == broj[id] && j < id ) ) );	
	}
}

void izadi_iz_KO (int id)
{
	broj[id] = 0;
}

//ideja 1 iz uputa, ali ne uspoređuju se samo po susjedna 4 bita nego svaki blok sa svakim
uint64_t zbrckanost (uint64_t x)
{
	uint64_t z = 0, i, j, k, pn1, pn2, b1, b2;

	for (i = 0; i < 15; i++)
	{
		b1 = 0;
		pn1 = UZMIBITOVE(x, 4*(i + 1), 4);
		for (j = 0; j < 4; j++)
			if (((1<<j) & pn1))
				b1++;
		for (k = i + 1; k < 16; k++)
		{
			b2 = 0;
			pn2 = UZMIBITOVE(x, 4*(k + 1), 4);
			for (j = 0; j < 4; j++)
				if (((1<<j) & pn2))
					b2++;
			if (b1 != b2)
				z++;
		}	
	}	

	return z;
}

uint64_t generiraj_dobar_broj(struct gmp_pomocno *p)

{
	uint64_t najbolji_broj = 0, i, broj, z;
	uint64_t najbolja_zbrckanost = 0;
		
	for (i = 0; i < velicina; i++)
	{
		broj = daj_novi_slucajan_prosti_broj (p);
		z = zbrckanost (broj);
		if (z > najbolja_zbrckanost)
		{
			najbolja_zbrckanost = z;
			najbolji_broj = broj;
		}
	}

	return najbolji_broj;
}

uint64_t procjeni_velicinu_grupe()
{
	uint64_t M = 1000;
	uint64_t SEKUNDI = 10;
	time_t t = time(NULL);
	uint64_t k = 0;
	uint64_t i, broj, brojeva_u_sekundi;

	while (time(NULL) < (t + SEKUNDI) )
	{
		k++;
		for (i = 0; i < M; i++)
		{
			broj = generiraj_dobar_broj(&p);
			stavi_u_MS(broj);
		}
	}

	brojeva_u_sekundi = (k * M / SEKUNDI);
	velicina = brojeva_u_sekundi / 2.5;
	
	while (ULAZ)
		uzmi_iz_MS();

	return velicina;

}

void *radnaDretva (void *id)
{
	
	int *d = id;
	uint64_t x;
	struct gmp_pomocno r;

	inicijaliziraj_generator (&r, *d);

    printf("Dretva %d pocinje.\n", d);
	while( kraj != 1 )
	{
		x = generiraj_dobar_broj(&r);

        sem_wait (&PRAZNI);
		//udi_u_KO(*d);
        sem_wait(&KO);
        printf("Dretva %d unutar K.O.\n", d);

		stavi_u_MS(x);
		printf("Dretva %d je stavila broj %lx u spremnik. \n", *d, x);
        
        printf("Dretva %d izlazi iz K.O.\n", d);
		//izadi_iz_KO(*d);
        sem_post(&KO);
        sem_wait (&PUNI);
	}
    printf("Dretva %d zavrsava.\n", d);

	obrisi_generator (&r);

	return NULL;
}

void *neradnaDretva(void *id)
{
	uint64_t y;
	int *d = id;

    printf("Dretva %d pocinje.\n", d);
	while(kraj != 1 )
	{
		sleep(3);

        sem_wait(&PUNI);
		//udi_u_KO(*d);
        sem_wait(&KO);
        printf("Dretva %d unutar K.O.\n", d);
		
        y = uzmi_iz_MS();
		printf("Dretva %d je uzela broj %lx iz spremnika.\n", *d, y);
        
        printf("Dretva %d izlazi iz K.O.\n", d);
		//izadi_iz_KO(*d);
        sem_post(&KO);
        sem_post(&PRAZNI);
	}
	printf("Dretva %d zavrsava.\n", d);

	return NULL;
}



int main(int argc, char *argv[])
{
	int *BR1, *BR2;
	pthread_t *t1, *t2;
	int i, j;

	inicijaliziraj_generator (&p, 0);

	velicina = procjeni_velicinu_grupe();

    sem_init (&KO, 0, 1);
    sem_init (&PUNI, 0, 10);
    sem_init (&PRAZNI, 0, 0);

	for (i = 0; i < brojDretvi * 2; i++)
	{
		broj[i] = 0;
		ulaz[i] = 0; 
	}

	BR1 = malloc (brojDretvi * sizeof(int));
	t1 = malloc (brojDretvi * sizeof(pthread_t));

	BR2 = malloc (brojDretvi * sizeof(int));
	t2 = malloc (brojDretvi * sizeof(pthread_t));

	for (i = 0; i < brojDretvi; i++ )
	{
		BR1[i] = i;
		if (pthread_create(&t1[i], NULL, radnaDretva, &BR1[i]))
		{
			fprintf(stderr, "Ne mogu stvoriti novu dretvu.\n");
			exit(1);
		}
	}

	for (i = 0; i < brojDretvi; i++ )
	{
		BR2[i] = i + brojDretvi;
		if (pthread_create(&t2[i], NULL, neradnaDretva, &BR2[i]))
		{
			fprintf(stderr, "Ne mogu stvoriti novu dretvu.\n");
			exit(1);
		}
	}

	sleep(20);

	kraj = 1;

	for (j = 0; j < brojDretvi; j++)
		pthread_join(t1[j], NULL);

	for (j = 0; j < brojDretvi; j++)
		pthread_join(t2[j], NULL);

	obrisi_generator (&p);

    
    sem_destroy (&KO);
    sem_destroy (&PUNI);
    sem_destroy (&PRAZNI);

	return 0;
}


/*
  prevođenje:
  - ručno: gcc program.c slucajni_prosti_broj.c -lgmp -lm -o program
  - preko Makefile-a: make
  pokretanje:
  - ./program
  - ili: make pokreni
  nepotrebne datoteke (.o, .d, program) NE stavljati u repozitorij
  - obrisati ih ručno ili s make obrisi
*/

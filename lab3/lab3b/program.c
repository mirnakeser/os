#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "slucajni_prosti_broj.h"

#define MASKA(bitova)			(-1 + (1<<bitova) )
#define UZMIBITOVE(broj,prvi,bitova) 	( ( broj >> (64-(prvi)) ) & MASKA(bitova) )

uint64_t MS[10], ULAZ = 0, IZLAZ = 0;
uint64_t velicina = 1;

int brojDretvi = 5;
int broj[10], ulaz[10];

int kraj = 0;

struct gmp_pomocno p;

pthread_mutex_t m;
pthread_cond_t red_prazni, red_puni;
int br_praznih = 10, br_punih = 0;


void stavi_u_MS(uint64_t broj)
{
	MS[ULAZ] = broj;
	ULAZ = (ULAZ + 1) % 10;
	return;
}

uint64_t uzmi_iz_MS()
{
	uint64_t broj;
	broj = MS[IZLAZ];
	IZLAZ = (IZLAZ + 1) % 10;
	return broj;
}
/*
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
*/
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
	
	

	return velicina;

}

void *radnaDretva (void *id)
{
	
	int *d = id;
	uint64_t x;
	struct gmp_pomocno r;

	inicijaliziraj_generator (&r, *d);

	while( kraj != 1 )
	{
		x = generiraj_dobar_broj(&r);

		//udi_u_KO(*d);
		pthread_mutex_lock (&m);
		while (br_praznih == 0 && kraj != 1)
			pthread_cond_wait(&red_prazni, &m);
		if ( kraj == 1)
		{
			pthread_mutex_unlock(&m);
			break;
		}
			

		stavi_u_MS(x);
		printf("Dretva %d je stavila broj %lx u spremnik. \n", *d, x);

		//izadi_iz_KO(*d);
		br_punih++;
		br_praznih--;
		pthread_cond_signal(&red_puni);
		pthread_mutex_unlock(&m);
	}

	obrisi_generator (&r);

	return NULL;
}

void *neradnaDretva(void *id)
{
	uint64_t y;
	int *d = id;

	while(kraj != 1 )
	{
		sleep(3);

		//udi_u_KO(*d);
		pthread_mutex_lock (&m);
		while (br_punih == 0 && kraj != 1)
			pthread_cond_wait(&red_puni, &m);
		if ( kraj == 1)
		{
			pthread_mutex_unlock(&m);
			break;
		}

		y = uzmi_iz_MS();
		printf("Dretva %d je uzela broj %lx iz spremnika.\n", *d, y);

		//izadi_iz_KO(*d);
		br_praznih++;
		br_punih--;
		pthread_cond_signal(&red_prazni);
		pthread_mutex_unlock(&m);
	}
	
	return NULL;
}



int main(int argc, char *argv[])
{
	int *BR1, *BR2;
	pthread_t *t1, *t2;
	int i, j;

	inicijaliziraj_generator (&p, 0);

	velicina = procjeni_velicinu_grupe();

	pthread_mutex_init (&m, NULL);
	pthread_cond_init (&red_puni, NULL);
	pthread_cond_init (&red_prazni, NULL);

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

	
	pthread_cond_broadcast(&red_puni);
		
	pthread_cond_broadcast(&red_prazni);


	for (j = 0; j < brojDretvi; j++)
		pthread_join(t1[j], NULL);

	for (j = 0; j < brojDretvi; j++)
		pthread_join(t2[j], NULL);

	obrisi_generator (&p);

	pthread_mutex_destroy (&m);
	pthread_cond_destroy(&red_puni);
	pthread_cond_destroy(&red_prazni);

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

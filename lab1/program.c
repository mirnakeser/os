
#include <stdio.h>
#include <time.h>

#include "slucajni_prosti_broj.h"

#define MASKA(bitova)			(-1 + (1<<bitova) )
#define UZMIBITOVE(broj,prvi,bitova) 	( ( broj >> (64-(prvi)) ) & MASKA(bitova) )

struct gmp_pomocno p;

uint64_t MS[10], ULAZ = 0, IZLAZ = 0;


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

/*
//profesorova ideja
uint64_t zbrckanost (uint64_t x)
{
	uint64_t z = 0, i, j, b1, pn;
	
	for (i = 0; i <= 64 - 6; i++)
	{
		b1 = 0;
		//podniz bitova od x: od x[i] do x[i+5]
		pn = UZMIBITOVE(x, i+6, 6);
		for (j = 0; j < 6; j++)
			if (((1<<j) & pn))
				b1++;
		if (b1 > 4)
			z += b1 - 4;
		else if (b1 < 2)
			z += 2 - b1;
	} 
	return z;
}
*/

uint64_t generiraj_dobar_broj(uint64_t velicina_grupe)
{
	uint64_t najbolji_broj = 0, i, broj, z;
	uint64_t najbolja_zbrckanost = 0;
	//uint64_t velicina_grupe = 1000;
		
	for (i = 0; i < velicina_grupe; i++)
	{
		broj = daj_novi_slucajan_prosti_broj (&p);
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
	uint64_t velicina_grupe = 1;
	uint64_t i, broj, brojeva_u_sekundi;

	while (time(NULL) < (t + SEKUNDI) )
	{
		k++;
		for (i = 0; i < M; i++)
		{
			broj = generiraj_dobar_broj(velicina_grupe);
			stavi_u_MS(broj);
		}
	}

	brojeva_u_sekundi = (k * M / SEKUNDI);
	velicina_grupe = brojeva_u_sekundi / 2.5;

	return velicina_grupe;

}

int main(int argc, char *argv[])
{
	uint64_t broj, a, velicina, broj_ispisa = 0;
	time_t t = time(NULL);

	inicijaliziraj_generator (&p, 0);

	velicina = procjeni_velicinu_grupe();

	while (broj_ispisa < 10)
	{
		broj = generiraj_dobar_broj(velicina);
		stavi_u_MS(broj);
		if(time(NULL) != t )
		{
			a = uzmi_iz_MS();
			printf("%ld.: %lx\n", broj_ispisa+1, a);
			broj_ispisa++;
			t = time(NULL);
		}
	}

	obrisi_generator (&p);

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

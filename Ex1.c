/* 
* 	
*	Gustavo J. Alfonso Ruiz
*		
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <pthread.h>

#include "Ex1.h"

/**************************************************/
temperature_data_t nfile_data[MAX_LINES];

static bool is_finished = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;			// Iniciamos nuestro mutex.
pthread_cond_t cond_var1  = PTHREAD_COND_INITIALIZER;		// Iniciamos nuestra condición de espera cond_var1.
pthread_cond_t cond_var2  = PTHREAD_COND_INITIALIZER;		// Iniciamos nuestra condición de espera cond_var2.

FILE *fp;													// File para nuestra funcion de lectura.
FILE *pf;													// File para nuestra funcion de escritura.

int num_lines = 0;											// Número de lineas que leeremos del archivo.
int current_thread = 1;										// Variable para asegurarnos que estamos en el thread correcto.

/**************************************************/
static void* readFile(void* ptr);
static void split_file_data(char* line, char token, file_data_t* data);
static void* writeFile(void* ptr);
static void fileExit(void);

/**************************************************/

int main(int argc, char *argv[])
{
	pthread_t t1, t2;		// Definimos nuestros dos threads.
    
	/* Abrimos el archivo con el nombre dado por el usuario. */
    fp = fopen(argv[1], "r");
    
	/* Comprobamos que el archivo existe, en caso contrario obtnemos error. */
    if (fp == NULL)
    {
        perror("Unable to open the file.\n");
        exit(EXIT_FAILURE);
    }
    
	/* 
	*	Creacion de los threads.
	*	El therad t1 sera el encargado de leer la información del fichero y de separar la temperatura.
	*	El thread t2 sera el encargado de escribir la información tomada por t1 en un nuevo fichero. 
	*/
    pthread_create(&t1, NULL, readFile, NULL);
    pthread_create(&t2, NULL, writeFile, NULL);   
    
    /* Esperamos a que los threads terminen. */
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    /* Nos aseguramos de que no queda ningun mutex funcionando. */
    pthread_mutex_destroy(&mutex);
    
    /* Cerramos los archivos. */
	fileExit();
	
	/* Finalizamos el programa. */
    return(0);
                 
}

/* 
*	Funcion que utilizará t1 para leer el archivo. 
*/
static void* readFile(void* ptr)
{
	/* Bloqueamos usando el mutex. */
	pthread_mutex_lock(&mutex);
	
	/* Loop que utilizaremos para comprobar que estamos usando el thread correcto. */
	while(current_thread == 2)
	{
		/* En caso de que no estemos en el thread correcto lo ponemos en espera y esperamos a que entre el thread correcto. */
		pthread_cond_wait(&cond_var1, &mutex);
	}
	
	/* Una vez nos asegurados de que estamos en el thread que queremos, continuamos con el código. */
	
	int bytes_read, i = 0;		// Variable que utilizaremos para comprobar si una línea esta vacía.
    char * line = NULL;    		// Variable que utilizaremos para comprobar si una línea esta vacía.
    size_t len;					// Variable que utilizaremos para comprobar si una línea esta vacía.
    	       
    /* Lee una linea del archivo. */
    bytes_read = getline(&line, &len, fp);
        
    /* ¿Hemos llegado al final del archivo? */
    if(bytes_read == -1)
    {
        printf("End of file detected.\n");
        is_finished = true;         
	}
    
    /* Loop hasta interrunpir el programa. */
    while(!is_finished)
	{
		/* Iniciamos nuestra estructura. */	
		file_data_t file_data = {0,0,0,0};
  
        /* Leemos una linea del archivo. */
    	bytes_read = getline(&line, &len, fp);

        /* ¿Hemos llegado al final del archivo? */
        if(bytes_read == -1)
    	{
            printf("End of file detected.\n");
            is_finished = true;
			break;         
    	}
        
		/* Función que utilizaremos para separar las lineas leidas del fichero y asignar la informacion a nuestra nueva estructura. */
        split_file_data(line, ',', &file_data);

        /* Asginamos la temperatura leida. */
        nfile_data[i].temperature = file_data.temperature;
        
        i++;
    	
    	    	
	}
    
    /* 
	*	Copiamos el numero de veces que nuestro programa ha leido una línea a nuestra variable global.
	*	Este dato será utilizado después para saber el número exacto de líneas que tenemos que escribir en nuestro nuevo archivo.
	*/
    num_lines = i;
    
    /* Mostramos el número de líneas leidas. */
    printf("Read %d lines.\n", num_lines);
    
    /* Actualizamos el thread. */
    current_thread = 2;
    
    printf("Thread 1 finished.\n");
    
    /* Signal cond_var2 */
    pthread_cond_signal(&cond_var2);
    
    /* Deslbloqueamos el mutex. */
    pthread_mutex_unlock(&mutex);
    
	return 0;
}

/* 
*	Función que utilizará t2 para copiar la información leida en un nuevo archivo. 
*/
static void* writeFile(void* ptr)
{
	/* Bloqueamos el mutex. */
	pthread_mutex_lock(&mutex);
	
	/* Loop que utilizaremos para comprobar que estamos usando el thread correcto. */
	while(current_thread == 1)
	{
		/* En caso de que no estemos en el thread correcto lo ponemos en espera y esperamos a que entre el thread correcto. */
		pthread_cond_wait(&cond_var2, &mutex);
	}

	/* Una vez nos asegurados de que estamos en el thread que queremos, continuamos con el código. */
	
	int count;		// Contador.
	
	/* Creamos nuestro nuevo archivo. */
	pf = fopen("writeFile.txt", "w");
	
	if (pf == NULL)
	{
	    perror("Unable to write the file.\n");
        exit(EXIT_FAILURE);
    	
	}
	
	/* Loop donde copiaremos la temperatura que hemos leido previamente en readFile. */
	for(count = 0; count < num_lines; count++)
	{
		fprintf(pf, "%d \n", nfile_data[count].temperature);
		
		/* Imprime por pantalla el numero copiado. */
		//printf("temp: %d, suscesfully copied.\n", nfile_data[count].temperature);
		
	}

	/* Mostramos por pantalla el número de líneas leidas. */
	printf("Write %d lines.\n", count);
	
	/* Actualizamos el thread. */
	current_thread = 1;
	
	printf("Thread 2 finished.\n");
	
	/* Signal cond_var1 */
    pthread_cond_signal(&cond_var1);
    
    /* Desbloqueamos el mutex. */
	pthread_mutex_unlock(&mutex);
	
	return 0;
}

/**************************************************/

/*
*	Funcion para separar los datos del archivo y añadirlos a una nueva estructura. Es la misma funcion de la PR1 solo que con nombre distinto.
*/
static void split_file_data(char* line, char token, file_data_t* data)
{
	int16_t* data_ptr[4] = {&(data->temperature), &(data->altitude), &(data->velocity), &(data->power)};
	char* save_ptr = line;
	char* split = NULL;
	int16_t i = 0;
	
	/*  Miramos por el primer token y lo separamos de la linea. */
	split = strtok_r(save_ptr, &token, &save_ptr);
	if (split != NULL)
	{
	   /* Convertimos el valor a integer y lo almacenamos en la posicion correspondiente. */
	   *data_ptr[i] = atoi(split);
	
	   i++;
	}
	
	/* Si quedan mas token, repetimos el proceso. */
	while (split != NULL)
	{
			    /* Miramos los token y los separamos de la linea. */
	    split = strtok_r(NULL, &token, &save_ptr);
	    if (split != NULL)
	    {
	      /* Convertimos el valor a integer y lo almacenamos en la posicion correspondiente. */
	      *data_ptr[i] = atoi(split);
	
	      i++;
	    }
	  }
    
}

/**************************************************/

static void fileExit(void)
{
	fclose(fp);
    fclose(pf);
}

/**************************************************/



#ifndef _EX1_H_
#define _EX1_H_

#define MAX_LINES    ( 9999 )

/**************************************************/
#include <stdint.h>

#include <pthread.h>

/**************************************************/
typedef struct 
{
  int16_t temperature;
} temperature_data_t;

typedef struct 
{
  int16_t temperature;
  int16_t altitude;
  int16_t velocity;
  int16_t power;
} file_data_t;
/**************************************************/

#endif /* _EX1_H_ */

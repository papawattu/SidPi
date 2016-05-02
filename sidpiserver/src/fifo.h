/* 
 * File:   fifo.h
 * Author: jamie
 *
 * Created on 08 January 2016, 23:16
 */

//#include <GenericTypeDefs.h>


#ifndef FIFO_H
#define	FIFO_H

#define BOOL unsigned char
#define TRUE 1
#define FALSE 0
#define UINT8 unsigned char

#ifdef	__cplusplus
extern "C" {
#endif

#define FIFO_MAX 65536 *512

typedef struct FIFO FIFO;

FIFO * initFIFO(const int);
int readFIFO(FIFO *);
int writeFIFO(FIFO *,UINT8);
int FIFOCount(FIFO * );
void resetFIFO(FIFO *);
BOOL isFIFOEmpty(FIFO *);
int isFIFOFull(FIFO *);
int FIFOSize(FIFO *);



#ifdef	__cplusplus
}
#endif

#endif	/* FIFO_H */


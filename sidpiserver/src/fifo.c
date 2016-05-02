#include <stdlib.h>
#include "fifo.h"

struct FIFO {
        volatile unsigned char * fifo_buf;
        volatile unsigned int fifo_size;
        volatile unsigned int fifo_count;
        volatile unsigned int fifo_first;
        volatile unsigned int fifo_last;
} fifo_p;

FIFO * initFIFO(const int size) {

    FIFO * _buf;

    if(size > FIFO_MAX || size <= 0) {
        return (void *) -1;
    }

    if((_buf = (FIFO *) malloc(sizeof(fifo_p))) ==NULL) {
    	return (void *) -1;
    }

    if((_buf->fifo_buf = (unsigned char *) malloc(size)) == NULL) {
    // failed to allocate buffer, so return -1
        return (void *)-1;
    }

    _buf->fifo_count = 0;
    _buf->fifo_first = 0;
    _buf->fifo_last = 0;
    _buf->fifo_size = size;

    return _buf;
}
int readFIFO(FIFO * fifo) {

    int retval = 0;
    if(fifo->fifo_count == 0) {
    // nothing to return fifo is emtpy
        return -1;
    }

    retval = fifo->fifo_buf[fifo->fifo_first];
    fifo->fifo_first = (fifo->fifo_first + 1) & fifo->fifo_size -1;
    fifo->fifo_count --;
    return retval;
}
int writeFIFO(FIFO * fifo,UINT8 byte) {

    if(fifo->fifo_count == fifo->fifo_size) {
    // buffer is full
        return -1;
    }

    fifo->fifo_buf[fifo->fifo_last] = byte;
    fifo->fifo_last = (fifo->fifo_last + 1) & fifo->fifo_size - 1;
    fifo->fifo_count ++;
    return fifo->fifo_count;
}
int FIFOCount(FIFO * fifo) {
    return fifo->fifo_count;
}
void resetFIFO(FIFO * fifo) {
    fifo->fifo_count = 0;
    fifo->fifo_first = 0;
    fifo->fifo_last = 0;
}

BOOL isFIFOEmpty(FIFO * fifo) {
    return (fifo->fifo_count == 0?TRUE:FALSE);
}

int isFIFOFull(FIFO * fifo) {
    return (fifo->fifo_count == fifo->fifo_size?1:0);
}

int FIFOSize(FIFO * fifo) {
	return fifo->fifo_size;
}
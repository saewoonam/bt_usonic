#ifndef _SIMPLE_DSP_H
#define _SIMPLE_DSP_H

// #define PERIOD_BUFFER_SIZE	256

#include "em_cmu.h"

extern int32_t k_goertzel;
extern int32_t k_speaker;
extern bool distant;

extern int16_t left_t[32];
extern int16_t right_t[32];
extern int _data_idx;

//extern uint32_t timer0_RTCC;
//extern uint32_t timer1_RTCC;

void calculate_periods_list(int start, int stop, float pw, uint16_t *list, int *N);
void calculate_period_k(int k, float pw, int buffer_size, uint16_t *list, int *N);
void calc_chirp(int start, int stop, float pw, uint8_t *chirp, int *N);
void calc_cross(int16_t *x, int Nx, int8_t *y, int Ny, int16_t *xy);
void cumsum(int *x, int Nx, int *ans);
float goertzel(int16_t *x,int32_t k);
float check_float(void);
void calc_chirp_v2(int k, float pw, int8_t *chirp, int *N);
void filter_biquad(int16_t *x, uint8_t filter);
int shape(int16_t* x);
int width(int16_t* x, int max_idx, float thresh, int *mid);
void shape_v2(int16_t* x, int16_t *max, int *max_idx);

int IQR(int16_t* a, int n, int *mid_index);

#endif

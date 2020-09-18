#include "simple_dsp.h"
#include "buffer_size_define.h"
// #include "ultrasound_define.h"
#include "math.h"
#include "app.h"
// #include "stdio.h"
#include "stdlib.h"

extern float pulse_width;


void calculate_periods_list(int start, int stop, float pw, uint16_t *list, int *N) {
	int timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (0 + 1);
	int top_start = (int)(timerFreq / start);
	*N = (int) (2 * (start*stop) / (stop + start) * pw);
	float top_step = (1.0*(start - stop)) / (start * stop) * timerFreq / (*N - 1);
	for (int i=0; i<*N; i++) {
		list[i] = top_start + (int) (i*top_step);
	}
}

void calculate_period_k(int k, float pw, int buffer_size, uint16_t *list, int *N) {
	int timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (0 + 1);
	int top_start = (int)(32*6*buffer_size/k);
	*N = (int) (timerFreq / top_start * pw);
	//  period is given by top+1
	// https://www.silabs.com/documents/public/application-notes/AN0014.pdf
	list[0] = top_start; //-1;
	//	for (int i=0; i<*N; i++) {
//		list[i] = top_start;
//	}
}


void calc_chirp(int start, int stop, float pw, uint8_t *chirp, int *N) {
	int time = 0;
	int offset = 0;
	int index = 0;
	int halfway = 0;
	int period;
	int prescale = 32*6;
	uint16_t list[TIMER_BUFFER_SIZE];
	int N_list=0;

	calculate_periods_list(start, stop, pw, list, &N_list);
	// printf("period_list_length: %d\r\n", N_list);
	for (int i=0; i<N_list; i++) {
		period = list[i];
	    // look at current period
	    // if in first half of period, set 1
	    halfway = period>>1;
	    while (time<(offset + halfway)) {
	        chirp[index] = 1;
	        index++;
	        time += prescale;
	    }
	    while (time<(offset+period)) {
	        chirp[index] = 0;
	        index++;
	        time += prescale;
	    }
	    offset += period;
	}
	*N = index;
}

// assumes x is longer than y
void calc_cross(int16_t *x, int Nx, int8_t *y, int Ny, int16_t *xy) {
	int dot=0;
	int16_t *tempx, i, j;
	int8_t *tempy;
	for(i=0; i<(Nx-Ny+1); i++) {
		dot = 0;
		tempx = x+i;
		tempy = y;
		for (j=0; j<Ny; j++) {
			// dot += ((*tempy++)>0) ? (*tempx++): -(*tempx++);
			/*  Two other option... not sure what will be fastest */
			dot += *tempx++ * *tempy++;
			/*
			if (*tempy++ == 0) {
				dot -= *tempx++;
			} else {
				dot += *tempx++;
			}
			*/
		}
		xy[i] = dot;
	}
}

void cumsum(int *x, int Nx, int *ans) {
	int * temp=ans;
	int prev=0;
	int * tempx = x;

	for (int i=0; i<Nx; i++) {
		*temp = prev + *tempx++;
		prev = *temp++;
	}
}

#define M_PI		3.14159265358979323846

float goertzel(int16_t *x, int32_t k) {
	float w, cw, c, P, z0, z1, z2;

	w = 2 * M_PI * k / BUFFER_SIZE;
	cw = cosf(w);
	c = 2 * cw;
	// sw = np.sin(w);
	z1 = 0;
	z2 = 0;
	for (int idx = 0; idx < BUFFER_SIZE; idx++) {
		z0 = x[idx] + c * z1 - z2;
		z2 = z1;
		z1 = z0;
	}
	// I = cw*z1 -z2;
	// Q = sw*z1;

	P = z2 * z2 + z1 * z1 - c * z1 * z2;
	return P;
}

float check_float(void) {
	return cosf(1.0);
}

void calc_chirp_v2(int k, float pw, int8_t *chirp, int *N) {
	int time = 0;
	int offset = 0;
	int index = 0;
	int halfway = 0;
	uint16_t period;
	int prescale = 32*6;
	int N_list=0;

	calculate_period_k(k, pw, BUFFER_SIZE, &period, &N_list);
	// printLog("calc_chirp_v2, N_list: %d\r\n", N_list);
	for (int i=0; i<N_list; i++) {
	    // look at current period
	    // if in first half of period, set 1
	    halfway = period>>1;
	    while (time<(offset + halfway)) {
	        chirp[index] = 1;
	        index++;
	        time += prescale;
	    }
	    while (time<(offset+period)) {
	        chirp[index] = -1;
	        index++;
	        time += prescale;
	    }
	    offset += period;
	}
	*N = index;
}

void filter_biquad(int16_t *x, uint8_t filter) {
	float y;
	float delay1=0;
	float delay2=0;
	float a1, a2, b0, b1, b2;
	// use https://arachnoid.com/BiQuadDesigner/
	// sample frequency = 100kHz
	if (filter==0) { //10kHz high pass
		a1 = -1.14292982;
		a2 = 0.41273895;
		b0 = 0.63891719;
		b1 = -1.27783439;
		b2 = 0.63891719;
	} else { // 20 kHz high pass
		a1 = -0.36950494;
		a2 = 0.19574310;
		b0 = 0.39131201;
		b1 = -0.78262402;
		b2 = 0.39131201;
	}
	for (int i=0; i< BUFFER_SIZE; i++) {
		y = b0 * x[i] + delay1;
		delay1 = b1 * x[i] - a1 * y + delay2;
		delay2 = b2 * x[i] - a2 * y;
		x[i] = (int) y; // put result back in place
 	}

}
int shape(int16_t* x) {
	for (int i=0; i<BUFFER_SIZE; i++) {
		x[i] = fabs(x[i]);
	}
	// filter_biquad(x, 1);

	float y;
	float delay1=0;
	float delay2=0;
	float a1, a2, b0, b1, b2;
    int max = 0;
    int max_idx = 0;
    float mean = 0;

    // low pass filter to integrate
		a1 = -1.91118480;
		a2 = 0.91496354;
		b0 = 9.44685778e-4;
		b1 = 0.00188937;
		b2 = 9.44685778e-4;

	for (int i=0; i< BUFFER_SIZE; i++) {
		y = b0 * x[i] + delay1;
		delay1 = b1 * x[i] - a1 * y + delay2;
		delay2 = b2 * x[i] - a2 * y;
		x[i] = (int) y; // put result back in place
		mean += y;
		if (x[i]>max) {
			max = x[i];
			max_idx = i;
		}
	}
//	mean /= BUFFER_SIZE;
//	max = 0;
//	max_idx = 0;
//	for (int i = 0; i < BUFFER_SIZE; i++) {
//		x[i] -= mean;
//		if (x[i] > max) {
//			max = x[i];
//			max_idx = i;
//		}
//	}
/*
	float temp = 0;
	for (int i = 0; i < BUFFER_SIZE; i++) {
		temp = x[i] *100.0 / max;
		x[i] = (int) temp;
	}
*/
/*  High pass filters don't work
	// 1Khz
	a1 = -1.91118480;
	a2 = 0.91496354;
	b0 = 0.95653708;
	b1 = -1.91307417;
	b2 = 0.95653708;
	// 2kHz
	a1 = -1.82267251;
	a2 = 0.83715906;
	b0 = 0.91495789;
	b1 = -1.82991579;
	b2 = 0.91495789;
*/
/*
	a1 = -1.95557182;
	a2 = 0.95653726;
	b0 = 0.97802727;
	b1 = -1.95605454;
	b2 = 0.97802727;

	a1 = -1.95428175;
	a2 = 0.95530373;
	b0 = 1.12058460;
	b1 = -2.19280254;
	b2 = 1.07323991;
*/
	// 1kHz high plateau
	if (distant) {
		max = 0;
		max_idx = 0;
		a1 = -1.90860898;
		a2 = 0.91260646;
		b0 = 1.11915390;
		b1 = -2.14173838;
		b2 = 1.02658197;

		for (int i=0; i< BUFFER_SIZE; i++) {
			y = b0 * x[i] + delay1;
			delay1 = b1 * x[i] - a1 * y + delay2;
			delay2 = b2 * x[i] - a2 * y;
			x[i] = (int) y; // put result back in place
			if (x[i]>max) {
				max = x[i];
				max_idx = i;
			}
		}
	}
	return max_idx;
}

void find_max(int16_t* x, int n, int16_t * max, int *idx) {
	for (int i=0; i<n; i++) {
		if (x[i]>*max) {
			*max = x[i];
			*idx = i;
		}
	}
}


int width(int16_t* x, int max_idx, float thresh, int *mid) {
	int left=max_idx;
	int right = max_idx;
	thresh = thresh * x[max_idx];
	while (x[left--]>thresh);
	while(x[right++]>thresh);
	*mid = (int )((right+left)/2);
	// printLog("left, right %d, %d\r\n", x[left], x[right]);
	return right-left;
}

void shape_v2(int16_t* x, int16_t *max, int *max_idx) {
	for (int i=0; i<BUFFER_SIZE; i++) {
		x[i] = fabs(x[i]);
	}
	// printLog("Shape_v2\r\n");
	float y;
	float delay1=0;
	float delay2=0;
	float a1, a2, b0, b1, b2;
//    int max = 0;
//    int max_idx = 0;
    float mean = 0;

    // low pass filter to integrate
		a1 = -1.91118480;
		a2 = 0.91496354;
		b0 = 9.44685778e-4;
		b1 = 0.00188937;
		b2 = 9.44685778e-4;

	for (int i=0; i< BUFFER_SIZE; i++) {
		y = b0 * x[i] + delay1;
		delay1 = b1 * x[i] - a1 * y + delay2;
		delay2 = b2 * x[i] - a2 * y;
		x[i] = (int) y; // put result back in place
		mean += y;
		if (x[i]> *max) {
			*max = x[i];
			*max_idx = i;
			// printLog("search %d %d\r\n", *max, *max_idx);
		}
	}
	printLog("max, max_idx: %d %d\r\n", *max, *max_idx);
	// check for max close in time
	if (*max_idx>300) {
		// printLog("Look for second peak sooner than %d\r\n", *max_idx);
		int16_t max2 = 0;
		int idx2 = 0;
		int pk2_idx = 0;
		int offset = pulse_width*1000*100;
		printLog("offset: %d\r\n", offset);
		find_max(x, *max_idx-offset, &max2, &idx2);
		int w = width(x, idx2, 0.5, &pk2_idx); // do we need this?
		int thresh = *max * 0.2;
		// printLog("thresh: %d\r\n", thresh);
		if (thresh < 30) thresh = 30;

		if ((max2>thresh) && (idx2+offset+1 < *max_idx)) {
			// replace with earlier peak
			*max_idx = idx2;
			*max = max2;
		} else {
			;
		}
		printLog("peak2 idx: %d, max value: %d, width: %d pk2_idx:%d\r\n", idx2, max2, w, pk2_idx);
	}
}


// from https://www.geeksforgeeks.org/interquartile-range-iqr/
// Function to give index of the median
int median(int16_t* a, int l, int r)
{
    int n = r - l + 1;
    n = (n + 1) / 2 - 1;
    return n + l;
}

// Function to calculate IQR
int IQR(int16_t* a, int n, int *m)
{
   // array should already be sorted
   //  sort(a, a + n);
    // Index of median of entire data
    int mid_index = median(a, 0, n);
    *m = a[mid_index];
    // Median of first half
    int Q1 = a[median(a, 0, mid_index)];
    // Median of second half
    int Q3 = a[median(a, mid_index + 1, n)];
    // IQR calculation
    return (Q3 - Q1);
}


uint64_t code_lsfr(int seed, int polynomial, int length, int *N, uint16_t *pw, int8_t *chirp ) {
    int lsb;
    unsigned int data = seed;
    uint64_t compact = 0;
    for (int i=0; i<8; i++) {
        // printf("i=%d, %d\n", i, polynomial & (1<<i));
        if (polynomial & (1<<i)) *N = i+1;
    }
    *N = (1<<*N) - 1;
    // printf("N: %d\r\n", *N);
    for (int i=0; i<*N; i++) {
        lsb = data & 1;
        pw[i] = length*192*lsb;
        for (int j=0; j<length; j++) {
            chirp[i*length + j] = 2*lsb-1;
        }
        data = data >> 1;
        compact <<= 1;
        if (lsb>0) {
            data = data ^ polynomial;
            compact += 1;
        }
    }
    // return 1;
    return compact;
}

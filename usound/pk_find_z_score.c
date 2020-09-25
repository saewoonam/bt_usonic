#include <stdio.h>
#include <math.h>
#include <string.h>


#define SAMPLE_LENGTH 1000

float stddev(float data[], int len);
float mean(float data[], int len);
void thresholding(float y[], int8_t signals[], int lag, float threshold, float influence);


void thresholding(float y[], int8_t signals[], int lag, float threshold, float influence) {
    memset(signals, 0, sizeof(int8_t) * SAMPLE_LENGTH);
    float filteredY[lag];
    memcpy(filteredY, y, sizeof(float) * lag);

    float avgFilter = mean(y, lag);
    float stdFilter = stddev(y, lag);

    for (int i = lag; i < SAMPLE_LENGTH; i++) {
        if ((y[i] - avgFilter) > threshold * stdFilter) {
			signals[i] = 1;
			filteredY[i % lag] = influence * y[i]
					+ (1 - influence) * filteredY[(i - 1) % lag];
        } else {
            signals[i] = 0;
        }
        avgFilter = mean(filteredY, lag);
        stdFilter = stddev(filteredY, lag);
    }
}

float mean(float data[], int len) {
    float sum = 0.0, mean = 0.0;

    int i;
    for(i=0; i<len; ++i) {
        sum += data[i];
    }

    mean = sum/len;
    return mean;


}

float stddev(float data[], int len) {
    float the_mean = mean(data, len);
    float standardDeviation = 0.0;

    int i;
    for(i=0; i<len; ++i) {
        standardDeviation += pow(data[i] - the_mean, 2);
    }

    return sqrt(standardDeviation/len);
}

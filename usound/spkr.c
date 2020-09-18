/*
 * spkr.c
 *
 *  Created on: Aug 21, 2020
 *      Author: nams
 */
// #include "speaker.h"
#include "simple_dsp.h"

#include "bg_types.h"
#include "app.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_gpio.h"
#include "em_device.h"
// #include "em_ldma.h"
#include "dmadrv.h"
#include "em_timer.h"
#include "em_rtcc.h"

#include "bsp.h"

#include "buffer_size_define.h"
#define TAPER
extern float pulse_width;
extern int8_t pdm_template[512];

#define C4

// float pulse_width = 5e-3; /**< Input: Square Chirp Duration, in seconds **/
float dutyCycle = 0.5; /**<  **/
int numWaves; /**< Calculated: Number of waves in the Square Chirp **/

uint32_t timerFreq; /**< Constant: Calculating using TIMER_PDM's maximum frequency and prescale value **/

uint16_t list_pwm[TIMER_BUFFER_SIZE]; /**< List of COMP values for the TIMER, for PWM **/
// uint16_t list_top[TIMER_BUFFER_SIZE]; /**< List of TOP values for the TIMER, for FREQ **/
uint16_t top = 768;
bool speaker_on = false;

unsigned int ldma_channelTMR_TOPV, ldma_channelTMR_COMP;

void setBuffer(uint16_t top_value, int n) {
	numWaves = n;
	top = top_value;
	for(int i=0; i<numWaves; i++) {
		list_pwm[i] = top>>1;
	}
}

void populateBuffers(int k_value) {
	// float pulse_width = 5e-3;
	int pw = pulse_width*1000;
	// printLog("pulse_width %d ms\r\n", pw);
	static int k=0;  // use this to remember k from call to call... if k_value<0, use previous k
	if (k_value > 0) {
		k = k_value;
	}
	// printLog("k: %d\r\n", k);
	calculate_period_k(k, pulse_width, BUFFER_SIZE, &top,
			&numWaves);
//	printLog("numWaves %d top %d\r\n", numWaves, top);
//	calculate_periods_list(freq_start, freq_stop, pulse_width, list_top,
//			&numWaves);
//	printLog("numWaves %d top %d\r\n", numWaves, list_top[0]);
	for (uint32_t i = 0; i < numWaves; i++) {
		list_pwm[i] = top * dutyCycle;
	}
#ifdef TAPER
	if (numWaves > 20) {
		float prev = 0;
		for (int i = 0; i < 9; i++) {
			prev += 0.05;
			list_pwm[i] = top * prev;
			list_pwm[numWaves - 1 - i] = top * prev;
		}
	}
#endif
}

void populateBuffers_gold(int k_value) {
	int code_length;
	// polynomial 51 or 33
	code_lsfr((k_value%32)+1, 51, 1, &code_length, list_pwm, pdm_template);
	numWaves = code_length;
	printLog("Finished populateBuffers_gold, numWave:%d\r\n", numWaves);
}
//bool dma_tmr_topv_cb(unsigned int channel, unsigned int sequenceNo,
//		void *userParam) {
//	printLog("tmr_topv_cb: channel %d, sequenceNo %d\r\n", channel, sequenceNo);
//	TIMER_Enable(TIMER1, false);
//	return 0;
//}

void timer1_prescale(int prescale);

void startDMADRV_TMR(void);

bool dma_tmr_comp_cb(unsigned int channel, unsigned int sequenceNo,
		void *userParam) {
//	uint32_t curr = RTCC_CounterGet();
//	printLog("%lu, %6lu, %6lu:  spkr tmr_comp_cb: channel %d\r\n", curr,
//			curr - prev_rtcc, 0L, channel);
	// printLog("tmr_comp_cb: channel %d, sequenceNo %d\r\n", channel, sequenceNo);
	TIMER_Enable(TIMER1, false);
	speaker_on=false;
    timer1_prescale(0);

	startDMADRV_TMR();
	return 0;
}

void play_speaker(void) {
	if (!speaker_on) {
		speaker_on=true;
		TIMER_Enable(TIMER1, true);
	} else printLog("%8ld, Already playing speaker\r\n", RTCC_CounterGet());
}

void startDMADRV_TMR(void) {
	//TOPV
//	DMADRV_MemoryPeripheral(ldma_channelTMR_TOPV,
//			dmadrvPeripheralSignal_TIMER1_UFOF, &TIMER1->TOPB, list_top, true,
//			numWaves, dmadrvDataSize2, dma_tmr_topv_cb, NULL);
	TIMER1->TOPB = top;

	//COMP
	DMADRV_MemoryPeripheral(ldma_channelTMR_COMP,
			dmadrvPeripheralSignal_TIMER1_CC1, &TIMER1->CC[1].OCB, list_pwm,
			true, numWaves, dmadrvDataSize2, dma_tmr_comp_cb, NULL);
}

void stopDMADRV_TMR(void) {
	DMADRV_StopTransfer(ldma_channelTMR_COMP);
}

void init_speaker(void) {
	// Setup ldma channels
	//	uint32_t e0 = DMADRV_AllocateChannel(&ldma_channelTMR_TOPV, NULL);
	//	printLog("DMADRV channel %d, retcode: %lu\r\n", ldma_channelTMR_TOPV, e0);
	uint32_t e1 = DMADRV_AllocateChannel(&ldma_channelTMR_COMP, NULL);
	printLog("DMADRV channel %d, retcode: %lu\r\n", ldma_channelTMR_COMP, e1);
	// CMU
	CMU_ClockEnable(cmuClock_TIMER1, true);
	// these aren't needed if in a bluetooth project
	CMU_ClockEnable(cmuClock_GPIO, true);
	CMU_ClockEnable(cmuClock_PRS, true);
	CMU_ClockEnable(cmuClock_RTCC, true);
	//CMU_ClockEnable(cmuClock_PDM, true);

	//gpio
//  GPIO_PinModeSet(gpioPortD, 2, gpioModePushPull, 0);     // SPKR_NEG (used for differential drive, not currently configured)
	GPIO_PinModeSet(gpioPortD, 3, gpioModePushPull, 0);     // SPKR_POS
//  debug header
	//timer1 to speaker pin
	GPIO->TIMERROUTE[1].ROUTEEN = GPIO_TIMER_ROUTEEN_CC1PEN; // | GPIO_TIMER_ROUTEEN_CC0PEN
//  GPIO->TIMERROUTE[0].CC0ROUTE = (gpioPortD << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (2 << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
#ifdef D3
	GPIO->TIMERROUTE[1].CC1ROUTE =
			(gpioPortD << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
					| (3 << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
#endif
#ifdef C4
	GPIO_PinModeSet(gpioPortC, 4, gpioModePushPull, 0);     // SPKR_POS
	GPIO->TIMERROUTE[1].CC1ROUTE =
			(gpioPortC << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
					| (4 << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
#endif
}

void beep(uint16_t period) {
	stopDMADRV_TMR();
	timer1_prescale(9); // this will get reset to 0 in the DMA callback
	// scale buffer size to frequency/pitch of 880
	int len = (880*500) / period;
	printLog("period: %d, len: %d\r\n", period, len);
	setBuffer(period, len);
	startDMADRV_TMR();
	play_speaker();
}

/*
void initTIMER(void) {
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.prescale = timerPrescale1;
	timerInit.enable = false;
	timerInit.debugRun = false;
	timerInit.riseAction = timerInputActionReloadStart;
	TIMER_Init(TIMER1, &timerInit);

	TIMER_InitCC_TypeDef timerCC0Init = TIMER_INITCC_DEFAULT;
	timerCC0Init.edge = timerEdgeBoth;
	timerCC0Init.mode = timerCCModeCapture;
	timerCC0Init.prsSel = TX_OBS_PRS_CHANNEL;
	timerCC0Init.prsInput = true;
	timerCC0Init.prsInputType = timerPrsInputAsyncPulse;
	TIMER_InitCC(TIMER1, 0, &timerCC0Init);

	TIMER_InitCC_TypeDef timerCC1Init = TIMER_INITCC_DEFAULT;
	timerCC1Init.mode = timerCCModePWM;
	TIMER_InitCC(TIMER1, 1, &timerCC1Init);

	// Enable TIMER0 interrupts
	TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
	NVIC_EnableIRQ(TIMER1_IRQn);
}
*/
/*
extern uint32_t prev_rtcc;
uint32_t timer1_RTCC = 0;
void TIMER1_IRQHandler(void) {
	// Acknowledge the interrupt
	uint32_t flags = TIMER_IntGet(TIMER1);
	TIMER_IntClear(TIMER1, flags);
	static uint32_t prev = 0;
	timer1_RTCC = RTCC_CounterGet();

	printLog("%lu, %4lu,  %4lu:TX\r\n", timer1_RTCC, timer1_RTCC - prev_rtcc, timer1_RTCC-prev);
	prev_rtcc = timer1_RTCC;
	prev = prev_rtcc;
}
*/

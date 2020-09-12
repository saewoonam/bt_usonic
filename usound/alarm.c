/*
 * alarm.c
 *
 *  Created on: Sep 9, 2020
 *      Author: nams
 */
#include "em_timer.h"
#include "em_cmu.h"
// #include "em_emu.h"
// #include "em_chip.h"
#include "em_gpio.h"
#include "app.h"

#define ALARM_FREQ 4000
#define C4

void alarm_finished(void);

void initTIMER1_alarm(void) {
	NVIC_DisableIRQ(TIMER1_IRQn);
	uint32_t timerFreq = 0;
	// Initialize the timer
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	// Configure TIMER1 Compare/Capture for output compare
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

	timerInit.prescale = timerPrescale256;
	timerInit.enable = false;
	timerCCInit.mode = timerCCModeCompare;
	timerCCInit.cmoa = timerOutputActionToggle;

	// configure, but do not start timer
	TIMER_Init(TIMER1, &timerInit);
#ifdef C4

	GPIO->TIMERROUTE[1].ROUTEEN = GPIO_TIMER_ROUTEEN_CC1PEN;
	GPIO->TIMERROUTE[1].CC1ROUTE =
			(gpioPortC << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
					| (4 << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
#else
	GPIO->TIMERROUTE[1].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
	GPIO->TIMERROUTE[1].CC0ROUTE = (gpioPortA << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
	| (6 << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
#endif
	TIMER_InitCC(TIMER1, 1, &timerCCInit);

	// Set Top value
	// Note each overflow event constitutes 1/2 the signal period
	timerFreq = CMU_ClockFreqGet(cmuClock_TIMER1) / (timerInit.prescale + 1);
	int topValue = timerFreq / (2 * ALARM_FREQ) - 1;
	TIMER_TopSet(TIMER1, topValue);

	/* Start the timer */
	//TIMER_Enable(TIMER1, true);
}

void initTIMER2(void) {

	CMU_ClockEnable(cmuClock_TIMER2, true);

	// Initialize the timer
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	// Configure TIMER2 Compare/Capture for output compare
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

	// Use PWM mode, which sets output on overflow and clears on compare events
	timerInit.prescale = timerPrescale512;
	timerInit.enable = false;
	timerCCInit.mode = timerCCModePWM;

	// Configure but do not start the timer
	TIMER_Init(TIMER2, &timerInit);

	// Configure CC Channel 0
	TIMER_InitCC(TIMER2, 0, &timerCCInit);


	// Set top value to overflow at the desired PWM_FREQ frequency
	uint16_t topValue = 1<<14;
	TIMER_TopSet(TIMER2, topValue);

	// Set compare value for initial duty cycle
	TIMER_CompareSet(TIMER2, 0, (uint32_t) (topValue>>1));

	// Start the timer
	// TIMER_Enable(TIMER2, true);

	// Enable TIMER0 compare event interrupts to update the duty cycle
	TIMER_IntEnable(TIMER2, TIMER_IEN_CC0);
	NVIC_EnableIRQ(TIMER2_IRQn);
}

void TIMER2_IRQHandler(void) {
	// Acknowledge the interrupt
	static bool play = false;
	static int count = 11;
	uint32_t flags = TIMER_IntGet(TIMER2);
	TIMER_IntClear(TIMER2, flags);

	printLog("IRQ Handler\r\n");
	GPIO_PinOutToggle(gpioPortB, 0);
	TIMER_Enable(TIMER1, play);
	if (play) {
		TIMER_TopSet(TIMER2, 8192);
	} else {
		TIMER_TopSet(TIMER2, 8192);
	}
	play = !play;
	count--;
	if (count==0) {
		TIMER_Enable(TIMER2, false);
		TIMER_Enable(TIMER1, false);
		printLog("Done with alarm\r\n");
		alarm_finished();

	}

}

void init_alarm() {
	GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 0);     // SPKR_POS

	initTIMER1_alarm();
	// initTIMER2();
}

void play_alarm(void) {
	stopDMADRV_TMR();
	initTIMER1_alarm();
	TIMER_Enable(TIMER2, true);
}

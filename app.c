/***************************************************************************//**
 * @file app.c
 * @brief SPP-over-BLE example
 *
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "app.h"
#include "spp_utils.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_rtcc.h"
#include "em_timer.h"

#include "dmadrv.h"
uint32_t timer0_RTCC;
uint32_t timer1_RTCC;

uint32_t prev_rtcc;

#include "em_pdm.h"
extern bool recording;
void initPDM(void);
void startLDMA_PDM(void);
void init_speaker(void);
void adcInit(void);

void initTIMER2(void);

void timer0_prescale(int prescale) {
	// Initialize the timer
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.enable = false;
	timerInit.clkSel = timerClkSelHFPerClk;
	timerInit.prescale = prescale;
	timerInit.riseAction = timerInputActionReloadStart;

	TIMER_Init(TIMER0, &timerInit);
}
void initTIMER0(void) {
	// Enable clock for TIMER0 module
	CMU_ClockEnable(cmuClock_TIMER0, true);
//	// Initialize the timer
//	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
//	timerInit.enable = false;
//	timerInit.clkSel = timerClkSelHFPerClk;
//
//	timerInit.riseAction = timerInputActionReloadStart;
//
//	TIMER_Init(TIMER0, &timerInit);
	timer0_prescale(0);
	// Configure the TIMER0 module for Capture mode and to trigger on every other edge
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	timerCCInit.eventCtrl = timerEventEvery2ndEdge;
	timerCCInit.edge = timerEdgeBoth; // Trigger an input capture on every edge
	timerCCInit.mode = timerCCModeCapture;
	timerCCInit.prsSel = RX_OBS_PRS_CHANNEL;
    //timerCCInit.prsSel = TX_OBS_PRS_CHANNEL;
	timerCCInit.prsInput = true;
	timerCCInit.prsInputType = timerPrsInputAsyncPulse;
	TIMER_InitCC(TIMER0, 0, &timerCCInit);
	// Enable TIMER0 interrupts
	TIMER_IntEnable(TIMER0, TIMER_IEN_CC0);
	NVIC_EnableIRQ(TIMER0_IRQn);
	// Enable the TIMER
	// TIMER_Enable(TIMER0, true);
}

extern bool pdm_on;
void TIMER0_IRQHandler(void) {
	// Acknowledge the interrupt
	uint32_t flags = TIMER_IntGet(TIMER0);
	TIMER_IntClear(TIMER0, flags);
	static uint32_t prev = 0;
	if (pdm_on) {
		if (!recording) {
			recording = true;
			startLDMA_PDM();
// #ifdef PDM_INTERRUPTS
			PDM->IEN |= PDM_IEN_UF | PDM_IEN_OF; //enable interrupts
// #endif
		}
	}
	if (false) {
		timer0_RTCC = RTCC_CounterGet();
		printLog("%lu, %lu, %lu   timer0 recording %d pdm: %d\r\n", timer0_RTCC,
				timer0_RTCC - prev_rtcc, timer0_RTCC-prev, recording, pdm_on);
		prev_rtcc = timer0_RTCC;
		prev = prev_rtcc;
	}
}

void timer1_prescale(int prescale) {
	// Initialize the timer
	TIMER_Enable(TIMER1, false);
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.enable = false;
	timerInit.clkSel = timerClkSelHFPerClk;
	timerInit.prescale = prescale;
	timerInit.riseAction = timerInputActionReloadStart;

	TIMER_Init(TIMER1, &timerInit);
	// TIMER_Enable(TIMER1, true);
}

void initTIMER1(void) {
	// Enable clock for TIMER1 module
	CMU_ClockEnable(cmuClock_TIMER1, true);
	// Initialize the timer
	TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
	timerInit.enable = false;
	timerInit.clkSel = timerClkSelHFPerClk;

	timerInit.riseAction = timerInputActionReloadStart;

	TIMER_Init(TIMER1, &timerInit);
	// Configure the TIMER1 module for Capture mode and to trigger on every other edge
	TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
	timerCCInit.eventCtrl = timerEventEvery2ndEdge;
	timerCCInit.edge = timerEdgeBoth; // Trigger an input capture on every edge
	timerCCInit.mode = timerCCModeCapture;
    // timerCCInit.prsSel = RX_OBS_PRS_CHANNEL;
	timerCCInit.prsSel = TX_OBS_PRS_CHANNEL;
	timerCCInit.prsInput = true;
	timerCCInit.prsInputType = timerPrsInputAsyncPulse;
	TIMER_InitCC(TIMER1, 0, &timerCCInit);

	TIMER_InitCC_TypeDef timerCC1Init = TIMER_INITCC_DEFAULT;
	timerCC1Init.mode = timerCCModePWM;
	TIMER_InitCC(TIMER1, 1, &timerCC1Init);

	// Enable TIMER1 interrupts
	TIMER_IntEnable(TIMER1, TIMER_IEN_CC0);
	NVIC_EnableIRQ(TIMER1_IRQn);
	// Enable the TIMER
	// TIMER_Enable(TIMER1, true);
	printLog("Done with TIMER1 init\r\n");
}

void TIMER1_IRQHandler(void) {
	// Acknowledge the interrupt
	uint32_t flags = TIMER_IntGet(TIMER1);
	TIMER_IntClear(TIMER1, flags);
	// static uint32_t prev = 0;
		timer1_RTCC = RTCC_CounterGet();
		// printLog("%lu, %4lu,  %4lu:TX\r\n", timer1_RTCC, timer1_RTCC - prev_rtcc, timer1_RTCC-prev);
		prev_rtcc = timer1_RTCC;
		// prev = prev_rtcc;
}

static void initTimers() {
	initTIMER0();
	initTIMER1();
	initTIMER2();
}



/* Main application */
void appMain(gecko_configuration_t *pconfig) {
#if DISABLE_SLEEP > 0
	pconfig->sleep.flags = 0;
#endif

	// pconfig->bluetooth.max_connections = 2; /// Use default... Hope this delays crash
	/* Initialize debug prints. Note: debug prints are off by default. See DEBUG_LEVEL in app.h */
	initLog();
	DMADRV_Init();
	initTimers();
	initPDM();
	init_speaker();
	adcInit();

	/* Initialize stack */
	gecko_init(pconfig);
	/*  jump to SPP program main loop */
	spp_main();
}

//void extra() {
//	  adcInit();
//	  readBatteryLevel();
//	  printLog("BatteryLevel %d\r\n", battBatteryLevel);
//	  /* Initialize stack */
//	  // gecko_init(pconfig);
//	  int32_t flash_ret = storage_init();
//	  uint32_t flash_size = storage_size();
//	  printLog("storage_init: %ld %ld\r\n", flash_ret, flash_size);
//	  determine_counts(flash_size);
//	  test_encrypt_compare();
//	  read_name_ps();
//}

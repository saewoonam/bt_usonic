//#include "microphone.h"
#include "simple_dsp.h"
#include "em_pdm.h"
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

#include "buffer_size_define.h"

extern uint32_t prev_rtcc;
extern float pulse_width;
extern int out;

#define PDM_INTERRUPTS
#define GOERTZEL


// PDM stuff
int16_t left[BUFFER_SIZE];
int16_t right[BUFFER_SIZE];
int16_t corr[BUFFER_SIZE];
int8_t pdm_template[512];
int N_pdm_template;

uint32_t pingBuffer[PP_BUFFER_SIZE];
uint32_t pongBuffer[PP_BUFFER_SIZE];
bool prevBufferPing;
unsigned int ldma_channelPDM;
bool recording = false;
// end PDM stuff
#define RECORD_TX

void initPDM(void) {
	// setup ldma for pdm
	uint32_t e2 = DMADRV_AllocateChannel(&ldma_channelPDM, NULL);
	printLog("DMADRV channel %d, retcode: %lx\r\n", ldma_channelPDM, e2);
	// configgure clock
	CMU_ClockEnable(cmuClock_PDM, true);
//    // configure PRS
//#ifdef RECORD_TX
//	PRS_ConnectConsumer(TX_OBS_PRS_CHANNEL, prsTypeAsync, prsConsumerTIMER0_CC0);
//#else
//	PRS_ConnectConsumer(RX_OBS_PRS_CHANNEL, prsTypeAsync, prsConsumerTIMER0_CC0);
//#endif
	//configure gpio for pdm
	GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 1);     // MIC_EN
	GPIO_PinModeSet(gpioPortC, 6, gpioModePushPull, 0);     // PDM_CLK
	GPIO_PinModeSet(gpioPortC, 7, gpioModeInput, 0);        // PDM_DATA
	GPIO_SlewrateSet(gpioPortC, 7, 7);
	GPIO->PDMROUTE.ROUTEEN = GPIO_PDM_ROUTEEN_CLKPEN;
	GPIO->PDMROUTE.CLKROUTE = (gpioPortC << _GPIO_PDM_CLKROUTE_PORT_SHIFT)
			| (6 << _GPIO_PDM_CLKROUTE_PIN_SHIFT);
	GPIO->PDMROUTE.DAT0ROUTE = (gpioPortC << _GPIO_PDM_DAT0ROUTE_PORT_SHIFT)
			| (7 << _GPIO_PDM_DAT0ROUTE_PIN_SHIFT);
	GPIO->PDMROUTE.DAT1ROUTE = (gpioPortC << _GPIO_PDM_DAT1ROUTE_PORT_SHIFT)
			| (7 << _GPIO_PDM_DAT1ROUTE_PIN_SHIFT);

}
void pdm_start(void) {
	// Config PDM registers
	PDM->CFG0 = PDM_CFG0_STEREOMODECH01_CH01ENABLE | PDM_CFG0_CH0CLKPOL_NORMAL
			| PDM_CFG0_CH1CLKPOL_INVERT | PDM_CFG0_FIFODVL_FOUR
			| PDM_CFG0_DATAFORMAT_DOUBLE16 | PDM_CFG0_NUMCH_TWO
			| PDM_CFG0_FORDER_FIFTH;
	PDM->CFG1 = (5 << _PDM_CFG1_PRESC_SHIFT);
#ifdef PDM_INTERRUPTS
	PDM -> IEN |= PDM_IEN_UF | PDM_IEN_OF;
	NVIC_EnableIRQ(PDM_IRQn);
#endif
	// Enable module
	PDM->EN = PDM_EN_EN;
	// Start filter
	while (PDM->SYNCBUSY)
		;
	PDM->CMD = PDM_CMD_START;
	// Config DSR/Gain
	while (PDM->SYNCBUSY)
		;
	// Changed: From 3 to 8
	PDM->CTRL = (5 << _PDM_CTRL_GAIN_SHIFT) | (32 << _PDM_CTRL_DSR_SHIFT);
}


void pdm_pause() {
	DMADRV_StopTransfer(ldma_channelPDM);
	recording = false;
	// stop pdm filter
	while (PDM->SYNCBUSY != 0) ;
	PDM->CMD = PDM_CMD_STOP;
	// clear pdm filter
	while (PDM->SYNCBUSY != 0) ;
	PDM->CMD = PDM_CMD_CLEAR;
	// flush pdfm fifo
	while (PDM->SYNCBUSY != 0) ;
	PDM->CMD = PDM_CMD_FIFOFL;
	// disable
	while (PDM->SYNCBUSY != 0) ;
	PDM->EN = PDM_EN_EN_DISABLE;

}

void PDM_IRQHandler(void) {
	uint32_t flags = PDM->IF_CLR = PDM->IF;
	// flags = PDM_IntGet();
	static int count = 0;

	if (recording) {
		if (flags & PDM_IF_OF) {
			// printLog("PDM OF\r\n");
			count++;
			// flush ovflw from efr32bg22 ref manual
			 while(PDM->SYNCBUSY !=0);
			 PDM->CMD = PDM_CMD_FIFOFL;
			if (count > 200) {
				printLog("Hit 200 OF's...disable for now");
				PDM->IEN &= ~(PDM_IEN_UF | PDM_IEN_OF); //disable interrupts
				count = 0;
			}
		}
		if (flags & PDM_IF_UF) {
			printLog("PDM UF\r\n");
		}
	}
}

void dump_array(uint8_t *data, int len) {
	uint8_t *temp = data;
	printLog("DATA %d\r\n", len);
	while (len > 0) {
		RETARGET_WriteChar(*temp++);
		len--;
	}
	// printLog("Done\r\n");
}

/* Function to sort an array using insertion sort*/
void insert(int item, int16_t arr[]) {
	int j;
	int i = _data_idx;
	arr[i] = item;

	if (i > 0) {
		j = i - 1;

		/* Move elements of arr[0..i-1], that are
		 greater than key, to one position ahead
		 of their current position */
		while (j >= 0 && arr[j] > item) {
			arr[j + 1] = arr[j];
			j = j - 1;
		}
		arr[j + 1] = item;
	}
}


bool pdm_dma_cb(unsigned int channel, unsigned int sequenceNo, void *userParam) {
	// printf("pdm_ldma_cb\r\n");
	static int offset = 0;
	//  int start;
	// static int index = 0;

	prevBufferPing = !prevBufferPing;
	// start = RTCC_CounterGet();
	if (recording) {
		if (offset < BUFFER_SIZE) {

			if (prevBufferPing) {
				for (int i = 0; i < PP_BUFFER_SIZE; i++) {
					left[i + offset] = pingBuffer[i] & 0x0000FFFF;
					right[i + offset] = (pingBuffer[i] >> 16) & 0x0000FFFF;
				}
			} else {
				for (int i = 0; i < PP_BUFFER_SIZE; i++) {
					left[offset + i] = pongBuffer[i] & 0x0000FFFF;
					right[offset + i] = (pongBuffer[i] >> 16) & 0x0000FFFF;
				}
			}

			offset += PP_BUFFER_SIZE;
		}
		// printLog("dma_cb: channel %d, sequenceNo %d, offset %d\r\n", channel, sequenceNo, offset);
		// times[index++] = RTCC_CounterGet() - start;
		if (offset == BUFFER_SIZE) {
#ifdef PDM_INTERRUPTS
			PDM->IEN &= ~(PDM_IEN_UF | PDM_IEN_OF); //disable interrupts
#endif
			// printLog("dma_cb: channel %d, sequenceNo %d, offset %d\r\n", channel, sequenceNo, offset);
			offset = 0;
			// index = 0;
			DMADRV_StopTransfer(channel);
			recording = false;

#ifdef GOERTZEL
			uint32_t t0 = RTCC_CounterGet();
			float P_left = goertzel(left, k_goertzel);
			float P_right =  goertzel(right, k_goertzel);

			uint32_t curr = RTCC_CounterGet();
			printf("%ld, %ld, %ld, %d ----:g[%ld]: %e, %e\r\n", curr, curr-prev_rtcc, curr-t0, sequenceNo,
					k_goertzel, P_left, P_right);
			prev_rtcc = curr;
			if ((P_left > 2e3) || (P_right > 2e3)) {

				calc_chirp_v2(k_goertzel, pulse_width, pdm_template,
						&N_pdm_template);

				curr = RTCC_CounterGet();
//				printf("%ld, %ld, %ld, **** calc_chirp\r\n",
//						curr, curr-prev_rtcc, curr-t0);
//				prev_rtcc = curr;
				filter_biquad(left, 0);
//				curr = RTCC_CounterGet();
//				printf("%ld, %ld, %ld, **** filter_biquad\r\n",
//						curr, curr-prev_rtcc, curr-t0);
//				prev_rtcc = curr;
				memset(corr, 0, BUFFER_SIZE << 1);
				calc_cross(left, BUFFER_SIZE, pdm_template, N_pdm_template,
						corr);
//				curr = RTCC_CounterGet();
//				printf("%ld, %ld, %ld, **** calc_cross\r\n",
//						curr, curr-prev_rtcc, curr-t0);
//				prev_rtcc = curr;
				int l_t = 0;
				int r_t = 0;
				int16_t l_pk = 0;
				int16_t r_pk = 0;
				// int l_t = shape(corr);
				shape_v2(corr, &l_pk, &l_t);
//				curr = RTCC_CounterGet();
//				printf("%ld, %ld, %ld, **** shape_v2\r\n",
//						curr, curr-prev_rtcc, curr-t0);
//				prev_rtcc = curr;
//				int p2_l, p2_r;
				// int left_w = width(corr, l_t, 0.5, &p2_l);
				if (out == 2)
					dump_array((uint8_t *) corr, BUFFER_SIZE << 1);

				filter_biquad(right, 0);
				memset(corr, 0, BUFFER_SIZE << 1);
				calc_cross(right, BUFFER_SIZE, pdm_template, N_pdm_template,
						corr);
				// int r_t = shape(corr);
				shape_v2(corr, &r_pk, &r_t);
				// int right_w = width(corr, r_t, 0.5, &p2_r);
				if (out == 2)
					dump_array((uint8_t *) corr, BUFFER_SIZE << 1);
				curr = RTCC_CounterGet();
				if (out == 1)
					dump_array((uint8_t *) left, BUFFER_SIZE << 1);
				if (out == 1)
					dump_array((uint8_t *) right, BUFFER_SIZE << 1);
				memset(left, 0, BUFFER_SIZE << 1);
				memset(right, 0, BUFFER_SIZE << 1);

//			printLog("%ld, %ld ----pk: (%d, %d), (%d, %d) width: %d, %d\r\n",
//					curr, curr-prev_rtcc, left_t, p2_l, right_t, p2_r, left_w, right_w);
				printLog("%ld, %ld ----pk: %d, %d\r\n", curr, curr - prev_rtcc,
						l_t, r_t);
				insert(l_t, left_t);
				insert(r_t, right_t);
				// insert(p2_l, left_t);
				// insert(p2_r, right_t);
				// left_t[_data_idx] = l_t;
				// right_t[_data_idx] = r_t;
				_data_idx++;
				prev_rtcc = curr;
			} else {
				if (out > 0) printLog("DATA 0\r\n");
				if (out > 0) printLog("DATA 0\r\n");

				curr = RTCC_CounterGet();
				printLog("%ld, %ld ----pk: %d, %d\r\n", curr, curr - prev_rtcc,
						0, 0);
				prev_rtcc = curr;
			}
#endif
		}
	}
	return 0;
}

void startLDMA_PDM(void) {
	static LDMA_Descriptor_t descLink[2];
	// static LDMA_TransferCfg_t periTransferTx;
	// PDM FIFO:
	static bool need_setup = true;
	LDMA_TransferCfg_t periTransferTx =
			LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_PDM_RXDATAV);
	if (need_setup) {

		// Link descriptors for ping-pong transfer
		descLink[0] =
				(LDMA_Descriptor_t )
								LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&(PDM->RXDATA), pingBuffer, PP_BUFFER_SIZE, +1);
		descLink[1] =
				(LDMA_Descriptor_t )
								LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&(PDM->RXDATA), pongBuffer, PP_BUFFER_SIZE, -1);
		need_setup = false;
	}
	// Next transfer writes to pingBuffer
	prevBufferPing = false;
//	LDMA_StartTransfer(LDMA_PDM_CHANNEL, (void*) &periTransferTx,
//			(void*) &descLink);

	DMADRV_LdmaStartTransfer(ldma_channelPDM, (void*) &periTransferTx,
			(void*) &descLink, pdm_dma_cb,
			NULL);

}


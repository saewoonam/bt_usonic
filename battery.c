/*
 * battery.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include "em_device.h"
#include "em_cmu.h"
#include "em_iadc.h"


// from https://docs.silabs.com/resources/bluetooth/code-examples/applications/reporting-battery-voltage-over-ble/source/app.c
#if defined(ADC_PRESENT)
/* 5V reference voltage, no attenuation on AVDD, 12 bit ADC data */
  #define ADC_SCALE_FACTOR   (5.0 / 4095.0)
#elif defined(IADC_PRESENT)
/* 1.21V reference voltage, AVDD attenuated by a factor of 4, 12 bit ADC data */
  #define ADC_SCALE_FACTOR   (4.84 / 4095.0)
#endif

/**
 * @brief Initialise ADC. Called after boot in this example.
 */

void adcInit(void)
{
#if defined(ADC_PRESENT)
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;

  CMU_ClockEnable(cmuClock_ADC0, true);
  CMU_ClockEnable(cmuClock_PRS, true);

  /* Only configure the ADC if it is not already running */
  if ( ADC0->CTRL == _ADC_CTRL_RESETVALUE ) {
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(1000000, 0);
    ADC_Init(ADC0, &init);
  }

  initSingle.acqTime = adcAcqTime16;
  initSingle.reference = adcRef5VDIFF;
  initSingle.posSel = adcPosSelAVDD;
  initSingle.negSel = adcNegSelVSS;
  initSingle.prsEnable = true;
  initSingle.prsSel = adcPRSSELCh4;

  ADC_InitSingle(ADC0, &initSingle);
#elif defined(IADC_PRESENT)
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t input = IADC_SINGLEINPUT_DEFAULT;

  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_PRS, true);

  /* Only configure the ADC if it is not already running */
  if ( IADC0->CTRL == _IADC_CTRL_RESETVALUE ) {
    IADC_init(IADC0, &init, &allConfigs);
  }

  input.posInput = iadcPosInputAvdd;

  IADC_initSingle(IADC0, &initSingle, &input);
  IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);
#endif
  return;
}


/***************************************************************************//**
 * @brief
 *    Initiates an A/D conversion and reads the sample when done
 *
 * @return
 *    The output of the A/D conversion
 ******************************************************************************/
static uint16_t getAdcSample(void)
{
#if defined(ADC_PRESENT)
  ADC_Start(ADC0, adcStartSingle);
  while ( !(ADC_IntGet(ADC0) & ADC_IF_SINGLE) )
    ;

  return ADC_DataSingleGet(ADC0);
#elif defined(IADC_PRESENT)
  /* Clear single done interrupt */
  IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);

  /* Start conversion and wait for result */
  IADC_command(IADC0, IADC_CMD_SINGLESTART);
  while ( !(IADC_getInt(IADC0) & IADC_IF_SINGLEDONE) ) ;

  return IADC_readSingleData(IADC0);
#endif
}


/***************************************************************************//**
 * @brief
 *    Measures the supply voltage by averaging multiple readings
 *
 * @param[in] avg
 *    Number of measurements to average
 *
 * @return
 *    The measured voltage
 ******************************************************************************/
float SUPPLY_measureVoltage(unsigned int avg)
{
  uint16_t adcData;
  float supplyVoltage;
  int i;

  adcInit();

  supplyVoltage = 0;

  for ( i = 0; i < avg; i++ ) {
    adcData = getAdcSample();
    supplyVoltage += (float) adcData * ADC_SCALE_FACTOR;
  }

  supplyVoltage = supplyVoltage / (float) avg;

  return supplyVoltage;
}

typedef struct {
  float         voltage;
  uint8_t       capacity;
} VoltageCapacityPair;

static VoltageCapacityPair battCR2032Model[] =
{ { 3.0, 100 }, { 2.9, 80 }, { 2.8, 60 }, { 2.7, 40 }, { 2.6, 30 },
  { 2.5, 20 }, { 2.4, 10 }, { 2.0, 0 } };

static uint8_t battBatteryLevel; /* Battery Level */

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/
// static uint8_t readBatteryLevel(void);

static uint8_t calculateLevel(float voltage, VoltageCapacityPair *model, uint8_t modelEntryCount)
{
  uint8_t res = 0;

  if (voltage >= model[0].voltage) {
    /* Return with max capacity if voltage is greater than the max voltage in the model */
    res = model[0].capacity;
  } else if (voltage <= model[modelEntryCount - 1].voltage) {
    /* Return with min capacity if voltage is smaller than the min voltage in the model */
    res = model[modelEntryCount - 1].capacity;
  } else {
    uint8_t i;
    /* Otherwise find the 2 points in the model where the voltage level fits in between,
       and do linear interpolation to get the estimated capacity value */
    for (i = 0; i < (modelEntryCount - 1); i++) {
      if ((voltage < model[i].voltage) && (voltage >= model[i + 1].voltage)) {
        res = (uint8_t)((voltage - model[i + 1].voltage)
                        * (model[i].capacity - model[i + 1].capacity)
                        / (model[i].voltage - model[i + 1].voltage));
        res += model[i + 1].capacity;
        break;
      }
    }
  }

  return res;
}


// static uint8_t readBatteryLevel(void)
void readBatteryLevel(void)
{
  float voltage = SUPPLY_measureVoltage(1);
  battBatteryLevel = calculateLevel(voltage, battCR2032Model, sizeof(battCR2032Model) / sizeof(VoltageCapacityPair));
  // uint8_t level = calculateLevel(voltage, battCR2032Model, sizeof(battCR2032Model) / sizeof(VoltageCapacityPair));

  //return level;
}

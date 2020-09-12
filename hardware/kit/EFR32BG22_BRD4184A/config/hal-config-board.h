#ifndef HAL_CONFIG_BOARD_H
#define HAL_CONFIG_BOARD_H

#include "em_device.h"
#include "hal-config-types.h"

// This file is auto-generated by Hardware Configurator in Simplicity Studio.
// Any content between $[ and ]$ will be replaced whenever the file is regenerated.
// Content outside these regions will be preserved.

// $[ANTDIV]
// [ANTDIV]$

// $[BTL_BUTTON]

#define BSP_BTL_BUTTON_PIN                   (1U)
#define BSP_BTL_BUTTON_PORT                  (gpioPortB)

// [BTL_BUTTON]$

// $[BUTTON]
#define BSP_BUTTON_PRESENT                   (1)

#define BSP_BUTTON0_PIN                      (1U)
#define BSP_BUTTON0_PORT                     (gpioPortB)

#define BSP_BUTTON_COUNT                     (1U)
#define BSP_BUTTON_INIT                      { { BSP_BUTTON0_PORT, BSP_BUTTON0_PIN } }
#define BSP_BUTTON_GPIO_DOUT                 (HAL_GPIO_DOUT_LOW)
#define BSP_BUTTON_GPIO_MODE                 (HAL_GPIO_MODE_INPUT)
// [BUTTON]$

// $[CMU]
#define BSP_CLK_HFXO_PRESENT                 (1)
#define BSP_CLK_HFXO_FREQ                    (38400000UL)
#define BSP_CLK_HFXO_INIT                     CMU_HFXOINIT_DEFAULT
#define BSP_CLK_HFXO_CTUNE                   (120)
#define BSP_CLK_LFXO_PRESENT                 (1)
#define BSP_CLK_LFXO_INIT                     CMU_LFXOINIT_DEFAULT
#define BSP_CLK_LFXO_FREQ                    (32768U)
#define BSP_CLK_LFXO_CTUNE                   (37U)
// [CMU]$

// $[COEX]
// [COEX]$

// $[DCDC]
#define BSP_DCDC_PRESENT                     (1)

#define BSP_DCDC_INIT                         EMU_DCDCINIT_DEFAULT
// [DCDC]$

// $[EMU]
// [EMU]$

// $[EUART0]
#define PORTIO_EUART0_RX_PIN                 (6U)
#define PORTIO_EUART0_RX_PORT                (gpioPortA)

#define PORTIO_EUART0_TX_PIN                 (5U)
#define PORTIO_EUART0_TX_PORT                (gpioPortA)

#define BSP_EUART0_TX_PIN                    (5U)
#define BSP_EUART0_TX_PORT                   (gpioPortA)

#define BSP_EUART0_RX_PIN                    (6U)
#define BSP_EUART0_RX_PORT                   (gpioPortA)

// [EUART0]$

// $[EXTFLASH]
#define BSP_EXTFLASH_CS_PIN                  (3U)
#define BSP_EXTFLASH_CS_PORT                 (gpioPortC)

#define BSP_EXTFLASH_INTERNAL                (0)
#define BSP_EXTFLASH_USART                   (HAL_SPI_PORT_USART0)
#define BSP_EXTFLASH_MOSI_PIN                (0U)
#define BSP_EXTFLASH_MOSI_PORT               (gpioPortC)

#define BSP_EXTFLASH_MISO_PIN                (1U)
#define BSP_EXTFLASH_MISO_PORT               (gpioPortC)

#define BSP_EXTFLASH_CLK_PIN                 (2U)
#define BSP_EXTFLASH_CLK_PORT                (gpioPortC)

// [EXTFLASH]$

// $[EZRADIOPRO]
// [EZRADIOPRO]$

// $[FEM]
// [FEM]$

// $[GPIO]
#define PORTIO_GPIO_SWV_PIN                  (3U)
#define PORTIO_GPIO_SWV_PORT                 (gpioPortA)

#define BSP_TRACE_SWO_PIN                    (3U)
#define BSP_TRACE_SWO_PORT                   (gpioPortA)

// [GPIO]$

// $[I2C0]
#define PORTIO_I2C0_SCL_PIN                  (3U)
#define PORTIO_I2C0_SCL_PORT                 (gpioPortD)

#define PORTIO_I2C0_SDA_PIN                  (2U)
#define PORTIO_I2C0_SDA_PORT                 (gpioPortD)

#define BSP_I2C0_SCL_PIN                     (3U)
#define BSP_I2C0_SCL_PORT                    (gpioPortD)

#define BSP_I2C0_SDA_PIN                     (2U)
#define BSP_I2C0_SDA_PORT                    (gpioPortD)

// [I2C0]$

// $[I2C1]
// [I2C1]$

// $[I2CSENSOR]

#define BSP_I2CSENSOR_ENABLE_PIN             (4U)
#define BSP_I2CSENSOR_ENABLE_PORT            (gpioPortA)

#define BSP_I2CSENSOR_PERIPHERAL             (HAL_I2C_PORT_I2C0)
#define BSP_I2CSENSOR_SCL_PIN                (3U)
#define BSP_I2CSENSOR_SCL_PORT               (gpioPortD)

#define BSP_I2CSENSOR_SDA_PIN                (2U)
#define BSP_I2CSENSOR_SDA_PORT               (gpioPortD)

// [I2CSENSOR]$

// $[IADC0]
// [IADC0]$

// $[IOEXP]
// [IOEXP]$

// $[LED]
#define BSP_LED_PRESENT                      (1)

#define BSP_LED0_PIN                         (0U)
#define BSP_LED0_PORT                        (gpioPortB)

#define BSP_LED_COUNT                        (1U)
#define BSP_LED_INIT                         { { BSP_LED0_PORT, BSP_LED0_PIN } }
#define BSP_LED_POLARITY                     (1)
// [LED]$

// $[LETIMER0]
// [LETIMER0]$

// $[LFXO]
// [LFXO]$

// $[MODEM]
// [MODEM]$

// $[PA]

#define BSP_PA_VOLTAGE                       (1800U)
// [PA]$

// $[PDM]
#define PORTIO_PDM_CLK_PIN                   (6U)
#define PORTIO_PDM_CLK_PORT                  (gpioPortC)

#define PORTIO_PDM_DAT0_PIN                  (7U)
#define PORTIO_PDM_DAT0_PORT                 (gpioPortC)

#define BSP_PDM_CLK_PIN                      (6U)
#define BSP_PDM_CLK_PORT                     (gpioPortC)

#define BSP_PDM_DAT0_PIN                     (7U)
#define BSP_PDM_DAT0_PORT                    (gpioPortC)

// [PDM]$

// $[PORTIO]
// [PORTIO]$

// $[PRS]
// [PRS]$

// $[PTI]
#define PORTIO_PTI_DFRAME_PIN                (5U)
#define PORTIO_PTI_DFRAME_PORT               (gpioPortC)

#define PORTIO_PTI_DOUT_PIN                  (4U)
#define PORTIO_PTI_DOUT_PORT                 (gpioPortC)

#define BSP_PTI_DFRAME_PIN                   (5U)
#define BSP_PTI_DFRAME_PORT                  (gpioPortC)

#define BSP_PTI_DOUT_PIN                     (4U)
#define BSP_PTI_DOUT_PORT                    (gpioPortC)

// [PTI]$

// $[SERIAL]
#define BSP_SERIAL_APP_PORT                  (HAL_SERIAL_PORT_USART1)
#define BSP_SERIAL_APP_TX_PIN                (5U)
#define BSP_SERIAL_APP_TX_PORT               (gpioPortA)

#define BSP_SERIAL_APP_RX_PIN                (6U)
#define BSP_SERIAL_APP_RX_PORT               (gpioPortA)

#define BSP_SERIAL_APP_CTS_PIN               (8U)
#define BSP_SERIAL_APP_CTS_PORT              (gpioPortA)

#define BSP_SERIAL_APP_RTS_PIN               (7U)
#define BSP_SERIAL_APP_RTS_PORT              (gpioPortA)

// [SERIAL]$

// $[SPIDISPLAY]
// [SPIDISPLAY]$

// $[SPINCP]
#define BSP_SPINCP_NHOSTINT_PIN              (0U)
#define BSP_SPINCP_NHOSTINT_PORT             (gpioPortB)

#define BSP_SPINCP_NWAKE_PIN                 (1U)
#define BSP_SPINCP_NWAKE_PORT                (gpioPortB)

#define BSP_SPINCP_USART_PORT                (HAL_SPI_PORT_USART0)
#define BSP_SPINCP_MOSI_PIN                  (0U)
#define BSP_SPINCP_MOSI_PORT                 (gpioPortC)

#define BSP_SPINCP_MISO_PIN                  (1U)
#define BSP_SPINCP_MISO_PORT                 (gpioPortC)

#define BSP_SPINCP_CLK_PIN                   (2U)
#define BSP_SPINCP_CLK_PORT                  (gpioPortC)

#define BSP_SPINCP_CS_PIN                    (3U)
#define BSP_SPINCP_CS_PORT                   (gpioPortC)

// [SPINCP]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[TIMER2]
// [TIMER2]$

// $[TIMER3]
// [TIMER3]$

// $[TIMER4]
// [TIMER4]$

// $[UARTNCP]
#define BSP_UARTNCP_USART_PORT               (HAL_SERIAL_PORT_USART1)
#define BSP_UARTNCP_TX_PIN                   (5U)
#define BSP_UARTNCP_TX_PORT                  (gpioPortA)

#define BSP_UARTNCP_RX_PIN                   (6U)
#define BSP_UARTNCP_RX_PORT                  (gpioPortA)

#define BSP_UARTNCP_CTS_PIN                  (8U)
#define BSP_UARTNCP_CTS_PORT                 (gpioPortA)

#define BSP_UARTNCP_RTS_PIN                  (7U)
#define BSP_UARTNCP_RTS_PORT                 (gpioPortA)

// [UARTNCP]$

// $[USART0]
#define PORTIO_USART0_CLK_PIN                (2U)
#define PORTIO_USART0_CLK_PORT               (gpioPortC)

#define PORTIO_USART0_CS_PIN                 (3U)
#define PORTIO_USART0_CS_PORT                (gpioPortC)

#define PORTIO_USART0_RX_PIN                 (1U)
#define PORTIO_USART0_RX_PORT                (gpioPortC)

#define PORTIO_USART0_TX_PIN                 (0U)
#define PORTIO_USART0_TX_PORT                (gpioPortC)

#define BSP_USART0_MOSI_PIN                  (0U)
#define BSP_USART0_MOSI_PORT                 (gpioPortC)

#define BSP_USART0_MISO_PIN                  (1U)
#define BSP_USART0_MISO_PORT                 (gpioPortC)

#define BSP_USART0_CLK_PIN                   (2U)
#define BSP_USART0_CLK_PORT                  (gpioPortC)

#define BSP_USART0_CS_PIN                    (3U)
#define BSP_USART0_CS_PORT                   (gpioPortC)

// [USART0]$

// $[USART1]
#define PORTIO_USART1_CTS_PIN                (8U)
#define PORTIO_USART1_CTS_PORT               (gpioPortA)

#define PORTIO_USART1_RTS_PIN                (7U)
#define PORTIO_USART1_RTS_PORT               (gpioPortA)

#define PORTIO_USART1_RX_PIN                 (6U)
#define PORTIO_USART1_RX_PORT                (gpioPortA)

#define PORTIO_USART1_TX_PIN                 (5U)
#define PORTIO_USART1_TX_PORT                (gpioPortA)

#define BSP_USART1_TX_PIN                    (5U)
#define BSP_USART1_TX_PORT                   (gpioPortA)

#define BSP_USART1_RX_PIN                    (6U)
#define BSP_USART1_RX_PORT                   (gpioPortA)

#define BSP_USART1_CTS_PIN                   (8U)
#define BSP_USART1_CTS_PORT                  (gpioPortA)

#define BSP_USART1_RTS_PIN                   (7U)
#define BSP_USART1_RTS_PORT                  (gpioPortA)

// [USART1]$

// $[VCOM]
// [VCOM]$

// $[VUART]
// [VUART]$

// $[WDOG]
// [WDOG]$

// $[Custom pin names]
#define MIC_ENABLE_PIN                       (0U)
#define MIC_ENABLE_PORT                      (gpioPortA)

#define IMU_CS_PIN                           (2U)
#define IMU_CS_PORT                          (gpioPortB)

#define IMU_INT_PIN                          (3U)
#define IMU_INT_PORT                         (gpioPortB)

#define IMU_ENABLE_PIN                       (4U)
#define IMU_ENABLE_PORT                      (gpioPortB)

#define MIC_CLK_PIN                          (6U)
#define MIC_CLK_PORT                         (gpioPortC)

#define MIC_DATA_PIN                         (7U)
#define MIC_DATA_PORT                        (gpioPortC)

// [Custom pin names]$

#if defined(_SILICON_LABS_MODULE)
#include "sl_module.h"
#endif

#endif /* HAL_CONFIG_BOARD_H */

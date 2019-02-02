#ifndef CONF_GSM_H_
#define CONF_GSM_H_

/** DTR pin of the GSM module to exit sleep mode. */
#define GSM_DTR_PIN                    PIN_PA20

#define GSM_RING_PIN					PIN_PB17
#define GSM_RING_EIC_PIN				PIN_PB17A_EIC_EXTINT1  
#define GSM_RING_EIC_LINE				1
#define GSM_RING_EIC_MUX				MUX_PB17A_EIC_EXTINT1 

/** Active level of the DTR pin to exit sleep mode. */
#define GSM_DTR_PIN_ACTIVE             true


/** \name GSM SERCOM USART configuration
 *  @{
 */
#define GSM_SERCOM                     SERCOM3
#define GSM_BAUDRATE				   115200
#define GSM_SERCOM_MUX                 USART_RX_1_TX_0_XCK_1
#define GSM_SERCOM_PAD0_MUX            PINMUX_PA22C_SERCOM3_PAD0
#define GSM_SERCOM_PAD1_MUX            PINMUX_PA23C_SERCOM3_PAD1
#define GSM_SERCOM_PAD2_MUX            PINMUX_UNUSED
#define GSM_SERCOM_PAD3_MUX            PINMUX_UNUSED

#endif /* CONF_GSM_H_ */
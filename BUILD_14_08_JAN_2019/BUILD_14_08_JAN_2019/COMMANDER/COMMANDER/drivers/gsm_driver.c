#include "gsm_driver.h"

static void gsm_ring_detect_pin_callback(void);

#define MAX_BUFFER_TMP	240

static QueueHandle_t gsm_rx_queue;

/** Timer to time out processing of commands in the GSM module */
static TimerHandle_t gsm_cmd_timeout_timer;

/** Semaphore to signal a busy GSM module while processing a command */
static SemaphoreHandle_t gsm_busy_semaphore;


/** FreeRTOS timer callback function, fired when the a timer period has elapsed.
 *
 *  \param[in]  timer  ID of the timer that has expired.
 */
static void gsm_timer_callback(TimerHandle_t timer)
{
}

void Flush_RX_Buffer(void)
{
	uint8_t ucharTemp_Value;
	portBASE_TYPE xStatus;
	while(1)
	{
		xStatus=xQueueReceive(gsm_rx_queue,&ucharTemp_Value,0);
		if (xStatus==errQUEUE_EMPTY)
		{
			break;
		}
	}
}

static void gsm_rx_handler(uint8_t instance)
{
	SercomUsart *const usart_hw = &GSM_SERCOM->USART;
	UNUSED(instance);
	if (usart_hw->INTFLAG.reg & SERCOM_USART_INTFLAG_RXC) 
	{
		/* Check if a data reception error occurred */
		uint8_t rx_error = usart_hw->STATUS.reg &
		(SERCOM_USART_STATUS_FERR | SERCOM_USART_STATUS_BUFOVF);
		/* If error occurred clear the error flags, otherwise queue new data */
		if (rx_error) 
		{
			usart_hw->STATUS.reg = rx_error;
		} 
		else
		{
			uint8_t data = (usart_hw->DATA.reg & SERCOM_USART_DATA_MASK);
			xQueueSendFromISR(gsm_rx_queue, &data, NULL);
		}
	}
}

static void gsm_ring_detect_pin_callback(void)
{
	
}

void gsm_init(void)	
{
	struct port_config gsm_pin_config;
	port_get_config_defaults(&gsm_pin_config);
	
	/* Configure pin to control the GSM module sleep state */
	gsm_pin_config.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(GSM_DTR_PIN, &gsm_pin_config);
	port_pin_set_output_level(GSM_DTR_PIN, !GSM_DTR_PIN_ACTIVE);
	
	//////////////////////////////////////////////////////////////////////////
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = GSM_RING_EIC_PIN;
	config_extint_chan.gpio_pin_mux = GSM_RING_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
	extint_chan_set_config(GSM_RING_EIC_LINE, &config_extint_chan);
		
	extint_chan_enable_callback(GSM_RING_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(gsm_ring_detect_pin_callback,GSM_RING_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	//////////////////////////////////////////////////////////////////////////
	
	gsm_rx_queue = xQueueCreate(240, sizeof(uint8_t));
	gsm_cmd_timeout_timer = xTimerCreate((const char *)"GSM Timeout",GSM_TIMEOUT_PERIOD_TICKS, pdFALSE, NULL, gsm_timer_callback);
	vSemaphoreCreateBinary(gsm_busy_semaphore);
	
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate		= GSM_BAUDRATE;
	config_usart.mux_setting	= GSM_SERCOM_MUX;
	config_usart.pinmux_pad0	= GSM_SERCOM_PAD0_MUX;
	config_usart.pinmux_pad1	= GSM_SERCOM_PAD1_MUX;
	config_usart.pinmux_pad2	= GSM_SERCOM_PAD2_MUX;
	config_usart.pinmux_pad3	= GSM_SERCOM_PAD3_MUX;
	config_usart.run_in_standby = true;
	while (usart_init(&gsm_usart,GSM_SERCOM, &config_usart) != STATUS_OK)
	{
		usart_reset(&gsm_usart);
	}
	usart_enable(&gsm_usart);
	_sercom_set_handler(_sercom_get_sercom_inst_index(GSM_SERCOM),gsm_rx_handler);
	GSM_SERCOM->USART.INTENSET.reg=SERCOM_USART_INTFLAG_RXC;
}

enum gsm_error gsm_send_at_command(const char *const atcommand,const char* aResponExit,const uint32_t aTimeoutMax,const uint8_t aLenOut, char *aResponOut)
{
	
	/* Try to acquire the command lock; if already busy with a command, abort */
	if (xSemaphoreTake(gsm_busy_semaphore, 1) == pdFALSE) 
	{
		return GSM_ERROR_OPERATION_IN_PROGRESS;
	}
	
	/* Enable DTR and wait for the module to be ready to accept a command */
	//port_pin_set_output_level(GSM_DTR_PIN, GSM_DTR_PIN_ACTIVE);
	//vTaskDelay(100 / portTICK_PERIOD_MS);
	
	//////////////////////////////////////////////////////////////////////////
	Flush_RX_Buffer();
	//////////////////////////////////////////////////////////////////////////
	enum gsm_error err_no=GSM_ERROR_NONE;
	
	uint8_t u8tRx_Index=0;
	char u8tTemp_Char=0;
	portBASE_TYPE xStatus;

	char *aDataBuffer = (char*) calloc(MAX_BUFFER_TMP,sizeof(char));
	
	//buffer created???
	if (aDataBuffer == NULL)
	{
		//port_pin_set_output_level(GSM_DTR_PIN, !GSM_DTR_PIN_ACTIVE);
		return 0;
	}

	//reset to all 0
	memset(aDataBuffer, '\0', MAX_BUFFER_TMP);
	
	/* Send the command to the GSM module when it is ready */
	usart_write_buffer_wait(&gsm_usart, (uint8_t *)atcommand, strlen(atcommand));	
	
	/* Start the timeout timer to ensure a timely response from the module */
	xTimerChangePeriod(gsm_cmd_timeout_timer,(aTimeoutMax / portTICK_PERIOD_MS),portMAX_DELAY);
	
	while (true)
	{
		if(xTimerIsTimerActive(gsm_cmd_timeout_timer))
		{
			if (u8tRx_Index<240)
			{
				xStatus=xQueueReceive(gsm_rx_queue,&u8tTemp_Char, 0);
				if(xStatus!=errQUEUE_EMPTY)
				{
					aDataBuffer[u8tRx_Index] = u8tTemp_Char;
					u8tRx_Index++;
				}
			}
			if (aResponExit != NULL)
			{
				if (strstr((const char*)aDataBuffer, (const char*)aResponExit) != NULL)
				{
					err_no = GSM_ERROR_NONE;
					break;
				}
			}
			if (strstr((const char*)aDataBuffer, (const char*)RESPONS_ERROR) != NULL)
			{
				err_no = GSM_ERROR_COMMAND_ERROR;
				break;
			}
		}
		else
		{
			xTimerStop(gsm_cmd_timeout_timer, portMAX_DELAY);
			if (u8tRx_Index==0)
			{
				err_no = GSM_ERROR_TIMEOUT;
				break;
			}
			else
			{
				err_no = GSM_ERROR_UNKWON;
				break;
			}
		}
	}
	
	//copy it to the out
	if ((aLenOut != 0) && (aResponOut != NULL) && (aLenOut > u8tRx_Index) && (err_no==GSM_ERROR_NONE))
	{
		memcpy(aResponOut, aDataBuffer, u8tRx_Index *sizeof(uint8_t));
	}
	
	//port_pin_set_output_level(GSM_DTR_PIN, !GSM_DTR_PIN_ACTIVE);
	
	xSemaphoreGive(gsm_busy_semaphore);
	free(aDataBuffer);
	return err_no;
}


enum gsm_error gsm_check_module(void)
{
	return gsm_send_at_command((const char*)("AT\r"),(const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_disable_data_flow_control(void)
{
	return gsm_send_at_command((const char*)("AT+IFC=0,0\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_is_network_registered(void)
{
	enum gsm_error errHomeNw;
	enum gsm_error errRomNw;
	errHomeNw = gsm_send_at_command((const char*)("AT+CREG?\r"), (const char*)"+CREG: 0,1",5000,0, NULL);
	if (errHomeNw==GSM_ERROR_NONE)
	{
		return GSM_NETWORK_REGISTERED;	
	}
	else
	{
		errRomNw = gsm_send_at_command((const char*)("AT+CREG?\r"), (const char*)"+CREG: 0,5",5000,0, NULL);
		{
			if (errRomNw==GSM_ERROR_NONE)
			{
				return GSM_NETWORK_REGISTERED;
			}
			else
			{
				return GSM_NETWORK_NOT_REGISTERED;
			}
		}
	}
}

enum gsm_error gsm_set_baudrate(void)
{
	char baurate_at_command[20]={0};
		
	sprintf(baurate_at_command, "AT+IPR=%d\r",GSM_BAUDRATE);
	
	return gsm_send_at_command((const char*)(baurate_at_command), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_set_auto_baudrate(void)
{
	return gsm_send_at_command((const char*)("AT+IPR=0\r"), (const char*)RESPONS_OK,5000,0, NULL);
}


enum gsm_error gsm_set_network_registration(void)
{
	return gsm_send_at_command((const char*)("AT+CREG=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_set_phone_full_functionality(void)
{
	return gsm_send_at_command((const char*)("AT+CFUN=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_check_phone_full_functionality(void)
{
	return gsm_send_at_command((const char*)("AT+CFUN?\r"), (const char*)"+CFUN: 1",10000,0, NULL);
}

enum gsm_error gsm_set_phone_minimum_functionality(void)
{
	return gsm_send_at_command((const char*)("AT+CFUN=0\r"), (const char*)RESPONS_OK,10000,0, NULL);
}

enum gsm_error gsm_check_phone_minimum_functionality(void)
{
	return gsm_send_at_command((const char*)("AT+CFUN?\r"), (const char*)"+CFUN: 0",10000,0, NULL);
}

enum gsm_error gsm_software_reset(void)
{
	return gsm_send_at_command((const char*)("AT+CFUN=1,1\r"), (const char*)RESPONS_OK,10000,0, NULL);
}

enum gsm_error gsm_detect_simcard(void)
{
	return gsm_send_at_command((const char*)("AT+CPIN?\r"), (const char*)"+CPIN: READY",5000,0, NULL);
}

enum gsm_error gsm_delete_all_sms(void)
{
	return gsm_send_at_command((const char*)("AT+CMGDA=\"DEL ALL\"\r"), (const char*)RESPONS_OK,25000,0, NULL);
}

enum gsm_error gsm_store_active_profile(void)
{
	return gsm_send_at_command((const char*)("AT&W\r"), (const char*)RESPONS_OK,5000,0, NULL);
}


enum gsm_error gsm_enable_calling_line_identification(void)
{
	return gsm_send_at_command((const char*)("AT+CLIP=1\r"), (const char*)RESPONS_OK,15000,0, NULL);
}

enum gsm_error gsm_enable_connected_line_identification_presentation(void)
{
	return gsm_send_at_command((const char*)("AT+COLP=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}


//The +CSCLK value can not be reset by AT&F or ATZ command.
enum gsm_error gsm_enable_sleep_mode(void)
{
	return gsm_send_at_command((const char*)("AT+CSCLK=1\r"), (const char*)RESPONS_OK,10000,0, NULL);
}


enum gsm_error gsm_check_connected_line_identification_presentation(void)
{
	uint8_t line_identification;
	
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_UNKNOWN;;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	if(gsm_send_at_command((const char*)("AT+COLP?\r"), (const char*)RESPONS_OK,5000,MAX_BUFFER, cmdx)==GSM_ERROR_NONE)
	{
		char *strig_cmp;

		strig_cmp = strstr(cmdx,"+COLP");
		
		if (strig_cmp!=NULL)
		{
			char *ptr_tocken;
			ptr_tocken = strtok(cmdx,":");
			ptr_tocken = strtok(NULL,":");
			ptr_tocken = strtok(ptr_tocken,",");
			RemoveSpaces(ptr_tocken);
			line_identification = atoi(ptr_tocken);
			if (line_identification == 0)
			{
				free(cmdx);
				return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_DISABLE;
			}
			else if (line_identification == 1)
			{
				free(cmdx);
				return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_ENABLE;
			}
			else
			{
				free(cmdx);
				return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_UNKNOWN;
			}
		}
		else
		{
			free(cmdx);
			return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_UNKNOWN;
		}
	}
	else
	{
		free(cmdx);
		return GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_UNKNOWN;
	}
}


enum gsm_error gsm_enable_list_current_calls_of_ME(void)
{
	return gsm_send_at_command((const char*)("AT+CLCC=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_factory_reset(void)
{
	return gsm_send_at_command((const char*)("AT&F\r"), (const char*)RESPONS_OK,15000,0, NULL);
}

enum gsm_error gsm_echo_off(void)
{
	return gsm_send_at_command((const char*)("ATE0\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_select_sms_message_formate_text_mode(void) //PDU:0,TEXT:1
{
	return gsm_send_at_command((const char*)("AT+CMGF=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_set_sms_text_mode_parameter(void)
{
	return gsm_send_at_command((const char*)("AT+CSMP=17,167,0,0\r"), (const char*)RESPONS_OK,7000,0, NULL);
}

enum gsm_error gsm_save_sms_settings_in_profile_0(void)
{
	return gsm_send_at_command((const char*)("AT+CSAS=0\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_save_sms_settings_in_profile_1(void)
{
	return gsm_send_at_command((const char*)("AT+CSAS=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

//return signal strenth Value between 0 to 5
// 0 - No network 
// 5 - FULL network
 
 /*
 0 -115 dBm or less
 1 -111 dBm
 2...30 -110... -54 dBm
 31 -52 dBm or greater
 99 not known or not detectable
 
 ////
 
 2--7     1
 8--13    2
 14--19   3
 20--25   4
 26--31   5
 
 */
uint8_t gsm_getsignalstrength(void)
{
	
	uint8_t sig_strength;
	
	const uint8_t MAX_BUFFER = 100;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	enum gsm_error gsm_err =  gsm_send_at_command((const char*)("AT+CSQ\r"), (const char*)RESPONS_OK,5000,MAX_BUFFER,cmdx);
	
	if(gsm_err == GSM_ERROR_NONE)
	{
		if (strstr(cmdx,"99")==NULL)
		{
			char *ptr_tocken;
			ptr_tocken = strtok(cmdx,":");
			ptr_tocken = strtok(NULL,":");
			ptr_tocken = strtok(ptr_tocken,",");
			RemoveSpaces(ptr_tocken);
			uint8_t nw = atoi(ptr_tocken);
			if (nw==0 || nw==1 || nw== 99)
			{
				sig_strength = 0;
			}
			else
			{
				if (nw>=2 && nw<=7)
				{
					sig_strength = 1;
				}
				else if (nw>=8 && nw<=13)
				{
					sig_strength = 2;
				}
				else if (nw>=14 && nw<=19)
				{
					sig_strength = 3;
				}
				else if (nw>=20 && nw<=25)
				{
					sig_strength = 4;
				}
				else if (nw>=26 && nw<=31)
				{
					sig_strength = 5;
				}
				else
				{
					sig_strength = 0;
				}
			}
		}
		else
		{
			sig_strength = 0;
		}
	}
	else
	{
		sig_strength = 0;
	}
	
	free(cmdx);
	
	return sig_strength;
}

void RemoveSpaces(char* source)
{
	char* i = source;
	char* j = source;
	while(*j != 0)
	{
		*i = *j++;
		if(*i != ' ')
		i++;
	}
	*i = 0;
}

enum gsm_error factory_defined_configuration(void)
{
	return gsm_send_at_command((const char*)("AT&F\r"), (const char*)RESPONS_OK,10000,0,NULL);
}

enum gsm_error gsm_enable_new_sms_message_indications(void)
{
	return gsm_send_at_command((const char*)("AT+CNMI=2,1,0,0,0\r"), (const char*)RESPONS_OK,5000,0,NULL);
}

enum gsm_error gsm_disable_new_sms_message_indications(void)
{
	return gsm_send_at_command((const char*)("AT+CNMI=1,0,0,0,0\r"), (const char*)RESPONS_OK,5000,0,NULL);
}

enum gsm_error gsm_send_sms(const char *phone_number, const char *message)
{
	/* Double-check the message length is acceptable (160 byte max payload) */
	if (strlen(message) > 250) 
	{
		return GSM_ERROR_MESSAGE_LENGTH;
	}
	/* Double-check the recipient phone number length */
	if (strlen(phone_number) < 6) 
	{
		return GSM_ERROR_PHONE_NUMBER_LENGTH;
	}
	
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 250;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER, "AT+CMGS=\"%s\"\r", phone_number);
	if (gsm_send_at_command((const char*)cmdx, (const char*)">",60000, 0, NULL)==GSM_ERROR_NONE)
	{
		memset(cmdx, '\0', MAX_BUFFER);
		
		snprintf((char*)cmdx, MAX_BUFFER, "%s\x1A\x0D",message);
		
	    err = gsm_send_at_command((const char*)cmdx, (const char*)RESPONS_OK,60000, 0, NULL);
		if (err == GSM_ERROR_NONE)
		{
			free(cmdx);
			return GSM_ERROR_NONE;
		}
		else 
		{
			free(cmdx);
			return GSM_ERROR_SMS_SEND_FAILED;
		}
	}
	else
	{
		free(cmdx);
		return GSM_ERROR_SMS_SEND_FAILED;
	}
}


uint8_t gsm_get_sms_index(uint8_t required_sms_status)
{
	uint8_t counted_comma = 0;
	uint8_t sms_index = 0;
	const uint8_t MAX_BUFFER = 230;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	enum gsm_error err;
	
	switch(required_sms_status)
	{
		case SMS_UNREAD:
		err = gsm_send_at_command((const char*)("AT+CMGL=\"REC UNREAD\"\r"), (const char*)RESPONS_OK,25000,MAX_BUFFER,cmdx);
		break;
		
		case  SMS_READ:
		err = gsm_send_at_command((const char*)("AT+CMGL=\"REC READ\"\r"), (const char*)RESPONS_OK,25000,MAX_BUFFER,cmdx);
		break;
		
		case SMS_ALL:
		err = gsm_send_at_command((const char*)("AT+CMGL=\"ALL\"\r"), (const char*)RESPONS_OK,25000,MAX_BUFFER,cmdx);
		break;
		
		default:
		free(cmdx);
		return 0;
	}
	if (err == GSM_ERROR_NONE)
	{
		for(int i=0; cmdx[i]!='\0'; i++)
		{
			if(cmdx[i]==',')
			{
				counted_comma++;
			} 
		}
		/*
		+CMGL: 1,"REC READ","+918140200752","","18/06/26,11:50:27+22"
		Hello

		+CMGL: 2,"REC READ","+918140200752","","18/06/26,11:51:27+22"
		Hello

		+CMGL: 3,"REC READ","+918140200752","","18/06/26,12:02:02+22"
		Hello
		
		
		PER SMS there is 5 COMMA...
		*/
		
		if (counted_comma>0 && (counted_comma%5==0))
		{
			//sms_index = counted_comma / 5;
			char *tocken;
			tocken = strtok(cmdx,",");
			for (uint8_t i=0;i<counted_comma-5;i++)
			{
				tocken = strtok(NULL,",");
			}
			char *cptr = strstr(tocken,"CMGL");
			cptr = strstr(cptr," ");
			
			sms_index =  atoi(cptr);
		}
		else
		{
			sms_index = 0;
		}
		
	}
	else
	{
		sms_index = 0;
	}
	free(cmdx);
	return sms_index;
}

enum gsm_error gsm_read_sms(uint8_t position, char *phone_number, uint8_t max_phone_len, char *SMS_text, uint8_t max_SMS_len)
{
	const uint8_t MAX_BUFFER = 230;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	uint8_t cmgr_at_command[10] = {0};
	
	snprintf((char*)cmgr_at_command, MAX_BUFFER, "AT+CMGR=%d\r",position);
	
	if (gsm_send_at_command((const char*)cmgr_at_command, (const char*)RESPONS_OK,5000, MAX_BUFFER, cmdx)==GSM_ERROR_NONE)
	{
		char *strig_cmp;
		char *p_char;
		char *p_char1;
		strig_cmp = strstr(cmdx,"+CMGR");
		if (strig_cmp != 0)
		{
			p_char = strchr((char *)(cmdx),',');
			p_char1 = p_char+2;
			p_char = strchr((char *)(p_char1),'"');
			uint8_t len;
			if (p_char != NULL)
			{
				*p_char = 0;
				len = strlen(p_char1);
				if(len < max_phone_len)
				{
					strcpy(phone_number, (char *)(p_char1));
				}
				else
				{
					memcpy(phone_number,(char *)p_char1,(max_phone_len-1));
					phone_number[max_phone_len]=0;
				}
			}
			p_char = strchr(p_char+1, 0x0a);
			if (p_char != NULL)
			{
				p_char++;
				p_char1 = strchr((char *)(p_char), 0x0d);
				if (p_char1 != NULL)
				{
					*p_char1 = 0;
				}
				len = strlen(p_char);
				if (len < max_SMS_len)
				{
					strcpy(SMS_text, (char *)(p_char));
				}
				else
				{
					memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
					SMS_text[max_SMS_len] = 0;
				}
			}
		}
		else
		{
			free(cmdx);
			return GSM_ERROR_SMS_NOT_AVAILABLE; 
		}
	}
	else
	{
		free(cmdx);
		return GSM_ERROR_SMS_NOT_AVAILABLE; 
	}
	free(cmdx);
	return GSM_ERROR_NONE; 
}


enum gsm_error gsm_call_to_dial_a_number(const char *to)
{
	const uint8_t MAX_BUFFER = 30;
	enum gsm_error err;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	
	//buffer created?
	if (cmdx == NULL)
	{
		return 0;
	}
	
	//init string
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER, "ATD%s;\r", to);
	
	err = gsm_send_at_command((const char*)cmdx, (const char*)RESPONS_OK,2000,0, NULL);
	free(cmdx);
	return err;
	
}

enum gsm_error gsm_answer_an_incomming_call(void)
{
	return gsm_send_at_command((const char*)("ATA\r"), (const char*)RESPONS_OK,5000,0, NULL);
}


enum gsm_error gsm_enable_network_time_update(void)
{
	return 	gsm_send_at_command((const char*)("AT+CLTS=1\r"), (const char*)RESPONS_OK,10000,0, NULL);
}

enum gsm_error gsm_disable_network_time_update(void)
{
	return 	gsm_send_at_command((const char*)("AT+CLTS=0\r"), (const char*)RESPONS_OK,10000,0, NULL);
}

enum gsm_error gsm_enable_DTMF_detection(void)
{
	return 	gsm_send_at_command((const char*)("AT+DDET=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_send_DTMF_Tone(char *tone)
{
	const uint8_t MAX_BUFFER = 30;
	enum gsm_error err;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	
	//buffer created?
	if (cmdx == NULL)
	{
		return 0;
	}
	
	//init string
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+VTS=\"%s\"\r",tone);
	
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,5000,0, NULL);
	free(cmdx);
	return err;
}

enum gsm_error gsm_hangup_call(void)
{
	return 	gsm_send_at_command((const char*)("AT+CHUP\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_disable_call_waiting(void)
{
	return gsm_send_at_command((const char*)("AT+CCWA=0,0\r"), (const char*)RESPONS_OK,18000,0, NULL);
}


enum gsm_error gsm_reject_all_incomming_calls(void)
{
	return gsm_send_at_command((const char*)("AT+GSMBUSY=1\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_enable_all_incomming_calls(void)
{
	return gsm_send_at_command((const char*)("AT+GSMBUSY=0\r"), (const char*)RESPONS_OK,5000,0, NULL);
}


enum gsm_error gsm_stop_play_record_file(void)
{
	return gsm_send_at_command((const char*)("AT+CREC=5\r"), (const char*)RESPONS_OK,2000,0, NULL);
}

enum gsm_error gsm_play_record_file(const char *filename,bool playInfinitely)
{
	//AT+CREC=4,"C:\User\555.amr",0,100$0D
	
	uint8_t repeat = 0;
	if (playInfinitely)
	{
		repeat = 1;
	}
	
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	snprintf((char*)cmdx, MAX_BUFFER, "AT+CREC=4,\"C:\\User\\%c.amr\",0,90,%u\r",filename,repeat);
	
	gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,2000,0, NULL);
	free(cmdx);
	return GSM_ERROR_NONE;
	
}

enum gsm_error gsm_get_internal_rtc_time(void)
{
	const uint8_t MAX_BUFFER = 70;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	enum gsm_error err;
	err = gsm_send_at_command((const char*)"AT+CCLK?\r", (const char*)RESPONS_OK,5000,MAX_BUFFER, cmdx);
	if(err==GSM_ERROR_NONE)
	{
		char *cclktocken;
		cclktocken = strtok(cmdx,"\"");
		cclktocken = strtok(NULL,"\"");
		
		cclktocken = strtok(cclktocken,"/");
		struct_internal_rtc.Network_year = atoi(cclktocken);
		
		cclktocken = strtok(NULL,"/");
		struct_internal_rtc.Network_month = atoi(cclktocken);
		
		cclktocken = strtok(NULL,",");
		struct_internal_rtc.Network_date = atoi(cclktocken);
		
		cclktocken = strtok(NULL,":");
		struct_internal_rtc.Network_hour = atoi(cclktocken);
		
		cclktocken = strtok(NULL,":");
		struct_internal_rtc.Network_minute = atoi(cclktocken);
		
		cclktocken = strtok(NULL,"+");
		struct_internal_rtc.Network_second = atoi(cclktocken);
	}
	else
	{
		struct_internal_rtc.Network_year = 0;
		struct_internal_rtc.Network_month = 0;
		struct_internal_rtc.Network_date = 0;
		struct_internal_rtc.Network_hour = 0;
		struct_internal_rtc.Network_minute = 0;
		struct_internal_rtc.Network_second = 0;
	}
	
	free(cmdx);
	return err;
}


enum gsm_error gsm_config_module(void)
{
	if (gsm_check_module() == GSM_ERROR_NONE)
	{
		if (gsm_factory_reset() == GSM_ERROR_NONE)
		{
			vTaskDelay(2000/portTICK_PERIOD_MS);
			
			if (gsm_detect_simcard() == GSM_ERROR_NONE)
			{
				if (gsm_echo_off() == GSM_ERROR_NONE)
				{
					if (gsm_set_baudrate() == GSM_ERROR_NONE)
					{
						vTaskDelay(2000/portTICK_PERIOD_MS);
						
						if (gsm_enable_calling_line_identification()==GSM_ERROR_NONE)
						{
							if (gsm_enable_connected_line_identification_presentation() == GSM_ERROR_NONE)
							{
								if (gsm_enable_list_current_calls_of_ME() == GSM_ERROR_NONE)
								{
									if (gsm_select_sms_message_formate_text_mode() == GSM_ERROR_NONE)
									{
										if (gsm_set_sms_text_mode_parameter() == GSM_ERROR_NONE)
										{
											if (gsm_enable_new_sms_message_indications() == GSM_ERROR_NONE)
											{
												if (gsm_enable_network_time_update() == GSM_ERROR_NONE)
												{
													if (gsm_enable_DTMF_detection() == GSM_ERROR_NONE)
													{
														//if (gsm_enable_sleep_mode()==GSM_ERROR_NONE)
														{
															if (gsm_store_active_profile() == GSM_ERROR_NONE)
															{
																return GSM_ERROR_NONE; 
															}
															else
															{
																return GSM_ERROR_CONFIG_FAILED;
															}
														}
														//else
														{
														//	return GSM_ERROR_CONFIG_FAILED;
														}
													}
													else
													{
														return GSM_ERROR_CONFIG_FAILED;
													}
												}
												else
												{
													return GSM_ERROR_CONFIG_FAILED;
												}
											}
											else
											{
												return GSM_ERROR_CONFIG_FAILED;
											}
										}
										else
										{
											return GSM_ERROR_CONFIG_FAILED;
										}
									}
									else
									{
										return GSM_ERROR_CONFIG_FAILED;
									}
								}
								else
								{
									return GSM_ERROR_CONFIG_FAILED;
								}
							}
							else
							{
								return GSM_ERROR_CONFIG_FAILED;
							}
						}
						else
						{
							return GSM_ERROR_CONFIG_FAILED;
						}
					}
					else
					{
						return GSM_ERROR_CONFIG_FAILED;
					}
				}
			}
			else
			{
				return GSM_ERROR_CONFIG_FAILED;
			}
		}
		else
		{
			return GSM_ERROR_CONFIG_FAILED;
		}
	}
	else
	{
		return GSM_ERROR_CONFIG_FAILED;
	}
	
	return GSM_ERROR_CONFIG_FAILED;
}

bool gsm_read_response_line(char *buffer,uint8_t length)
{
	bool line_non_empty = false;
	while (length > 1) 
	{
		uint8_t curr_rx;
		/* Fetch next buffered character received from the module */
		if (xQueueReceive(gsm_rx_queue, &curr_rx, 500 / portTICK_PERIOD_MS) == pdFALSE)
		{
			return false;
		}

		if (curr_rx == '\n') 
		{
			/* Ignore newline characters */
		}
		else if (curr_rx != '\r') 
		{
			/* Non end-of-command CR character */
			*(buffer++) = curr_rx;
			length--;
			line_non_empty = true;
		}
		else
		{
			/* End of command, finished reading line */
			break;
		}
	}

	*(buffer) = '\0';
	
	return line_non_empty;
}

char gsm_responseLine_isNew_SMS_Received(char *response)
{
	
	char sms_index = 0;
	
	if (strstr(response,"+CMTI:"))
	{
		char *ptr_tocken;
		ptr_tocken = strtok(response,",");
		ptr_tocken = strtok(NULL,",");
		
		RemoveSpaces(ptr_tocken);
		
		sms_index = atoi(ptr_tocken);
		
	}
	else
	{
		sms_index =  0;
	}
	
	return sms_index;
}

bool gsm_responseLine_isRinging(char *response)
{
	if (strstr(response,"RING"))
	{
		return true;	
	}
	else
	{
		return false;
	}
}

bool gsm_responseLine_get_IncommingCallNo(char *response,char *phone_number)
{
	if(strstr(response,"+CLIP"))
	{
		  char *ptr_tocken;
		  ptr_tocken = strtok(response,":"); 
		  ptr_tocken = strtok(NULL,",");
		  ptr_tocken = strtok(ptr_tocken,"\"");
		  ptr_tocken = strtok(NULL,"\"");
		  
		  if (!strstr(ptr_tocken,"+"))
		  {
			  return false;
		  }
		  strcpy(phone_number, (char *)(ptr_tocken));		  
		  return true;
	}
	else
	{
		return false;
	}
}

bool gsm_responseLine_isCallCut(char *response)
{
	if (strstr(response,"NO CARRIER"))
	{
		return true;
	}
	else if (strstr(response,"BUSY"))
	{
		return true;
	}
	else if (strstr(response,"NO ANSWER"))
	{
		return true;
	}
	else if (strstr(response,"ERROR"))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool gsm_responseLine_isNew_DTMF_Command_Received(char *response)
{
	if (strstr(response,"+DTMF:"))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool gsm_responseLine_is_StopSound_Received(char *response)
{
	if (strstr(response,"+CREC: 0"))
	{
		return true;
	}
	else
	{
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////
//FTP
//////////////////////////////////////////////////////////////////////////

enum gsm_error gsm_configure_contype_gprs(void)
{
	return gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r"), (const char*)RESPONS_OK,5000,0, NULL);
}

enum gsm_error gsm_configure_bearer_apn(void)
{
	char buffer[100];
	
	uint8_t i = 0;
	char *p, *s;
	const uint8_t MAX_BUFFER = 100;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		return GSM_ERROR_UNKWON;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	gsm_send_at_command((const char*)("AT+COPS?\r"),(const char*)RESPONS_OK,75000,MAX_BUFFER,cmdx);
	
	if (NULL != (s = strstr(cmdx, "+COPS:")))
	{
		s = strstr((char *)(s),"\"");
		s = s + 1;
		p = strstr((char *)(s),"\"");
		if (NULL != s)
		{
			i = 0;
			while (s < p)
			{
				buffer[i++] = *(s++);
			}
			buffer[i] = '\0';
		}
	}
	
	StringtoUpperCase(buffer);
	
	if(strstr(buffer,"IDEA"))
	{
		free(cmdx);
		return 	gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"APN\",\"internet\"\r"), (const char*)RESPONS_OK,3000,0, NULL);
	}
	else if (strstr(buffer,"VODA"))
	{
		free(cmdx);
		return 	gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"APN\",\"www\"\r"), (const char*)RESPONS_OK,3000,0, NULL);
	}
	else if (strstr(buffer,"BSNL"))
	{
		free(cmdx);
		return 	gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"APN\",\"bsnlnet\"\r"), (const char*)RESPONS_OK,3000,0, NULL);
	}
	else if (strstr(buffer,"AIRTEL"))
	{
		free(cmdx);
		return 	gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"\r"), (const char*)RESPONS_OK,3000,0, NULL);
	}
	else
	{
		free(cmdx);
		return 	gsm_send_at_command((const char*)("AT+SAPBR=3,1,\"APN\",\"\"\r"), (const char*)RESPONS_OK,3000,0, NULL);
	}
}

enum gsm_error gsm_query_gprs_contex(void)
{
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	if (cmdx == NULL)
	{
		free(cmdx);
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	
	enum gsm_error err;
	
	err = gsm_send_at_command((const char*)("AT+SAPBR=2,1\r"), (const char*)RESPONS_OK,5000,MAX_BUFFER,cmdx);
	
	if (err == GSM_ERROR_NONE)
	{
		if (strstr(cmdx,"+SAPBR: 1,0") != NULL)
		{
			err = GSM_GPRS_BEARER_IS_CONNECTING;
		}
		else if (strstr(cmdx,"+SAPBR: 1,1") != NULL)
		{
			err = GSM_GPRS_BEARER_IS_CONNECTED;
		}
		else if (strstr(cmdx,"+SAPBR: 1,2") != NULL)
		{
			err = GSM_GPRS_BEARER_IS_CLOSING;
		}
		else if (strstr(cmdx,"+SAPBR: 1,3") != NULL)
		{
			err = GSM_GPRS_BEARER_IS_CLOSED;
		}
		
	}
	
	free(cmdx);
	
	return err;
	
}

enum gsm_error gsm_open_gprs_contex(void)
{
	return gsm_send_at_command((const char*)("AT+SAPBR=1,1\r"), (const char*)RESPONS_OK,85000,0, NULL);
}

enum gsm_error gsm_close_gprs_contex(void)
{
	return gsm_send_at_command((const char*)("AT+SAPBR=0,1\r"), (const char*)RESPONS_OK,65000,0, NULL);
}

enum gsm_error gsm_set_ftp_bearer_profile(void)
{
	return gsm_send_at_command((const char*)("AT+FTPCID=1\r"), (const char*)RESPONS_OK,3000,0, NULL);
}

enum gsm_error gsm_set_ftp_server_address(char *address)
{
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+FTPSERV=\"%s\"\r",address);
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,3000,0,NULL);
	free(cmdx);
	return err;
}

enum gsm_error gsm_set_ftp_user_name(char *username)
{
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+FTPUN=\"%s\"\r",username);
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,3000,0,NULL);
	free(cmdx);
	return err;
}

enum gsm_error gsm_set_ftp_user_password(char *password)
{
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+FTPPW=\"%s\"\r",password);
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,3000,0,NULL);
	free(cmdx);
	return err;
}

enum gsm_error gsm_set_ftp_download_file_name(char *filename)
{
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+FTPGETNAME=\"%s\"\r",filename);
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,3000,0,NULL);
	free(cmdx);
	return err;
}

enum gsm_error gsm_set_ftp_download_file_path(char *path)
{
	enum gsm_error err;
	const uint8_t MAX_BUFFER = 50;
	char *cmdx = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (cmdx == NULL)
	{
		return 0;
	}
	memset(cmdx, '\0', MAX_BUFFER);
	snprintf((char*)cmdx, MAX_BUFFER,"AT+FTPGETPATH=\"%s\"\r",path);
	err = gsm_send_at_command((const char*)(cmdx), (const char*)RESPONS_OK,3000,0,NULL);
	free(cmdx);
	return err;
}


enum gsm_error gsm_get_the_size_of_specified_file_in_ftp_server(uint32_t *file_size)
{
	enum gsm_error err;
	
	err = gsm_send_at_command((const char*)("AT+FTPSIZE\r"), (const char*)"+FTPSIZE: 1,",76000,0,NULL);
	
	const uint8_t MAX_BUFFER = 50;
	char *aDataBuffer = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (aDataBuffer == NULL)
	{
		return 0;
	}
	memset(aDataBuffer, '\0', MAX_BUFFER);
	
	if (err == GSM_ERROR_NONE)
	{
		if (xSemaphoreTake(gsm_busy_semaphore, 1) == pdFALSE)
		{
			return GSM_ERROR_OPERATION_IN_PROGRESS;
		}
		
		vTaskDelay(100/portTICK_PERIOD_MS);
		
		uint8_t u8tRx_Index=0;
		char u8tTemp_Char=0;
		portBASE_TYPE xStatus;
		
		for (uint8_t i=0;i<50;i++)
		{
			xStatus=xQueueReceive(gsm_rx_queue,&u8tTemp_Char, 0);
			if(xStatus!=errQUEUE_EMPTY)
			{
				if (u8tTemp_Char == '\r')
				{
					break;
				}
				else
				{
					aDataBuffer[u8tRx_Index] = u8tTemp_Char;
					u8tRx_Index++;
				}
			}
			else
			{
				break;
			}
		}
		
		char *ptrfilesize;
		
		ptrfilesize = strtok(aDataBuffer,",");
		
		uint16_t size_err_code = atoi(ptrfilesize);
		
		if (size_err_code == 0)
		{
			ptrfilesize = strtok(NULL,",");
			*file_size = atoi(ptrfilesize);
			err = GSM_ERROR_NONE;
		}
		else
		{
			*file_size = 0;
			
			switch(size_err_code)
			{
				case 61:
				err =  GSM_FTP_NET_ERROR;
				break;
			
				case 62:
				err = GSM_FTP_DNS_ERROR;
				break;
			
				case 63:
				err = GSM_FTP_CONNECT_ERROR;
				break;
			
				case 64:
				err = GSM_FTP_TIMEOUT_ERROR;
				break;
			
				case 65:
				err = GSM_FTP_SERVER_ERROR;
				break;
			
				case 66:
				err = GSM_FTP_OPERATION_NOT_ALLOW;
				break;
			
				case 70:
				err = GSM_FTP_REPLAY_ERROR;
				break;
			
				case 71:
				err = GSM_FTP_USER_ERROR;
				break;
			
				case 72:
				err = GSM_FTP_PASSWORD_ERROR;
				break;
			
				case 73:
				err = GSM_FTP_TYPE_ERROR;
				break;
			
				case 74:
				err = GSM_FTP_REST_ERROR;
				break;
			
				case 75:
				err = GSM_FTP_PASSIVE_ERROR;
				break;
			
				case 76:
				err = GSM_FTP_ACTIVE_ERROR;
				break;
			
				case 77:
				err = GSM_FTP_OPERATE_ERROR;
				break;
			
				case 78:
				err = GSM_FTP_UPLOAD_ERROR;
				break;
			
				case 79:
				err = GSM_FTP_DOWNLOAD_ERROR;
				break;
			
				case 86:
				err = GSM_FTP_MANUAL_QUIT_ERROR;
				break;
			
				default:
				err = GSM_FTP_UNKNOWN_ERROR;
				break;
			}
		}
		
		xSemaphoreGive(gsm_busy_semaphore);
	}
	
	free(aDataBuffer);
	
	return err;
	
}

enum gsm_error gsm_open_ftp_get_session(void)
{
	return gsm_send_at_command((const char*)"AT+FTPGET=1\r", (const char*)"+FTPGET: 1,1",76000,0,NULL);
}

enum gsm_error gsm_read_ftp_download_data(uint16_t size_to_download, char *recv_data,uint16_t *downloaded_data_size)
{	
	enum gsm_error err;
	
	char ftpget[20] = {0};
	snprintf(ftpget,20,"AT+FTPGET=2,%d\r",size_to_download);

	err = gsm_send_at_command((const char*)(ftpget), (const char*)"+FTPGET: ",76000,0,NULL);
	
	const uint8_t MAX_BUFFER = 50;
	char *aDataBuffer = (char*) calloc(MAX_BUFFER,sizeof(char));
	//buffer created???
	if (aDataBuffer == NULL)
	{
		return 0;
	}
	memset(aDataBuffer, '\0', MAX_BUFFER);
	
	if (err == GSM_ERROR_NONE)
	{
		if (xSemaphoreTake(gsm_busy_semaphore, 1) == pdFALSE)
		{
			return GSM_ERROR_OPERATION_IN_PROGRESS;
		}
		
		//vTaskDelay(10/portTICK_PERIOD_MS);
		
		uint8_t u8tRx_Index=0;
		char u8tTemp_Char=0;
		portBASE_TYPE xStatus;
		
		for (uint8_t i=0;i<50;i++)
		{
			xStatus=xQueueReceive(gsm_rx_queue,&u8tTemp_Char, 10);
			if(xStatus!=errQUEUE_EMPTY)
			{
				if (u8tTemp_Char == '\r')
				{
					xQueueReceive(gsm_rx_queue,&u8tTemp_Char, 10); //remove \n fom buffer				
					break;
				}
				else
				{
					aDataBuffer[u8tRx_Index] = u8tTemp_Char;
					u8tRx_Index++;
				}
			}
			else
			{
				break;
			}
		}
		
		char *ptrtkn;
		
		ptrtkn = strtok(aDataBuffer,",");
		
		uint16_t err_code = atoi(ptrtkn);
		
		if (err_code == 1)
		{
			ptrtkn = strtok(NULL,",");
			
			if (atoi(ptrtkn) == 1)
			{
				*downloaded_data_size = 0;
				err = GSM_FTP_DOWNLOAD_DATA_AVAILABLE;
			}
			else
			{
				*downloaded_data_size = 0;
				err = GSM_FTP_DOWNLOAD_FAILED;
			}
		}
		else if (err_code == 2)
		{
			ptrtkn = strtok(NULL,",");
			uint16_t recv_data_size = atoi(ptrtkn);
			if (recv_data_size == 0)
			{
				err = GSM_FTP_DOWNLOAD_NO_DATA_AVAILABLE_IN_BUFFER;
				*downloaded_data_size = 0;
			}
			else if (recv_data_size>0)
			{
				*downloaded_data_size = recv_data_size;			
				xTimerChangePeriod(gsm_cmd_timeout_timer,((5*1000)/portTICK_PERIOD_MS),portMAX_DELAY);	
				u8tRx_Index=0;
				u8tTemp_Char=0;
						
				while (1)
				{
					if(xTimerIsTimerActive(gsm_cmd_timeout_timer))
					{
						if (u8tRx_Index<recv_data_size)
						{
							xStatus=xQueueReceive(gsm_rx_queue,&u8tTemp_Char, 0);
							if(xStatus!=errQUEUE_EMPTY)
							{
								*recv_data=u8tTemp_Char;
								recv_data++;
								u8tRx_Index++;
							}
						}
						else
						{
							err = GSM_FTP_DOWNLOAD_SUCCESS;
							break;
						}
					}
					else
					{
						err = GSM_FTP_DOWNLOAD_FAILED;
						break;
					}
				}
			}	
		}
		else
		{
			*downloaded_data_size = 0;
			err = GSM_FTP_DOWNLOAD_FAILED;
		}
		
		xSemaphoreGive(gsm_busy_semaphore);
	}
	
	free(aDataBuffer);
	return err;

}

enum gsm_error gsm_config_gprs(void)
{
	if (gsm_configure_contype_gprs() == GSM_ERROR_NONE)
	{
		if (gsm_configure_bearer_apn() == GSM_ERROR_NONE)
		{
			return GSM_ERROR_NONE;
		}
		else
		{
			return GSM_ERROR_CONFIG_FAILED;
		}
	}
	else
	{
		return GSM_ERROR_CONFIG_FAILED;
	}
}

enum gsm_error gsm_start_gprs(void)
{
	if (gsm_query_gprs_contex() == GSM_GPRS_BEARER_IS_CLOSED)
	{
		if (gsm_open_gprs_contex() == GSM_ERROR_NONE)
		{
			return GSM_ERROR_NONE;
		}
		else
		{
			return GSM_ERROR_CONFIG_FAILED;
		}
	}
	else
	{
		return GSM_ERROR_CONFIG_FAILED;
	}
}

enum gsm_error gsm_stop_gprs(void)
{
	if (gsm_query_gprs_contex() == GSM_GPRS_BEARER_IS_CLOSED)
	{
		return GSM_ERROR_NONE;
	}
	else
	{
		return gsm_close_gprs_contex();
	}
}

enum gsm_error config_ftp(char *address,char *username,char *password,char *filename,char *path)
{
	if (gsm_set_ftp_server_address(address) == GSM_ERROR_NONE)
	{
		if (gsm_set_ftp_user_name(username) == GSM_ERROR_NONE)
		{
			if (gsm_set_ftp_user_password(password) == GSM_ERROR_NONE)
			{
				if (gsm_set_ftp_download_file_name(filename) == GSM_ERROR_NONE)
				{
					if (gsm_set_ftp_download_file_path(path) == GSM_ERROR_NONE)
					{
						return GSM_ERROR_NONE;
					}
					else
					{
						return GSM_FTP_ERROR;
					}
				}
				else
				{
					return GSM_FTP_ERROR;
				}
			}
			else
			{
				return GSM_FTP_ERROR;
			}
		}
		else
		{
			return GSM_FTP_ERROR;
		}
	}
	else
	{
		return GSM_FTP_ERROR;
	}
}
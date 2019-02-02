#ifndef GSM_DRIVER_H_
#define GSM_DRIVER_H_

#include <asf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include "yalgo.h"
#include "conf_gsm.h"

#define  SMS_UNREAD 0
#define  SMS_READ   1
#define  SMS_ALL    2

//respons ok / error
/**
@brief	Respon OK string
*/
#define RESPONS_OK		"OK"
/**
@brief	Respon ERROR string
*/
#define RESPONS_ERROR	"ERROR"

enum gsm_error
{
	/** No error was encountered while issuing the command sequence. */
	GSM_ERROR_NONE,
	/** Module is not yet ready for the request. */
	GSM_ERROR_NOT_READY,
	/** An ongoing operation was in progress that blocked the request. */
	GSM_ERROR_OPERATION_IN_PROGRESS,
	/** The constructed message length was too large. */
	GSM_ERROR_MESSAGE_LENGTH,
	/** The recipient phone number length was too short. */
	GSM_ERROR_PHONE_NUMBER_LENGTH,
	/** A lock-out is ongoing for new SMS or data transactions. */
	GSM_ERROR_LOCKOUT,
	
	GSM_ERROR_SMS_SEND_FAILED,
	
	GSM_ERROR_TIMEOUT,
	
	GSM_ERROR_UNKWON,
	
	GSM_NETWORK_REGISTERED,
	
	GSM_NETWORK_NOT_REGISTERED,
	
	GSM_ERROR_COMMAND_ERROR,
	
	GSM_ERROR_SMS_NOT_AVAILABLE,
	
	GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_ENABLE,
	
	GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_DISABLE,
	
	GSM_CONNECTED_LINE_IDENTIFICATION_PRESENTATION_UNKNOWN,
	
	GSM_ERROR_CONFIG_FAILED,
	
	///
	GSM_GPRS_BEARER_IS_CONNECTING,
	
	GSM_GPRS_BEARER_IS_CONNECTED,
	
	GSM_GPRS_BEARER_IS_CLOSING,
	
	GSM_GPRS_BEARER_IS_CLOSED,
	///
	
	GSM_FTP_ERROR,
	
	GSM_FTP_DOWNLOAD_SUCCESS,
	
	GSM_FTP_DOWNLOAD_FAILED,
	
	GSM_FTP_DOWNLOAD_NO_DATA_AVAILABLE_IN_BUFFER,    //waitting for buffer completed or download complted
	
	GSM_FTP_DOWNLOAD_DATA_AVAILABLE,
	
	///////
	
	GSM_FTP_NET_ERROR = 61,
	
	GSM_FTP_DNS_ERROR = 62,
	
	GSM_FTP_CONNECT_ERROR = 63,
	
	GSM_FTP_TIMEOUT_ERROR = 64,
	
	GSM_FTP_SERVER_ERROR = 65,
	
	GSM_FTP_OPERATION_NOT_ALLOW = 66,
	
	GSM_FTP_REPLAY_ERROR = 70,
	
	GSM_FTP_USER_ERROR = 71,
	
	GSM_FTP_PASSWORD_ERROR = 72,
	
	GSM_FTP_TYPE_ERROR = 73,
	
	GSM_FTP_REST_ERROR = 74,
	
	GSM_FTP_PASSIVE_ERROR = 75,
	
	GSM_FTP_ACTIVE_ERROR = 76,
	
	GSM_FTP_OPERATE_ERROR = 77,
	
	GSM_FTP_UPLOAD_ERROR = 78,
	
	GSM_FTP_DOWNLOAD_ERROR = 79,
	
	GSM_FTP_MANUAL_QUIT_ERROR = 86,
	
	GSM_FTP_UNKNOWN_ERROR = 90,
};


struct usart_module gsm_usart;

#define GSM_TIMEOUT_PERIOD_TICKS       (5 * 1000 / portTICK_PERIOD_MS)


struct strRTC
{
	volatile uint8_t Network_year;
	volatile uint8_t Network_month;
	volatile uint8_t Network_date;
	volatile uint8_t Network_hour;
	volatile uint8_t Network_minute;
	volatile uint8_t Network_second;
	
}struct_internal_rtc;

void Flush_RX_Buffer(void);


void gsm_init(void);
enum gsm_error gsm_send_at_command(const char *const command,const char* aResponExit,const uint32_t aTimeoutMax,const uint8_t aLenOut, char *aResponOut);
enum gsm_error gsm_check_module(void);
enum gsm_error gsm_disable_data_flow_control(void);
enum gsm_error gsm_is_network_registered(void);
enum gsm_error gsm_set_baudrate(void);
enum gsm_error gsm_set_auto_baudrate(void);
enum gsm_error gsm_set_network_registration(void);
enum gsm_error gsm_set_phone_full_functionality(void);
enum gsm_error gsm_check_phone_full_functionality(void);
enum gsm_error gsm_set_phone_minimum_functionality(void);
enum gsm_error gsm_check_phone_minimum_functionality(void);
enum gsm_error gsm_software_reset(void);
enum gsm_error gsm_detect_simcard(void);
enum gsm_error gsm_delete_all_sms(void);
enum gsm_error gsm_store_active_profile(void);
enum gsm_error gsm_enable_calling_line_identification(void);
enum gsm_error gsm_enable_connected_line_identification_presentation(void);
enum gsm_error gsm_check_connected_line_identification_presentation(void);
enum gsm_error gsm_enable_list_current_calls_of_ME(void);
enum gsm_error gsm_factory_reset(void);
enum gsm_error gsm_echo_off(void);
enum gsm_error gsm_select_sms_message_formate_text_mode(void);
enum gsm_error gsm_set_sms_text_mode_parameter(void);
enum gsm_error gsm_save_sms_settings_in_profile_0(void);
enum gsm_error gsm_save_sms_settings_in_profile_1(void);

enum gsm_error gsm_enable_sleep_mode(void);
uint8_t gsm_getsignalstrength(void);

void RemoveSpaces(char* source);

enum gsm_error factory_defined_configuration(void);

enum gsm_error gsm_enable_new_sms_message_indications(void);
enum gsm_error gsm_disable_new_sms_message_indications(void);

enum gsm_error gsm_send_sms(const char *phone_number, const char *message);
uint8_t gsm_get_sms_index(uint8_t required_sms_status);
enum gsm_error gsm_read_sms(uint8_t position, char *phone_number, uint8_t max_phone_len, char *SMS_text, uint8_t max_SMS_len);

enum gsm_error gsm_config_module(void);

enum gsm_error gsm_disable_call_waiting(void);
enum gsm_error gsm_reject_all_incomming_calls(void);
enum gsm_error gsm_enable_all_incomming_calls(void);

enum gsm_error gsm_play_record_file(const char *filename,bool playInfinitely);
enum gsm_error gsm_stop_play_record_file(void);

char gsm_responseLine_isNew_SMS_Received(char *response);
bool gsm_responseLine_isRinging(char *response);
bool gsm_responseLine_get_IncommingCallNo(char *response,char *phone_number);
bool gsm_responseLine_isCallCut(char *response);
bool gsm_responseLine_isNew_DTMF_Command_Received(char *response);
bool gsm_responseLine_is_StopSound_Received(char *response);

enum gsm_error gsm_call_to_dial_a_number(const char *to);
enum gsm_error gsm_answer_an_incomming_call(void);
enum gsm_error gsm_hangup_call(void);

enum gsm_error gsm_enable_network_time_update(void);
enum gsm_error gsm_disable_network_time_update(void);

enum gsm_error gsm_enable_DTMF_detection(void);
enum gsm_error gsm_send_DTMF_Tone(char *tone);

enum gsm_error gsm_get_internal_rtc_time(void);

bool gsm_read_response_line(char *buffer,uint8_t length);



//////////////////////////////////////////////////////////////////////////
enum gsm_error gsm_configure_contype_gprs(void);
enum gsm_error gsm_configure_bearer_apn(void);
enum gsm_error gsm_query_gprs_contex(void);
enum gsm_error gsm_open_gprs_contex(void);
enum gsm_error gsm_close_gprs_contex(void);
enum gsm_error gsm_set_ftp_bearer_profile(void);
enum gsm_error gsm_set_ftp_server_address(char *address);
enum gsm_error gsm_set_ftp_user_name(char *username);
enum gsm_error gsm_set_ftp_user_password(char *password);
enum gsm_error gsm_set_ftp_download_file_name(char *filename);
enum gsm_error gsm_set_ftp_download_file_path(char *path);
enum gsm_error gsm_get_the_size_of_specified_file_in_ftp_server(uint32_t *file_size);
enum gsm_error gsm_open_ftp_get_session(void);
enum gsm_error gsm_read_ftp_download_data(uint16_t size_to_download, char *recv_data,uint16_t *downloaded_data_size);


enum gsm_error gsm_config_gprs(void);
enum gsm_error gsm_start_gprs(void);
enum gsm_error gsm_stop_gprs(void);

enum gsm_error config_ftp(char *address,char *username,char *password,char *filename,char *path);


#endif /* GSM_DRIVER_H_ */
#ifndef GSM_SERVICE_H_
#define GSM_SERVICE_H_

#include "gsm_driver.h"
#include "eeprom_driver.h"
#include "motor_service.h"
#include "lcd_driver.h"

bool simReInit;
bool isRegisteredNumber;

uint8_t soundWaitTime;
uint32_t soundWait;
bool bplaySound;
char playFile;
bool inCall;

bool zeroPressed;

uint16_t callCutWaitTime;  //x100 = mSec
uint32_t callCutWait;

bool isMsgFromAdmin;

bool commandsAccepted;
uint16_t  acceptCommandsTime;
uint32_t tempAcceptCommandTime;

volatile char currentStatus;
volatile char currentCallStatus;

volatile uint8_t Signal_Strength;

uint8_t nr;
bool callAccepted;

uint8_t retries;

bool m2mAck;
uint8_t m2mEventCalls;
bool m2mEventStaged;
bool m2mEvent;
uint8_t m2mEventNo;
bool keyPressed;

bool eventStaged;
uint32_t tempEventStageTime;
char stagedEventType;

char actionType;

bool freezeIncomingCalls;

bool obtainNewEvent;
uint32_t obtainEventTimer;

uint8_t currentPlayingFileIndex;
uint8_t maxPlayingFiles;
char playFilesList[8];

#define STR_MOTOR "MOTOR "
#define STR_ON "ON"
#define STR_OFF "OFF"

#define SEND_TO_M2M_MASTER 0x02
#define SEND_TO_M2M_REMOTE 0x01

volatile bool initialized;

void setObtainEvent(void);
void operateOnStagedEvent(void);
void makeResponseAction(void);
void endCall(void);
void makeCall(void);
void acceptCall(void);
void playSound(char actionType, bool newAction);
bool playSoundElligible(void);
void triggerPlaySound(void);
void playSoundAgain(char *string);
void playRepeatedFiles(char *fileList);
bool callTimerExpire(void);
char OutGoingcallState(char *response);
bool registerEvent(char eventType);
void registerM2MEvent(uint8_t eventNo);
void setMotorMGRResponse(char response);
void checkRespSMS(char t1);
void subDTMF(void);
void processOnDTMF(char *dtmf_cmd);
void processOnSMS(char *received_command, bool admin,bool response_sms_processed_cmd,bool alterNumber, char *phone_number);
void buildStatusMessage(char *resep_msg);

bool checkNumber(char *number);
bool checkNoCallTime(void);
void verifyRemoteNumber(void);
void sendSMS(char *msg, bool predefMsg, uint8_t isM2M);
void sendDTMFTone(uint8_t eventNo);
void getSystemTime(uint8_t *Hours, uint8_t *Minutes);


void start_gsm_service(void);

bool busy(void);
bool checkNotInCall(void);
bool gsm_checkSleepElligible(void);


#endif /* GSM_SERVICE_H_ */
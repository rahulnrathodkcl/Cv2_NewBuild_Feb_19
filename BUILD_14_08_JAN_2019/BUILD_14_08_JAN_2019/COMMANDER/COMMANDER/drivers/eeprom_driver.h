#ifndef EEPROM_DRIVER_H_
#define EEPROM_DRIVER_H_

#include <asf.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////
// PAGE LOCATIONS::::::::::::
//////////////////////////////////////////////////////////////////////////

uint8_t page_data[EEPROM_PAGE_SIZE];

#define ADMIN_1_MOBILE_NUMBER_PAGE		0
#define ADMIN_2_MOBILE_NUMBER_PAGE		1
#define ADMIN_3_MOBILE_NUMBER_PAGE		2
#define ADMIN_4_MOBILE_NUMBER_PAGE		3
#define ADMIN_5_MOBILE_NUMBER_PAGE		4

#define USER_COUNTER_PAGE				5

#define USER_1_MOBILE_NUMBER_PAGE		6
#define USER_2_MOBILE_NUMBER_PAGE		7
#define USER_3_MOBILE_NUMBER_PAGE		8
#define USER_4_MOBILE_NUMBER_PAGE		9	
#define USER_5_MOBILE_NUMBER_PAGE		10
#define USER_6_MOBILE_NUMBER_PAGE		11
#define USER_7_MOBILE_NUMBER_PAGE		12
#define USER_8_MOBILE_NUMBER_PAGE		13
#define USER_9_MOBILE_NUMBER_PAGE		14
#define USER_10_MOBILE_NUMBER_PAGE		15
#define USER_11_MOBILE_NUMBER_PAGE		16
#define USER_12_MOBILE_NUMBER_PAGE		17
#define USER_13_MOBILE_NUMBER_PAGE		18
#define USER_14_MOBILE_NUMBER_PAGE		19
#define USER_15_MOBILE_NUMBER_PAGE		20

#define ALTARNATE_NUMBERS_PAGE			21

#define M2M_NUMBERS_PAGE				22

#define USER_SETTING_PARAMETERS_PAGE	23

#define FACTORY_SETTING_PARAMETERS_PAGE	24

//////////////////////////////////////////////////////////////////////////
///////////////////////////DND SETTINGS///////////////////////////////////
#define DND_OFF							'O'
#define DND_SINGLEPHASING				'S'
#define DND_LIGHT						'L'

//////////////////////////////////////////////////////////////////////////

struct user_count
{
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee; //1
	//////////////////////////////////////////////////////////////////////////
	
	uint8_t total_user_no_count;	//1
	uint8_t current_user_no_count;	//1
	uint8_t dummy1;					//1

////////////////////////////////////////////////////
uint8_t primaryNumberIndex;				//1
uint8_t secondaryNumberIndex;			//1

////////////////////////////////////////////////////

	
}user_count_struct;

struct mobile_no_struct
{
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee; //1
	//////////////////////////////////////////////////////////////////////////
	uint8_t dummy1;					//1
	uint8_t dummy2;					//1
	uint8_t dummy3;					//1
	char mobile_no_ee[20];			//20
};

struct alternateNumber
{
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee; //1
	//////////////////////////////////////////////////////////////////////////
	uint8_t alterNumberSetting;		//1 
	uint8_t alterNumberPresent;		//1 
	uint8_t dummy;					//1				
	char alternateNumber_ee[20];	//20
	
}alternateNumber_struct;

struct m2m_Numbers
{
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee;	//1
	//////////////////////////////////////////////////////////////////////////
	uint8_t m2mPresent;				//1
	uint8_t m2mVerified;			//1
	
	uint8_t m2mRemotePresent;		//1
	uint8_t m2mRemoteVerified;		//1
	
	uint8_t m2mSetting;				//1
	
	uint8_t dummy1;					//1
	uint8_t dummy2;					//1
	
	char m2mNumber_ee[20];			//20
	char m2mremoteNumber_ee[20];	//20
	
}m2m_Numbers_struct;

struct user_settings_parameter
{
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee;			//1
	//////////////////////////////////////////////////////////////////////////
	uint8_t autoStartAddress;				//1
	uint16_t autoStartTimeAddress;			//2
	
	char dndAddress;						//1
	uint8_t responseAddress;				//1
	uint16_t starDeltaTimerAddress;			//2
	
	uint8_t eventStageAddress;				//1
	uint8_t noCallAddress;					//1
	uint8_t noCallStartTimeHourAddress;		//1
	
	uint8_t noCallStartTimeMinuteAddress;	//1
	uint8_t noCallStopTimeHourAddress;		//1
	uint8_t noCallStopTimeMinuteAddress;	//1
	uint8_t lowVoltAddress;					//1
	
	uint8_t currentDetectionAddress;		//1
	uint32_t normalLoadAddress;				//4
	uint32_t overloadAddress;				//4
	uint32_t underloadAddress;				//4
	
	uint8_t underloadPerAddress;			//1
	uint8_t overloadPerAddress;				//1
	uint8_t jumperSettingAddress;			//1
	uint8_t dummy1;							//1
	
	uint8_t preventOverFlowAddress;			//1
	uint8_t waterBypassAddress;				//1
	uint8_t dummy2;							//1
	uint8_t dummy3;							//1
	
	
////////////////////////////////////////////////////
	uint8_t detectSinglePhasing;			//1
	uint16_t singlePhasingVoltage;			//2
////////////////////////////////////////////////////
	uint8_t detectPhaseSequence;			//1


	//uint8_t bypassAddress;				//1
}user_settings_parameter_struct;


struct factory_settings_parameter
{
	
	//////////////////////////////////////////////////////////////////////////
	uint8_t u8tfirst_time_write_ee;			//1
	//////////////////////////////////////////////////////////////////////////
	uint8_t ENABLE_CURRENT;					//1
	uint8_t AMPERE_SPEAK;					//1
	uint8_t ENABLE_M2M;						//1
	
	uint8_t ENABLE_GP;						//1
	uint8_t ENABLE_WATER;					//1
	uint8_t DUAL_LEVEL;						//1
	uint8_t NOLEVELCHANGECALL;				//1
	
	char DeviceID_ee[20];					//20
	
}factory_settings_parameter_struct;


void configure_eeprom(void);
void config_mobile_no_ee(const uint8_t page_loc,const char *mobile_number);
void init_eeprom(void);

void getNumbers(char *string);
char *getIndexedNumber(char *IndexNo,uint8_t index);

bool isPrimaryNumber(char *number);
bool isAlterNumber(char *number);

bool isM2MNumber(char *number);
bool isM2MRemoteNumber(char *number);

char *getM2MNumber(char *m2mNo);
char *getM2MRemoteNumber(char *m2mNoRemotNo);

void setM2MVerify(bool flag);
void setM2MRemoteVerified(bool flag);

void saveM2MSettings(bool flag);

void addM2MNumber(char *no);
void addM2MRemoteNumber(char *no);

char *getActiveNumber(char *ActiveNo);

uint8_t checkExists(char *number);

bool addNumber(char *number);
bool removeNumber(char *numer);
void clearNumbers(bool admin);
void saveAlterNumberSetting(bool flag);
bool addAlternateNumber(char *numer);

void saveAutoStartSettings(bool flag);
void saveAutoStartTimeSettings(uint16_t value);
void saveDNDSettings(char dndFlag);
//void saveBypassSettings(bool flag);
void saveSinglePhasingSettings(bool singlePhasing);
void saveSinglePhasingVoltage(uint16_t voltage);
void savePhaseSequenceProtectionSettings(bool phaseSequence);

void saveResponseSettings(char response);
void saveNoCallSettings(bool value,uint8_t startHour,uint8_t startMinute,uint8_t stopHour,uint8_t stopMinute);
void saveWaterBypassSettings(bool flag);
void savePreventOverFlowSettings(bool flag);
void setJumperSettings(uint8_t jumperVal);
bool setOverloadPer(uint8_t overloadPerValue);
bool setUnderloadPer(uint8_t underloadPerValue);
void calcCurrentValues(void);
void setUnderloadValue(uint32_t underValue);
void setOverloadValue(uint32_t overValue);
void setNormalLoadValue(uint32_t normalVal);
void setCurrentDetection(bool cValue);
char *getDeviceId(char *deviceID);
void saveStarDeltaTimer(uint16_t StartDeltaTime);
void saveEventStageSettings(uint8_t data);

bool isAdmin(char *number);

////////////////////////////////

void setPrimaryNumberIndex(uint8_t index);
void setSecondaryNumberIndex(uint8_t index);

bool addPrimaryIndexedNumber(char *number);
bool addSecondaryIndexedNumber(char *number);

///////////////////////////////

#endif /* EEPROM_DRIVER_H_ */
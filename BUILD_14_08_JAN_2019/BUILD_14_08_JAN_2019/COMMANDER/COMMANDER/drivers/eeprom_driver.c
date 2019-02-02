#include "eeprom_driver.h"

void configure_eeprom(void)
{
	enum status_code error_code = eeprom_emulator_init();
	if (error_code == STATUS_ERR_NO_MEMORY)
	{
		while (true);
	}
	else if (error_code != STATUS_OK)
	{
		eeprom_emulator_erase_memory();
		eeprom_emulator_init();
	}
}

void init_eeprom(void)
{
	configure_eeprom();
	
	eeprom_emulator_read_page(USER_COUNTER_PAGE, page_data);
	memcpy(&user_count_struct,page_data,sizeof(user_count_struct));
	if (user_count_struct.u8tfirst_time_write_ee != 85)
	{
		user_count_struct.u8tfirst_time_write_ee = 85;
		user_count_struct.total_user_no_count    = 15;
		user_count_struct.current_user_no_count  = 0;
		user_count_struct.primaryNumberIndex = 0;
		user_count_struct.secondaryNumberIndex = 1;
		
		
		memcpy(page_data,&user_count_struct,sizeof(user_count_struct));
		eeprom_emulator_write_page(USER_COUNTER_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	//////////////////////////////////////////////////////////////////////////
	config_mobile_no_ee(ADMIN_1_MOBILE_NUMBER_PAGE,"7041196959");
	config_mobile_no_ee(ADMIN_2_MOBILE_NUMBER_PAGE,"7698439201");
	config_mobile_no_ee(ADMIN_3_MOBILE_NUMBER_PAGE,"7383614214");
	config_mobile_no_ee(ADMIN_4_MOBILE_NUMBER_PAGE,"7383622678");
	//config_mobile_no_ee(ADMIN_5_MOBILE_NUMBER_PAGE,"9586135978");
	config_mobile_no_ee(ADMIN_5_MOBILE_NUMBER_PAGE,"8140200752");
	//////////////////////////////////////////////////////////////////////////
	config_mobile_no_ee(USER_1_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_2_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_3_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_4_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_5_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_6_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_7_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_8_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_9_MOBILE_NUMBER_PAGE ,"0000000000");
	config_mobile_no_ee(USER_10_MOBILE_NUMBER_PAGE,"0000000000");
	config_mobile_no_ee(USER_11_MOBILE_NUMBER_PAGE,"0000000000");
	config_mobile_no_ee(USER_12_MOBILE_NUMBER_PAGE,"0000000000");
	config_mobile_no_ee(USER_13_MOBILE_NUMBER_PAGE,"0000000000");
	config_mobile_no_ee(USER_14_MOBILE_NUMBER_PAGE,"0000000000");
	config_mobile_no_ee(USER_15_MOBILE_NUMBER_PAGE,"0000000000");
	
	//////////////////////////////////////////////////////////////////////////
	eeprom_emulator_read_page(ALTARNATE_NUMBERS_PAGE, page_data);
	memcpy(&alternateNumber_struct,page_data,sizeof(alternateNumber_struct));
	if (alternateNumber_struct.u8tfirst_time_write_ee != 85)
	{
		alternateNumber_struct.u8tfirst_time_write_ee = 85;
		
		alternateNumber_struct.alterNumberPresent = false;
		alternateNumber_struct.alterNumberSetting = false;
		
		memset(alternateNumber_struct.alternateNumber_ee, '\0', sizeof(alternateNumber_struct.alternateNumber_ee));
		strcpy(alternateNumber_struct.alternateNumber_ee,"0000000000");
		
		memcpy(page_data,&alternateNumber_struct,sizeof(alternateNumber_struct));
		eeprom_emulator_write_page(ALTARNATE_NUMBERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	//////////////////////////////////////////////////////////////////////////
	eeprom_emulator_read_page(M2M_NUMBERS_PAGE, page_data);
	memcpy(&m2m_Numbers_struct,page_data,sizeof(m2m_Numbers_struct));
	if (m2m_Numbers_struct.u8tfirst_time_write_ee != 85)
	{
		m2m_Numbers_struct.u8tfirst_time_write_ee = 85;
		
		memset(m2m_Numbers_struct.m2mNumber_ee, '\0', sizeof(m2m_Numbers_struct.m2mNumber_ee));
		strcpy(m2m_Numbers_struct.m2mNumber_ee,"0000000000");
		
		memset(m2m_Numbers_struct.m2mremoteNumber_ee, '\0', sizeof(m2m_Numbers_struct.m2mremoteNumber_ee));
		strcpy(m2m_Numbers_struct.m2mremoteNumber_ee,"0000000000");
		
		m2m_Numbers_struct.m2mPresent			= false;
		m2m_Numbers_struct.m2mVerified			= false;
		
		m2m_Numbers_struct.m2mRemotePresent		= false;
		m2m_Numbers_struct.m2mRemoteVerified	= false;
		
		m2m_Numbers_struct.m2mSetting			= false;
		
		memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
		eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	//////////////////////////////////////////////////////////////////////////
	eeprom_emulator_read_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	memcpy(&user_settings_parameter_struct,page_data,sizeof(user_settings_parameter_struct));
	if (user_settings_parameter_struct.u8tfirst_time_write_ee != 85)
	{
		user_settings_parameter_struct.u8tfirst_time_write_ee		= 85;
		
		user_settings_parameter_struct.autoStartAddress				= false;
		user_settings_parameter_struct.autoStartTimeAddress			= 50;
		user_settings_parameter_struct.dndAddress					= DND_OFF;
		user_settings_parameter_struct.responseAddress				= 'T';
		user_settings_parameter_struct.starDeltaTimerAddress		= 2;
		//user_settings_parameter_struct.bypassAddress				= false;
		user_settings_parameter_struct.eventStageAddress			= 0;
		user_settings_parameter_struct.noCallAddress				= false;
		user_settings_parameter_struct.noCallStartTimeHourAddress	= 0;
		user_settings_parameter_struct.noCallStartTimeMinuteAddress = 0;
		user_settings_parameter_struct.noCallStopTimeHourAddress	= 0;
		user_settings_parameter_struct.noCallStopTimeMinuteAddress	= 0;
		user_settings_parameter_struct.lowVoltAddress				= 0;
		user_settings_parameter_struct.currentDetectionAddress		= false;
		user_settings_parameter_struct.normalLoadAddress			= 0;
		user_settings_parameter_struct.overloadAddress				= 0;
		user_settings_parameter_struct.underloadAddress				= 0;
		user_settings_parameter_struct.underloadPerAddress			= 85;
		user_settings_parameter_struct.overloadPerAddress			= 120;
		user_settings_parameter_struct.jumperSettingAddress			= 1;
		user_settings_parameter_struct.preventOverFlowAddress		= false;
		user_settings_parameter_struct.waterBypassAddress			= false;
		
		user_settings_parameter_struct.detectSinglePhasing			= true;
		user_settings_parameter_struct.singlePhasingVoltage			= 80;
		user_settings_parameter_struct.detectPhaseSequence			= true;

		memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
		eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
	//////////////////////////////////////////////////////////////////////////
	eeprom_emulator_read_page(FACTORY_SETTING_PARAMETERS_PAGE, page_data);
	memcpy(&factory_settings_parameter_struct,page_data,sizeof(factory_settings_parameter_struct));
	if (factory_settings_parameter_struct.u8tfirst_time_write_ee != 85)
	{
		factory_settings_parameter_struct.u8tfirst_time_write_ee	= 85;
		factory_settings_parameter_struct.AMPERE_SPEAK				= true;
		factory_settings_parameter_struct.ENABLE_CURRENT			= true;
		factory_settings_parameter_struct.DUAL_LEVEL			    = true;
		factory_settings_parameter_struct.ENABLE_WATER				= true;
		factory_settings_parameter_struct.ENABLE_GP					= true;
		factory_settings_parameter_struct.ENABLE_M2M				= false;
		factory_settings_parameter_struct.NOLEVELCHANGECALL			= false;
		
		memset(factory_settings_parameter_struct.DeviceID_ee, '\0', sizeof(factory_settings_parameter_struct.DeviceID_ee));
		strcpy(factory_settings_parameter_struct.DeviceID_ee,"0000000000");
		
		memcpy(page_data,&factory_settings_parameter_struct,sizeof(factory_settings_parameter_struct));
		eeprom_emulator_write_page(FACTORY_SETTING_PARAMETERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
	}
}

void config_mobile_no_ee(const uint8_t page_loc,const char *mobile_number)
{
	struct mobile_no_struct mobile_no;
	memset(page_data, '\0', sizeof(page_data));
	eeprom_emulator_read_page(page_loc, page_data);
	memcpy(&mobile_no,page_data,sizeof(mobile_no));
	if (mobile_no.u8tfirst_time_write_ee != 85)
	{
		mobile_no.u8tfirst_time_write_ee = 85;
		mobile_no.dummy1 = 0;
		mobile_no.dummy2 = 0;
		mobile_no.dummy3 = 0;
		memset(mobile_no.mobile_no_ee, '\0', sizeof(mobile_no.mobile_no_ee));
		strcpy(mobile_no.mobile_no_ee,mobile_number);
		
		memcpy(page_data,&mobile_no,sizeof(mobile_no));
		eeprom_emulator_write_page(page_loc, page_data);
		eeprom_emulator_commit_page_buffer();
	}
}


void getNumbers(char *string)
{
	strcpy(string,"");
	
	for(uint8_t i=0;i<user_count_struct.current_user_no_count;i++)
	{
		struct mobile_no_struct mobile_no;
		memset(page_data, '\0', sizeof(page_data));
		eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+i, page_data);
		memcpy(&mobile_no,page_data,sizeof(mobile_no));
		if(i==user_count_struct.primaryNumberIndex)
		{
			strcat(string,"P:");
		}
		else if(i==user_count_struct.secondaryNumberIndex)
		{
			strcat(string,"S:");
		}
		strcat(string,mobile_no.mobile_no_ee);
		strcat(string,"\n");
	}
}

char *getIndexedNumber(char *IndexNo, uint8_t index)
{
	strcpy(IndexNo,"");
	if(user_count_struct.current_user_no_count>index)
	{
		struct mobile_no_struct mobile_no;
		memset(page_data, '\0', sizeof(page_data));
		eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+index, page_data);
		//eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+index-1, page_data);
		memcpy(&mobile_no,page_data,sizeof(mobile_no));
		strcat(IndexNo,mobile_no.mobile_no_ee);
	}
	return IndexNo;
}

bool isPrimaryNumber(char *number)
{
	if(user_count_struct.current_user_no_count > 0)
	{
		char primaryNumber[20] = {0};
		getIndexedNumber(&primaryNumber,user_count_struct.primaryNumberIndex);
		if(strstr(number,primaryNumber))
		{
			return true;
		}
		else
		{
			return isAlterNumber(number);
		}
		//struct mobile_no_struct mobile_no;
		//memset(page_data, '\0', sizeof(page_data));
		//eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE, page_data);
		//if (strstr(number,mobile_no.mobile_no_ee))
		//{
		//return true;
		//}
		//else
		//{
		//return isAlterNumber(number);
		//}
	}
	return false;
}

bool isAlterNumber(char *number)
{
	if(user_count_struct.current_user_no_count > 0)
	{
		if ((alternateNumber_struct.alterNumberPresent) && (strstr(number, alternateNumber_struct.alternateNumber_ee)))
		{
			return true;
		}
	}
	return false;
}

bool isM2MNumber(char *number)
{
	if ((m2m_Numbers_struct.m2mPresent) && strstr(number,m2m_Numbers_struct.m2mNumber_ee))
	{
		return true;
	}
	return false;
}

bool isM2MRemoteNumber(char *number)
{
	if ((m2m_Numbers_struct.m2mRemotePresent) && strstr(number,m2m_Numbers_struct.m2mremoteNumber_ee))
	{
		return true;
	}
	return false;
}

char *getM2MNumber(char *m2mNo)
{
	if (m2m_Numbers_struct.m2mPresent)
	{
		//strstr(m2mNo,m2m_Numbers_struct.m2mNumber_ee);
		strcpy(m2mNo,m2m_Numbers_struct.m2mNumber_ee);
	}
	else
	{
		strcpy(m2mNo,"");
	}
	return m2mNo;
}

char *getM2MRemoteNumber(char *m2mNoRemotNo)
{
	if (m2m_Numbers_struct.m2mRemotePresent)
	{
		strcpy(m2mNoRemotNo,m2m_Numbers_struct.m2mremoteNumber_ee);
	}
	else
	{
		strcpy(m2mNoRemotNo,"");
	}
	return m2mNoRemotNo;
}

void setM2MVerify(bool flag)
{
	m2m_Numbers_struct.m2mVerified = (uint8_t)flag;
	memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
	eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setM2MRemoteVerified(bool flag)
{
	m2m_Numbers_struct.m2mRemoteVerified = (uint8_t)flag;
	memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
	eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveM2MSettings(bool flag)
{
	m2m_Numbers_struct.m2mSetting = (uint8_t)flag;
	memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
	eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void addM2MNumber(char *no)
{
	m2m_Numbers_struct.m2mPresent = true;
	strcpy(m2m_Numbers_struct.m2mNumber_ee,no);
	memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
	eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
	setM2MVerify(false);
}

void addM2MRemoteNumber(char *no)
{
	m2m_Numbers_struct.m2mRemotePresent = true;
	strcpy(m2m_Numbers_struct.m2mremoteNumber_ee,no);
	memcpy(page_data,&m2m_Numbers_struct,sizeof(m2m_Numbers_struct));
	eeprom_emulator_write_page(M2M_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
	setM2MRemoteVerified(false);
	saveM2MSettings(false);
}

char *getActiveNumber(char *ActiveNo)
{
	if (user_count_struct.current_user_no_count > 0)
	{
		if ((alternateNumber_struct.alterNumberSetting))
		{
			strcpy(ActiveNo,alternateNumber_struct.alternateNumber_ee);
		}
		else
		{
			struct mobile_no_struct mobile_no;
			memset(page_data, '\0', sizeof(page_data));
			eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+user_count_struct.primaryNumberIndex, page_data);
			memcpy(&mobile_no,page_data,sizeof(mobile_no));
			strcpy(ActiveNo,mobile_no.mobile_no_ee);
		}
	}
	else																					// no effect of this portion as registerEvent checks for numbercount is > 0
	{
		struct mobile_no_struct mobile_no;
		memset(page_data, '\0', sizeof(page_data));
		eeprom_emulator_read_page(ADMIN_1_MOBILE_NUMBER_PAGE, page_data);
		memcpy(&mobile_no,page_data,sizeof(mobile_no));
		strcpy(ActiveNo,mobile_no.mobile_no_ee);
	}
	
	return ActiveNo;
}
uint8_t checkExists(char *number)
{
	if (user_count_struct.current_user_no_count > 0)
	{
		//if(isPrimaryNumber(number))
		//{
		//return 0;
		//}
		
		for (uint8_t i=0;i<user_count_struct.current_user_no_count;i++)
		{
			struct mobile_no_struct mobile_no;
			memset(page_data, '\0', sizeof(page_data));
			eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+i, page_data);
			memcpy(&mobile_no,page_data,sizeof(mobile_no));
			if (strstr(number,mobile_no.mobile_no_ee))
			{
				return i;
			}
		}
	}

	if(isAlterNumber(number))
	{
		return 0xFD;
	}

	if((m2m_Numbers_struct.m2mVerified) && isM2MNumber(number))
	{
		return 0xFE;
	}
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		if ((m2m_Numbers_struct.m2mRemoteVerified) && isM2MRemoteNumber(number))
		{
			return 0xFE;
		}
	}
	return 0xFF;
}

bool addNumber(char *number)
{
	if (user_count_struct.current_user_no_count == user_count_struct.total_user_no_count)
	{
		return false;
	}
	else
	{
		if (checkExists(number) > 0xF0)
		{
			struct mobile_no_struct mobile_no;
			memset(page_data, '\0', sizeof(page_data));
			eeprom_emulator_read_page((USER_1_MOBILE_NUMBER_PAGE+user_count_struct.current_user_no_count), page_data);
			memcpy(&mobile_no,page_data,sizeof(mobile_no));
			
			memset(mobile_no.mobile_no_ee, '\0', sizeof(mobile_no.mobile_no_ee));
			strcpy(mobile_no.mobile_no_ee,number);
			
			memcpy(page_data,&mobile_no,sizeof(mobile_no));
			eeprom_emulator_write_page((USER_1_MOBILE_NUMBER_PAGE+user_count_struct.current_user_no_count), page_data);
			eeprom_emulator_commit_page_buffer();
			
			user_count_struct.current_user_no_count++;
			
			memcpy(page_data,&user_count_struct,sizeof(user_count_struct));
			eeprom_emulator_write_page(USER_COUNTER_PAGE, page_data);
			eeprom_emulator_commit_page_buffer();
			
			return true;
		}
	}
	return false;
}

bool removeNumber(char *numer)
{
	if (user_count_struct.current_user_no_count < 2)
	{
		return false;
	}
	else
	{
		uint8_t loc =  checkExists(numer);
		if ((loc < user_count_struct.total_user_no_count) && (loc != user_count_struct.primaryNumberIndex))	//number is not special number(i.e. alter,m2m,m2mRemote) and not primary number
		{
			for (uint8_t i=loc;i<(user_count_struct.current_user_no_count-1);i++)
			{
				memset(page_data, '\0', sizeof(page_data));
				eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+i+1, page_data);
				eeprom_emulator_write_page(USER_1_MOBILE_NUMBER_PAGE+i, page_data);
				eeprom_emulator_commit_page_buffer();
			}
			user_count_struct.current_user_no_count--;
			
			if(loc==user_count_struct.secondaryNumberIndex || loc>user_count_struct.current_user_no_count-1)
			{
				user_count_struct.secondaryNumberIndex= 1;
			}
			memcpy(page_data,&user_count_struct,sizeof(user_count_struct));
			eeprom_emulator_write_page(USER_COUNTER_PAGE, page_data);
			eeprom_emulator_commit_page_buffer();
			return true;
		}
	}
	return false;
}

void clearNumbers(bool admin)
{
	if (admin)
	{
		user_count_struct.current_user_no_count = 0;
	}
	else
	{
		memset(page_data, '\0', sizeof(page_data));
		eeprom_emulator_read_page(USER_1_MOBILE_NUMBER_PAGE+user_count_struct.primaryNumberIndex, page_data);
		eeprom_emulator_write_page(USER_1_MOBILE_NUMBER_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();

		user_count_struct.current_user_no_count = 1;
		
	}
	user_count_struct.primaryNumberIndex=0;
	user_count_struct.secondaryNumberIndex=1;

	memcpy(page_data,&user_count_struct,sizeof(user_count_struct));
	user_count_struct.primaryNumberIndex=0;
	user_count_struct.secondaryNumberIndex=1;
	eeprom_emulator_write_page(USER_COUNTER_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
	
	saveAlterNumberSetting(false);
	
	alternateNumber_struct.alterNumberPresent = false;
	memcpy(page_data,&alternateNumber_struct,sizeof(alternateNumber_struct));
	eeprom_emulator_write_page(ALTARNATE_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveAlterNumberSetting(bool flag)
{
	alternateNumber_struct.alterNumberSetting = flag;
	memcpy(page_data,&alternateNumber_struct,sizeof(alternateNumber_struct));
	eeprom_emulator_write_page(ALTARNATE_NUMBERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

bool addAlternateNumber(char *numer)
{
	if (user_count_struct.current_user_no_count > 0)
	{
		alternateNumber_struct.alterNumberPresent = true;
		strcpy(alternateNumber_struct.alternateNumber_ee,numer);
		memcpy(page_data,&alternateNumber_struct,sizeof(alternateNumber_struct));
		eeprom_emulator_write_page(ALTARNATE_NUMBERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
		
		return true;
	}
	return false;
}

void saveAutoStartSettings(bool flag)
{
	user_settings_parameter_struct.autoStartAddress = (uint8_t)flag;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveAutoStartTimeSettings(uint16_t value)
{
	user_settings_parameter_struct.autoStartTimeAddress = value;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveDNDSettings(char flag)
{
	user_settings_parameter_struct.dndAddress = flag;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

//void saveBypassSettings(bool flag)
//{
//user_settings_parameter_struct.bypassAddress = (uint8_t)flag;
//memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
//eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
//eeprom_emulator_commit_page_buffer();
//}

void saveResponseSettings(char response)
{
	user_settings_parameter_struct.responseAddress = response;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveNoCallSettings(bool flag,uint8_t startHour,uint8_t startMinute,uint8_t stopHour,uint8_t stopMinute)
{
	user_settings_parameter_struct.noCallAddress = (uint8_t)flag;
	if (user_settings_parameter_struct.noCallAddress)
	{
		user_settings_parameter_struct.noCallStartTimeHourAddress = startHour;
		user_settings_parameter_struct.noCallStartTimeMinuteAddress = startMinute;
		
		user_settings_parameter_struct.noCallStopTimeHourAddress = stopHour;
		user_settings_parameter_struct.noCallStopTimeMinuteAddress = stopMinute;
	}
	
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveWaterBypassSettings(bool flag)
{
	user_settings_parameter_struct.waterBypassAddress = (uint8_t)flag;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void savePreventOverFlowSettings(bool flag)
{
	user_settings_parameter_struct.preventOverFlowAddress = (uint8_t)flag;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setJumperSettings(uint8_t jumperVal)
{
	user_settings_parameter_struct.jumperSettingAddress = jumperVal;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

bool setOverloadPer(uint8_t overloadPerValue)
{
	if(overloadPerValue>100)
	{
		user_settings_parameter_struct.overloadPerAddress = overloadPerValue;
		memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
		eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
		
		if (user_settings_parameter_struct.currentDetectionAddress)
		{
			calcCurrentValues();
		}
		return true;
	}
	return false;
}

bool setUnderloadPer(uint8_t underloadPerValue)
{
	if(underloadPerValue>0 && underloadPerValue <100)
	{
		user_settings_parameter_struct.underloadPerAddress = underloadPerValue;
		memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
		eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
		eeprom_emulator_commit_page_buffer();
		if (user_settings_parameter_struct.currentDetectionAddress)
		{
			calcCurrentValues();
		}
		return true;
	}
	return false;
}

void saveSinglePhasingSettings(bool singlePhasing)
{
	user_settings_parameter_struct.detectSinglePhasing=singlePhasing;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveSinglePhasingVoltage(uint16_t voltage)
{
	user_settings_parameter_struct.singlePhasingVoltage= voltage;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void savePhaseSequenceProtectionSettings(bool phaseSequenceSetting)
{
	user_settings_parameter_struct.detectPhaseSequence= phaseSequenceSetting;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}


void calcCurrentValues(void)
{
	uint16_t temp = (user_settings_parameter_struct.normalLoadAddress * (float)user_settings_parameter_struct.underloadPerAddress) / 100.0;
	setUnderloadValue(temp);

	temp = (user_settings_parameter_struct.normalLoadAddress * (float)user_settings_parameter_struct.overloadPerAddress) / 100.0;
	setOverloadValue(temp);
}

void setUnderloadValue(uint32_t underValue)
{
	user_settings_parameter_struct.underloadAddress = underValue;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setOverloadValue(uint32_t overValue)
{
	user_settings_parameter_struct.overloadAddress = overValue;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setNormalLoadValue(uint32_t normalVal)
{
	user_settings_parameter_struct.normalLoadAddress = normalVal;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setCurrentDetection(bool cValue)
{
	user_settings_parameter_struct.currentDetectionAddress = cValue;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

char *getDeviceId(char *deviceID)
{
	strcpy(deviceID,factory_settings_parameter_struct.DeviceID_ee);
	return deviceID;
}

void saveStarDeltaTimer(uint16_t StartDeltaTime)
{
	user_settings_parameter_struct.starDeltaTimerAddress = StartDeltaTime;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void saveEventStageSettings(uint8_t data)
{
	user_settings_parameter_struct.eventStageAddress = data;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

bool isAdmin(char *number)
{
	for (uint8_t i=0;i<5;i++)
	{
		struct mobile_no_struct mobile_no;
		memset(page_data, '\0', sizeof(page_data));
		eeprom_emulator_read_page(ADMIN_1_MOBILE_NUMBER_PAGE+i, page_data);
		memcpy(&mobile_no,page_data,sizeof(mobile_no));
		
		if (strstr(number,mobile_no.mobile_no_ee))
		{
			return true;
		}
	}
	return false;
}


void setPrimaryNumberIndex(uint8_t index)
{
	user_count_struct.primaryNumberIndex = index;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

void setSecondaryNumberIndex(uint8_t index)
{
	user_count_struct.secondaryNumberIndex = index;
	memcpy(page_data,&user_settings_parameter_struct,sizeof(user_settings_parameter_struct));
	eeprom_emulator_write_page(USER_SETTING_PARAMETERS_PAGE, page_data);
	eeprom_emulator_commit_page_buffer();
}

bool addPrimaryIndexedNumber(char *number)
{
	//checkUserExists would return a 1 based index,starting at 1 and not at 0
	uint8_t index = checkExists(number);

	uint8_t newPrimaryIndex=0xFF;
	if(index >= user_count_struct.total_user_no_count)								//the numebr does not exists, need to add it
	{
		newPrimaryIndex = user_count_struct.current_user_no_count;
		addNumber(number);
	}
	else if(index<user_count_struct.total_user_no_count)
	{
		newPrimaryIndex = index;
	}

	if(newPrimaryIndex < user_count_struct.total_user_no_count)
	{
		setPrimaryNumberIndex(newPrimaryIndex);
		return true;
	}

	return false;
}

bool addSecondaryIndexedNumber(char *number)
{
	//check if any 2 numbers are present in system one of which would be primary, than only allow to add secondary number
	if(user_count_struct.current_user_no_count<2)
	return false;

	//checkUserExists would return a 1 based index,starting at 1 and not at 0
	uint8_t index = checkExists(number);

	uint8_t newSecondaryIndex=0xFF;
	if(index >= user_count_struct.total_user_no_count)								//number not present, need to add the number
	{
		newSecondaryIndex = user_count_struct.current_user_no_count;
		addNumber(number);
	}
	else if(index < user_count_struct.total_user_no_count)
	{
		newSecondaryIndex = index;
	}

	if(newSecondaryIndex < user_count_struct.total_user_no_count)
	{
		setSecondaryNumberIndex(newSecondaryIndex);
		return true;
	}
	return false;
}

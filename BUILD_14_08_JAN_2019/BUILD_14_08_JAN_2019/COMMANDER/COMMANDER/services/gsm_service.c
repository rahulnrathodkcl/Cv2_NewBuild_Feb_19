#include "gsm_service.h"

static void vTask_GSM_service(void *params);


void setObtainEvent(void)
{
	if (!obtainNewEvent  && xTaskGetTickCount() - obtainEventTimer > 1000)
	{
		obtainNewEvent = true;
	}
}

void operateOnStagedEvent(void)
{
	uint8_t temp1= 1;
	if(eventStaged)
	{
		temp1 = user_settings_parameter_struct.eventStageAddress;
	}
	
	if(obtainNewEvent && (retries==1 || (xTaskGetTickCount()-tempEventStageTime>((unsigned long)temp1*60000L))))
	{
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			if (eventStaged)
			{
				actionType=stagedEventType;
				eventStaged=false;
			}
			else if (m2mEventStaged)
			{
				m2mEventNo=stagedEventType;
				m2mEvent=true;
				m2mEventStaged=false;
			}
			makeResponseAction();
		}
	}
}

void makeResponseAction(void)
{
	if(user_settings_parameter_struct.responseAddress != 'N' || m2mEvent)
	{
		freezeIncomingCalls = true;
		//acceptCommands();
		makeCall();
	}
}

void endCall(void)
{
	nr  = 0;
	inCall=false;
	gsm_hangup_call();
	//unsigned long temp = xTaskGetTickCount();
	vTaskDelay(1000);
	freezeIncomingCalls = false;
	if (factory_settings_parameter_struct.ENABLE_CURRENT)
	{
		zeroPressed=false;
	}
	
	if ((factory_settings_parameter_struct.ENABLE_M2M == true)?
	((currentStatus == 'N' || currentStatus == 'R') && currentCallStatus == 'O' && !m2mEvent):
	((currentStatus == 'N' || currentStatus == 'R') && currentCallStatus == 'O'))
	{
		if((retries==0) && !callAccepted && (user_settings_parameter_struct.responseAddress=='T') && (user_count_struct.current_user_no_count>1))
		{
			retries=1;
			eventStaged=true;
			tempEventStageTime=xTaskGetTickCount();
			stagedEventType=actionType;
		}
	}
	
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		if((currentStatus == 'I' || currentStatus=='R') && currentCallStatus == 'O' && m2mEvent && m2m_Numbers_struct.m2mSetting)
		{
			m2mEventCalls++;
			if(m2mAck)
			{
				setM2MEventState(m2mEventNo,ME_CLEARED);
			}
			else
			{
				if(m2mEventCalls<2)
				{
					tempEventStageTime=xTaskGetTickCount();
					stagedEventType=m2mEventNo;
					m2mEventStaged=true;
				}
				else
				{
					setM2MEventState(m2mEventNo,ME_NOTAVAILABLE);
				}
			}
			m2mEvent = false;
		}
		keyPressed=false;
		m2mAck=false;
	}
	callAccepted = false;
	currentStatus = 'N';
	currentCallStatus = 'N';

	isRegisteredNumber=false;
	obtainEventTimer = xTaskGetTickCount();
	obtainNewEvent = false;
}


void makeCall(void)
{
	inCall=true;
	
	char command[20] = {0};
	
	if(m2m_Numbers_struct.m2mSetting && m2mEvent)
	{
		getM2MRemoteNumber(command);
	}
	else
	{
		if(retries)
		{
			getIndexedNumber(command,user_count_struct.secondaryNumberIndex);
		}
		else
		{
			getActiveNumber(command);
		}
	}
	gsm_call_to_dial_a_number(command);
	callCutWait = xTaskGetTickCount();
	currentStatus = 'R';
	currentCallStatus = 'O';
}

void acceptCall(void)
{
	isRegisteredNumber=false;   //clear flag for next call, in case any error occures and endCall() is not called for ending the call
	callAccepted = true;
	gsm_answer_an_incomming_call();
	currentStatus = 'I';
	currentCallStatus = 'I';
	playSound('M',true);
}

//void playSound(char actionType, bool newAction=true)
void playSound(char actionTypeT, bool newAction)
{
	gsm_stop_play_record_file();
	bplaySound = true;
	if (newAction)
	{
		maxPlayingFiles=1;
		currentPlayingFileIndex=0;
		playFilesList[currentPlayingFileIndex]=actionTypeT;
		playFilesList[currentPlayingFileIndex+1]='\0';
		actionType = actionTypeT;
	}
	playFile = actionTypeT;
}

bool playSoundElligible(void)
{
	return (bplaySound && ((xTaskGetTickCount() - soundWait) > (soundWaitTime * 100)));
}

void triggerPlaySound(void)
{
	if(maxPlayingFiles>1)
	{
		gsm_play_record_file((char*)playFile,false);
	}
	else
	{
		gsm_play_record_file((char*)playFile,true);
	}
	bplaySound = false;
}

void playSoundAgain(char *string)
{
	if (!bplaySound && gsm_responseLine_is_StopSound_Received(string))
	{
		if(maxPlayingFiles>1)
		{
			if(currentPlayingFileIndex<maxPlayingFiles-1)
			{
				playSound(playFilesList[++currentPlayingFileIndex],false);
			}
			else
			{
				playSound('M',true);
			}
		}
	}
}

void playRepeatedFiles(char *fileList)
{
	if(strlen(fileList)<8)
	{
		currentPlayingFileIndex=0;
		maxPlayingFiles=strlen(fileList);
		strcpy(playFilesList,fileList);
		soundWait = xTaskGetTickCount();
		playFile = playFilesList[currentPlayingFileIndex];
		bplaySound = true;
	}
}

bool callTimerExpire(void)
{
	return ((xTaskGetTickCount() - callCutWait) >= (callCutWaitTime * 100));
}

char OutGoingcallState(char *response)
{
	if(strstr(response,"+CLCC: 1,0,2"))
	{
		return 'D';								//dialling
	}
	else if (strstr(response,"+CLCC: 1,0,3"))
	{
		return 'R';								//alerting
	}
	else if (strstr(response,"+CLCC: 1,0,0"))
	{
		return 'I';								//active call
	}
	else if (strstr(response,"+CLCC: 1,0,6"))
	{
		return 'E';								//call ended
	}
	else
	{
		return 'N';
	}
}

bool registerEvent(char eventType)
{
	//if(isSIMReset())
	//{
	//return false;
	//}
	if(user_count_struct.current_user_no_count==0 || user_settings_parameter_struct.responseAddress=='N')
	{
		return true;
	}
	if (!initialized)
	{
		return true;
	}
	if(!eventStaged && actionType==eventType)
	{
		return true;
	}
	//if ((factory_settings_parameter_struct.ENABLE_M2M==true)?
	//(currentStatus == 'N' && currentCallStatus == 'N' && obtainNewEvent && !eventStaged && !m2mEventStaged):
	//(currentStatus == 'N' && currentCallStatus == 'N' && obtainNewEvent && !eventStaged))
	if(currentStatus == 'N' && currentCallStatus == 'N' && obtainNewEvent && !eventStaged && !m2mEventStaged)
	{
		if(user_settings_parameter_struct.noCallAddress && checkNoCallTime())
		{
			return true;
		}
		retries=0;
		if(user_settings_parameter_struct.eventStageAddress>0x00)
		{
			tempEventStageTime=xTaskGetTickCount();
			stagedEventType=eventType;
			eventStaged=true;
		}
		else
		{
			actionType = eventType;
			makeResponseAction();
		}
		return true;
	}
	else
	{
		return false;
	}
}

void registerM2MEvent(uint8_t eventNo)
{
	if (!initialized)
	{
		setM2MEventState(eventNo,ME_CLEARED);
		return;
	}

	if (currentStatus == 'N' && currentCallStatus == 'N' && obtainNewEvent && !eventStaged && !m2mEventStaged)
	{
		setM2MEventState(eventNo,ME_SERVICING);
		m2mEvent=true;
		m2mEventNo = eventNo;
		m2mEventCalls=0;
		makeResponseAction();
		return;
	}
}

void setMotorMGRResponse(char response)
{
	if(currentStatus!='I')    // not in Call than return.
	{
		return;
	}
	playSound(response,true);
}

void checkRespSMS(char t1)
{
	if (!callAccepted && user_settings_parameter_struct.responseAddress=='A')
	{
		actionType = t1;
		sendSMS("",false,false);
	}
}

void subDTMF(void)
{
	gsm_stop_play_record_file();
	callCutWait = xTaskGetTickCount();
}

void processOnDTMF(char *dtmf_cmd)
{
	char dtmf = dtmf_cmd[0];
	
	//LCD_clear();
	//lcd_printf("%c",dtmf);
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		if (m2mEvent)
		{
			if(dtmf == 'A')
			{
				m2mAck=true;
				gsm_hangup_call();
			}
		}
	}
	else
	{
		if (dtmf == '1') //Motor On
		{
			subDTMF();
			startMotor(true);
		}
		else if (dtmf == '2') //Motor Off
		{
			subDTMF();
			stopMotor(true,false,false);
		}
		else if (dtmf == '3') //Status
		{
			subDTMF();
			statusOnCall();
		}
		else if (dtmf == '4') //underground status
		{
			if (factory_settings_parameter_struct.ENABLE_WATER)
			{
				subDTMF();
				waterStatusOnCall(false);
			}
		}
		else if (dtmf == '5') //overHead Status
		{
			if (factory_settings_parameter_struct.ENABLE_GP)
			{
				subDTMF();
				overHeadWaterStatusOnCall(false);
			}
		}
		else if(dtmf == '0')
		{
			if (factory_settings_parameter_struct.ENABLE_CURRENT)
			{
				if(zeroPressed)
				{
					autoSetCurrent();   //to enable or disable current detection
					subDTMF();
					zeroPressed=false;
				}
				else
				{
					zeroPressed=true;
				}
			}
		}
		else if (dtmf == '7') //Speak Current Ampere On Call
		{
			if (factory_settings_parameter_struct.AMPERE_SPEAK)
			{
				subDTMF();
				speakAmpere();
			}
		}
		else if(dtmf=='D')
		{
			// currentOperation = '1';   //m2m 1
			saveAutoStartSettings(true);  //set AutoStart to True in EEPROM
			resetAutoStart(true);
			startMotor(false);
			sendDTMFTone(0xFF);
		}
		else if(dtmf=='C')
		{
			// currentOperation = '2';   //m2m 2
			saveAutoStartSettings(false);  //set AutoStart to false in EEPROM
			stopMotor(false,false,true);
			sendDTMFTone(0xFF);
		}
		else if (dtmf == '8') //Set AUTOTIMER ON
		{
			subDTMF();
			saveAutoStartSettings(true);  //set AutoStart to True in EEPROM
			resetAutoStart(true);
			playSound(')',true);     // playFile AutoStart is On
		}
		else if (dtmf == '9') //Set AUTOTIMER OFF
		{
			subDTMF();
			saveAutoStartSettings(false);  //set AUtoStart to False in EEPROM
			resetAutoStart(true);
			playSound('[',true); //playFile autoStart is turned oFF
		}
	}
}

void processOnSMS(char *received_command, bool admin,bool response_sms_processed_cmd,bool alterNumber, char *phone_number)
{
	char resep_msg[250];

	if(alterNumber)
	{
		if(!StringstartsWith(received_command,"AMON"))
		return;
	}

	if (StringstartsWith(received_command,"WAIT"))
	{
		enum gsm_error err = gsm_disable_call_waiting();
		if (response_sms_processed_cmd == true)
		{
			if (err == GSM_ERROR_NONE)
			{
				strcpy(resep_msg,"GSM CALL WAITTING DISABLE : SUCCESS");
			}
			else
			{
				strcpy(resep_msg,"GSM CALL WAITTING DISABLE : FAILED");
			}
		}
	}
	else if (StringstartsWith(received_command,"CLEARALL"))
	{
		clearNumbers(admin);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"ALL USER NUMBERS CLEARED");
		}
	}
	else if (StringstartsWith(received_command,"DEFAULT"))
	{
		saveAutoStartSettings(false);
		if (factory_settings_parameter_struct.ENABLE_WATER)
		{
			saveWaterBypassSettings(false);
			if (factory_settings_parameter_struct.ENABLE_M2M)
			{
				saveM2MSettings(false);
			}
			else
			{
				savePreventOverFlowSettings(false);
			}
		}
		if (factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			setOverloadPer(120);
			setUnderloadPer(85);
			setCurrentDetection(false);
		}
		saveEventStageSettings(0);
		//saveBypassSettings(false);
		saveDNDSettings(false);
		saveResponseSettings('C');
		saveAutoStartTimeSettings(50);
		saveStarDeltaTimer(2);
		
		saveSinglePhasingSettings(true);
		saveSinglePhasingVoltage(80);
		savePhaseSequenceProtectionSettings(true);
		setPrimaryNumberIndex(0);
		setSecondaryNumberIndex(1);
		
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"ALL SETTINGS ARE ARE NOW FACTORY DEFAULT");
		}
	}
	else if (StringstartsWith(received_command,"NUM"))
	{
		response_sms_processed_cmd = true;
		getNumbers(resep_msg);
		if (strstr(resep_msg,""))
		{
			memset(resep_msg, '\0', sizeof(resep_msg));
			strcpy(resep_msg,"No Numbers Exists");
		}
	}
	else if (StringstartsWith(received_command,"RESET"))
	{
		system_reset();
	}
	else if (StringstartsWith(received_command,"DID"))
	{
		response_sms_processed_cmd=true;
		strcpy(resep_msg,factory_settings_parameter_struct.DeviceID_ee);

		//sprintf(resep_msg,"Software:%s\nModel:%d\nDeviceId:%lu\nHW:%d",
		//SOFTWARE_VER,factory_parameter_struct.u16tmodelNo,factory_parameter_struct.u32deviceId,
		//factory_parameter_struct.u16thardwareVer);

	}
	else if (StringstartsWith(received_command,"AUTOON"))
	{
		saveAutoStartSettings(true);
		resetAutoStart(true);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"MOTOR AUTOON ON");
		}
	}
	else if (StringstartsWith(received_command,"AUTOOFF"))
	{
		saveAutoStartSettings(false);
		resetAutoStart(true);
	}
	else if (StringstartsWith(received_command,"WBYPON"))
	{
		if (factory_settings_parameter_struct.ENABLE_WATER)
		{
			saveWaterBypassSettings(true);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"WATER BYPASS ON");
			}
		}
	}
	else if (StringstartsWith(received_command,"WBYPOFF"))
	{
		if (factory_settings_parameter_struct.ENABLE_WATER)
		{
			saveWaterBypassSettings(false);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"WATER BYPASS OFF");
			}
		}
	}
	else if (StringstartsWith(received_command,"SPPON"))
	{
		saveSinglePhasingSettings(true);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"SPP ON");
		}
	}
	else if (StringstartsWith(received_command,"SPPOFF"))
	{
		saveSinglePhasingSettings(false);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"SPP OFF");
		}
	}
	else if(StringstartsWith(received_command,"SPPV"))
	{
		memmove(received_command,received_command+4,strlen(received_command));
		uint8_t sppVoltage = atoi(received_command);
		if(sppVoltage<20) sppVoltage=20;
		if(sppVoltage>440) sppVoltage=440;
		saveSinglePhasingVoltage(sppVoltage);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"SPP VOLT SET");
		}
	}
	else if(StringstartsWith(received_command,"SEQON"))
	{
		savePhaseSequenceProtectionSettings(true);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"SEQP ON");
		}
	}
	else if(StringstartsWith(received_command,"SEQOFF"))
	{
		savePhaseSequenceProtectionSettings(false);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"SEQP OFF");
		}
	}
	else if (StringstartsWith(received_command,"DND"))
	{
		if (strlen(received_command)>3)
		{
			memmove(received_command,received_command+3,strlen(received_command));
			if(received_command[0]=='L' || received_command[0]=='S' || received_command[0]=='O')
			{
				saveDNDSettings((char)received_command);  //save specific RESPONSE settings
				if (response_sms_processed_cmd == true)
				{
					strcpy(resep_msg,"DND : ");
					strcat(resep_msg,received_command);
					strcat(resep_msg," OK");
				}
			}
			else
			{
				if (response_sms_processed_cmd == true)
				{
					strcpy(resep_msg,"DND ERROR");
				}
			}
		}
	}
	else if (StringstartsWith(received_command,"RESP"))
	{
		if (strlen(received_command)>4)
		{
			memmove(received_command,received_command+4,strlen(received_command));
			if(received_command[0]=='C' || received_command[0]=='A' || received_command[0]=='T' || received_command[0]=='N')
			{
				saveResponseSettings((char)received_command);  //save specific RESPONSE settings
				if (response_sms_processed_cmd == true)
				{
					strcpy(resep_msg,"RESP : ");
					strcat(resep_msg,received_command);
					strcat(resep_msg," OK");
				}
			}
			else
			{
				if (response_sms_processed_cmd == true)
				{
					strcpy(resep_msg,"RESP ERROR");
				}
			}
		}
	}
	//else if (StringstartsWith(received_command,"SJMP"))
	//{
	//strcpy(resep_msg,"New hardware does not required Jumper Setting");
	//}
	else if (StringstartsWith(received_command,"OVR"))
	{
		memmove(received_command,received_command+3,strlen(received_command));
		uint8_t ovr_per = atoi(received_command);
		if(ovr_per>104)
		{
			setOverloadPer(ovr_per);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"OVR SET OK");
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"UNDR"))
	{
		memmove(received_command,received_command+4,strlen(received_command));
		uint8_t undr_per = atoi(received_command);
		if(undr_per<98 && undr_per>0)
		{
			setUnderloadPer(undr_per);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"UNDR SET OK");
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"ASTAT"))
	{
		if(factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			sprintf(resep_msg,"C:%u.%u\nN:%u\nO:%u\nU:%u\nOP:%u\nUP:%u",
			Analog_Parameter_Struct.Motor_Current_IntPart,
			Analog_Parameter_Struct.Motor_Current_DecPart,
			user_settings_parameter_struct.normalLoadAddress,
			user_settings_parameter_struct.overloadAddress,
			user_settings_parameter_struct.underloadAddress,
			user_settings_parameter_struct.overloadPerAddress,
			user_settings_parameter_struct.underloadPerAddress);
			response_sms_processed_cmd = true;
		}

	}
	else if (StringstartsWith(received_command,"OVFON"))
	{
		if (factory_settings_parameter_struct.ENABLE_WATER && !(factory_settings_parameter_struct.ENABLE_M2M))
		{
			savePreventOverFlowSettings(true);  //set DND to False in EEPROM
			if(response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"PREVENT OVERFLOW SETTING ON");
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"OVFOFF"))
	{
		if (factory_settings_parameter_struct.ENABLE_WATER && !(factory_settings_parameter_struct.ENABLE_M2M))
		{
			savePreventOverFlowSettings(false);  //set DND to False in EEPROM
			if(response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"PREVENT OVERFLOW SETTING OFF");
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"M2MON"))
	{
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			if(m2m_Numbers_struct.m2mRemotePresent && !(m2m_Numbers_struct.m2mRemoteVerified))
			{
				response_sms_processed_cmd = false;
				verifyRemoteNumber();
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"M2MOFF"))
	{
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			saveM2MSettings(false);
			if(response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"M2M SETTING OFF");
			}
		}
		else
		{
			response_sms_processed_cmd = false;
		}
	}
	else if (StringstartsWith(received_command,"STATUS"))
	{
		//todo: implement STATUS msg
		buildStatusMessage(&resep_msg);
	}
	else if (StringstartsWith(received_command,"AMON") && (admin || alterNumber))
	{
		if (alternateNumber_struct.alterNumberPresent)
		{
			saveAlterNumberSetting(true);
			if(response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"ALTERNATE NUMBER ON");
			}
		}
	}
	else if (StringstartsWith(received_command,"AMOFF"))
	{
		saveAlterNumberSetting(false);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"ALTERNATE MOBILE NUMBER OFF");
		}
	}
	else if (StringstartsWith(received_command,"NCOFF"))
	{
		saveNoCallSettings(false,0,0,0,0);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"NO CALL SETTING OFF");
		}
	}
	else if (StringstartsWith(received_command,"GETTIME"))
	{
		uint8_t globalHours,globalMinutes;
		getSystemTime(&globalHours,&globalMinutes);
		
		sprintf(resep_msg,"TIME : %u:%u",
		globalHours,globalMinutes);
		response_sms_processed_cmd = true;
	}
	else if (StringstartsWith(received_command,"SETTIME"))
	{
		//todo: implement
		//set internal RTC of Either MCU or SIM800 to check no call time
		

	}
	else if (StringstartsWith(received_command,"NCTIME"))
	{
		char *ptrclcc;
		uint8_t startHH, startMM, stopHH, stopMM;

		ptrclcc = strtok(received_command,"-");
		ptrclcc = strtok(NULL,":");
		startHH=atoi(*ptrclcc);
		ptrclcc = strtok(NULL,"-");
		startMM=atoi(*ptrclcc);
		ptrclcc = strtok(NULL,":");
		stopHH=atoi(*ptrclcc);
		ptrclcc = strtok(NULL,":");
		stopMM=atoi(*ptrclcc);

		if(startHH>=0 && startHH<24 && startMM>=0 && startMM<60)
		{
			if(stopHH>=0 && stopHH<24 && stopMM>=0 && stopMM<60)
			{
				if(startHH==stopHH && startMM==stopMM)
				{
					saveNoCallSettings(false,0,0,0,0);
				}
				else
				{
					saveNoCallSettings(true,startHH,startMM,stopHH,stopMM);
					if(response_sms_processed_cmd)
					{
						strcpy(resep_msg,"NO CALL TIMINGS SET");
					}
				}
			}
		}
	}
	else if (StringstartsWith(received_command,"STAGE"))
	{
		memmove(received_command,received_command+5,strlen(received_command));
		uint16_t stageLevel = atoi(received_command);
		if (stageLevel < 0) stageLevel = 0;
		if (stageLevel > 5) stageLevel = 5;
		saveEventStageSettings(stageLevel);  //Store in EEPROM the EVENT STAGE
		if(response_sms_processed_cmd)
		{
			sprintf(resep_msg,"CALL STAGE SET TO : %d", stageLevel);
		}
	}
	else if (StringstartsWith(received_command,"STARTIME"))
	{
		memmove(received_command,received_command+8,strlen(received_command));
		uint16_t STARTIME = atoi(received_command);
		if (STARTIME < 2)
		{
			STARTIME = 2;
		}
		if (STARTIME > 1200)
		{
			STARTIME = 1200;
		}
		saveStarDeltaTimer(STARTIME);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"STARTIME : ");
			strcpy(resep_msg,STARTIME);
		}
	}
	else if (StringstartsWith(received_command,"AUTOTIME"))
	{
		memmove(received_command,received_command+8,strlen(received_command));
		uint16_t AUTOTIME = atoi(received_command);
		if (AUTOTIME < 50)
		{
			AUTOTIME = 50;
		}
		if (AUTOTIME > 28800)
		{
			AUTOTIME = 28800;
		}
		saveAutoStartTimeSettings(AUTOTIME);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"AUTOTIME : ");
			strcpy(resep_msg,AUTOTIME);
		}
	}
	else if (StringstartsWith(received_command,"BAL"))
	{
		//the sms here will have the URC number needed to check the balance
		// The received sms will be like BAL*141# for vodafone
		// We need to extract the URC number and check the balance, and send it to user
		//todo: implement mechanism to check current balance of sim card
		
	}
	else if(StringstartsWith(received_command,"MP+"))
	{
		memmove(received_command,received_command+3,strlen(received_command));
		if(addPrimaryIndexedNumber(received_command))
		{
			strcpy(resep_msg,"P NO ADDDED");
		}
	}
	else if(StringstartsWith(received_command,"MS+"))
	{
		memmove(received_command,received_command+3,strlen(received_command));
		if(addSecondaryIndexedNumber(received_command))
		{
			strcpy(resep_msg,"S NO ADDDED");
		}
	}
	else if (StringstartsWith(received_command,"M+"))
	{
		memmove(received_command,received_command+2,strlen(received_command));
		addNumber(received_command);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"MOBILE NO:");
			strcat(resep_msg,"\n");
			strcat(resep_msg,received_command);
			strcat(resep_msg,"\n");
			strcat(resep_msg,"ADDED SUCCESSFULLY");
		}
	}
	else if (StringstartsWith(received_command,"M-"))
	{
		memmove(received_command,received_command+2,strlen(received_command));
		removeNumber(received_command);
		if (response_sms_processed_cmd == true)
		{
			strcpy(resep_msg,"MOBILE NO:");
			strcat(resep_msg,"\n");
			strcat(resep_msg,received_command);
			strcat(resep_msg,"\n");
			strcat(resep_msg,"REMOVED SUCCESSFULLY");
		}
	}
	else if (StringstartsWith(received_command,"AM+"))
	{
		//if (isNumeric(str))
		{
			memmove(received_command,received_command+3,strlen(received_command));
			bool t  = addAlternateNumber(received_command);
			
			if (response_sms_processed_cmd == true)
			{
				if (t == true)
				{
					strcpy(resep_msg,"ALTERNATE MOBILE NO ADD SUCCESS");
				}
				else
				{
					strcpy(resep_msg,"ALTERNATE MOBILE NO ADD FAILED");
					
				}
			}
			
		}
	}
	else if (StringstartsWith(received_command,"MM+"))
	{
		//if (isNumeric(str))
		{
			memmove(received_command,received_command+3,strlen(received_command));
			addM2MNumber(received_command);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"M2M NO. ADDED");
			}
		}
	}
	
	else if (StringstartsWith(received_command,"MR+"))
	{
		//if (isNumeric(str))
		if(factory_settings_parameter_struct.ENABLE_M2M)
		{
			memmove(received_command,received_command+3,strlen(received_command));
			
			addM2MRemoteNumber(received_command);
			if (response_sms_processed_cmd == true)
			{
				strcpy(resep_msg,"M2M REMOTE NO. ADDED");
			}
		}
	}
	if (response_sms_processed_cmd == true)
	{
		gsm_send_sms(phone_number,resep_msg);
	}
}

void buildStatusMessage(char *resep_msg)
{
	uint8_t network= gsm_getsignalstrength();
	char strACState[10],strSeq[7],strMotor[5];
	switch(structThreePhase_state.u8t_phase_ac_state)
	{
		case AC_3PH:
		sprintf(strACState,(const uint8_t*)("ON"));
		break;
		case AC_2PH:
		sprintf(strACState,(const uint8_t*)("2 PHASE"));
		break;
		default:
		sprintf(strACState,(const uint8_t*)("OFF"));
		break;
	}
	
	if(structThreePhase_state.u8t_phase_sequence_flag == THREEPHASE_OK)
	{
		sprintf(strSeq,(const uint8_t*)("OK"));
	}
	else
	{
		sprintf(strSeq,(const uint8_t*)("ERROR"));
	}
	
	if(getMotorState())
	{
		sprintf(strMotor,(const uint8_t*)("ON"));
	}
	else
	{
		sprintf(strMotor,(const uint8_t*)("OFF"));
	}
	
	sprintf(resep_msg,"RY:%d YB:%d BR:%d\nAC:%s\nSequence:%s\nMotor:ON\nCurrent:%dA\nNetwork:%d",
	Analog_Parameter_Struct.PhaseRY_Voltage,Analog_Parameter_Struct.PhaseYB_Voltage,Analog_Parameter_Struct.PhaseBR_Voltage,
	strACState,strSeq,Analog_Parameter_Struct.Motor_Current_IntPart,network);
}


bool checkNumber(char *number)
{
	if ((isAdmin(number)) || (checkExists(number)<user_count_struct.total_user_no_count))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool checkNoCallTime(void)
{
	uint8_t globalHours, globalMinutes;
	getSystemTime(&globalHours,&globalMinutes);
	if(globalHours >= (user_settings_parameter_struct.noCallStartTimeHourAddress) && globalHours <=(user_settings_parameter_struct.noCallStopTimeHourAddress))
	{
		if(user_settings_parameter_struct.noCallStartTimeHourAddress==user_settings_parameter_struct.noCallStopTimeHourAddress)
		{
			if(globalMinutes>=(user_settings_parameter_struct.noCallStartTimeMinuteAddress) && globalMinutes<=(user_settings_parameter_struct.noCallStopTimeMinuteAddress))
			return true;
			else
			return false;
		}
		if(globalHours==user_settings_parameter_struct.noCallStartTimeHourAddress)
		{
			if(globalMinutes>=user_settings_parameter_struct.noCallStartTimeMinuteAddress)
			return true;
			else
			return false;
		}
		if(globalHours==user_settings_parameter_struct.noCallStopTimeHourAddress)
		{
			if(globalMinutes<=(user_settings_parameter_struct.noCallStopTimeMinuteAddress))
			return true;
			else
			return false;
		}
		if(globalHours > (user_settings_parameter_struct.noCallStartTimeHourAddress) && globalHours < (user_settings_parameter_struct.noCallStopTimeHourAddress))
		return true;
	}
	return false;
}

void verifyRemoteNumber(void)
{
	sendSMS(("VMM01"),true,SEND_TO_M2M_REMOTE);
}

void sendSMS(char *msg, bool predefMsg, uint8_t isM2M)  ////void sendSMS(char *msg, bool predefMsg = false, uint8_t isM2M);
{
	inCall=true;
	if (!predefMsg)
	{
		switch(actionType)
		{
			case 'S':
			strcat(msg,STR_MOTOR);
			strcat(msg,STR_ON);
			break;
			case 'O':
			case 'U':
			case 'C':
			case 'F':
			strcat(msg,STR_MOTOR);
			strcat(msg,STR_OFF);
			break;
			default:
			return;
		}
	}
	
	char phone_number[20];
	
	if(isM2M==SEND_TO_M2M_MASTER)
	{
		getM2MNumber(phone_number);
	}
	else if (isM2M==SEND_TO_M2M_REMOTE)
	{
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			getM2MRemoteNumber(phone_number);
		}
	}
	else
	{
		if (isMsgFromAdmin)
		{
			struct mobile_no_struct mobile_no;
			memset(page_data, '\0', sizeof(page_data));
			eeprom_emulator_read_page(ADMIN_1_MOBILE_NUMBER_PAGE, page_data);
			memcpy(&mobile_no,page_data,sizeof(mobile_no));
			strcpy(phone_number,mobile_no.mobile_no_ee);
		}
		else
		{
			getActiveNumber(phone_number);
		}
	}
	gsm_send_sms(phone_number,msg);
	isMsgFromAdmin = false;
	inCall=false;
}

void sendDTMFTone(uint8_t eventNo)
{
	if(eventNo==0xFF)
	{
		gsm_send_DTMF_Tone('A');
	}
	else if (eventNo==0 && factory_settings_parameter_struct.ENABLE_M2M)
	{
		gsm_send_DTMF_Tone('D');
	}
	else if (eventNo==1 && factory_settings_parameter_struct.ENABLE_M2M)
	{
		gsm_send_DTMF_Tone('C');
	}
}

void getSystemTime(uint8_t *Hours, uint8_t *Minutes)
{
	if (gsm_get_internal_rtc_time() == GSM_ERROR_NONE)
	{
		*Hours = struct_internal_rtc.Network_hour;
		*Minutes = struct_internal_rtc.Network_minute;
	}
}

//////////////////////////////////////////////////////////////////////////
//------------GSM OPERATION----------------------$$$$$$$$$$$$$$
#define GSM_STATUS_POSITION		PIN_PA27
#define GSM_STATUS_OK			port_pin_get_input_level(GSM_STATUS_POSITION)
#define GSM_STATUS_ERROR		(!port_pin_get_input_level(GSM_STATUS_POSITION))

#define GSM_PWR_DDR		REG_PORT_DIR1
#define GSM_PWR_PORT	REG_PORT_OUT1
#define GSM_PWR_POS		PORT_PB16
#define GSM_PWR_AS_OP	GSM_PWR_DDR|=GSM_PWR_POS
#define GSM_PWR_ON		GSM_PWR_PORT|=GSM_PWR_POS
#define GSM_PWR_OFF		GSM_PWR_PORT&=~(GSM_PWR_POS)
//////////////////////////////////////////////////////////////////////////
static void vTask_GSM_service(void *params)
{
	
	uint32_t network_update_time = 0;
	
	GSM_PWR_AS_OP;
	
	struct port_config pin_conf_gsm_status;
	port_get_config_defaults(&pin_conf_gsm_status);
	pin_conf_gsm_status.direction  = PORT_PIN_DIR_INPUT;
	pin_conf_gsm_status.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(GSM_STATUS_POSITION, &pin_conf_gsm_status);
	
	gsm_init();
	
	initialized = false;
	
	inCall=false;
	simReInit=false;
	
	bool boolGsm_config_flag			=false;
	bool boolOne_Time_Msg_Delete_Flag   =false;
	
	Signal_Strength = 0;
	
	soundWaitTime = 5;
	bplaySound = false;
	
	actionType = 'N';
	callCutWaitTime = 580;
	nr = 0;
	currentStatus = 'N';
	currentCallStatus = 'N';
	callAccepted = false;
	freezeIncomingCalls = false;
	obtainNewEvent = true;
	isMsgFromAdmin = false;
	eventStaged=false;
	stagedEventType = 'N';
	isRegisteredNumber=false;
	retries=0;
	if (factory_settings_parameter_struct.ENABLE_CURRENT)
	{
		zeroPressed=false;
	}
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		m2mAck=false;
		m2mEventCalls=m2mEventNo=0;
		m2mEventStaged=false;
		m2mEvent=false;
		keyPressed=false;
	}
	
	if(GSM_STATUS_OK)
	{
		boolGsm_config_flag			=false;
		boolOne_Time_Msg_Delete_Flag   =false;
		
		GSM_PWR_ON;
		vTaskDelay(3000);
		GSM_PWR_OFF;
		vTaskDelay(5000);
	}
	
	for (;;)
	{
		if (GSM_STATUS_OK)
		{
			if (boolGsm_config_flag == false)
			{
				if (gsm_is_network_registered() == GSM_NETWORK_REGISTERED)
				{
					if(gsm_config_module()==GSM_ERROR_NONE)
					{
						for (uint8_t i=0;i<20;i++)
						{
							Signal_Strength = gsm_getsignalstrength();
							vTaskDelay(50);
						}
						boolGsm_config_flag = true;
					}
					else
					{
						boolGsm_config_flag = false;
					}
				}
				else
				{
					vTaskDelay(2000/portTICK_PERIOD_MS);
				}
			}
			else
			{
				if ((boolOne_Time_Msg_Delete_Flag == false) && (boolGsm_config_flag == true))
				{
					if (gsm_delete_all_sms() == GSM_ERROR_NONE)
					{
						boolOne_Time_Msg_Delete_Flag = true;
						initialized = true;
					}
					else
					{
						boolOne_Time_Msg_Delete_Flag = false;
					}
				}
				else
				{
					if (currentStatus == 'N' && currentCallStatus == 'N')
					{
						////Update network
						if (xTaskGetTickCount() - network_update_time>= (1*30*1000))
						{
							network_update_time = xTaskGetTickCount();
							Signal_Strength = gsm_getsignalstrength();
						}
						
						setObtainEvent();
						if (eventStaged || m2mEventStaged)
						{
							operateOnStagedEvent();
						}
					}
					else if (currentStatus == 'I' || currentStatus == 'R')
					{
						if (callTimerExpire())
						{
							char t1 = actionType;
							endCall();
							checkRespSMS(t1);
						}
						if (factory_settings_parameter_struct.ENABLE_M2M)
						{
							if(m2mEvent && callAccepted)
							{
								if(!keyPressed &&  xTaskGetTickCount() - callCutWait > 2000)
								{
									keyPressed=true;
									sendDTMFTone(m2mEventNo);
								}
							}
						}
						if (playSoundElligible())
						{
							triggerPlaySound();
						}
					}
					//////////////////////////////////////////////////////////////////////////
					char response[64] = {0};
					if (gsm_read_response_line(response,sizeof(response)))
					{
						uint8_t sms_index;
						sms_index = gsm_responseLine_isNew_SMS_Received(response);
						if (sms_index>0)
						{
							char phone_number[15];
							char Received_SMS[160];
							gsm_read_sms(sms_index,phone_number,15,Received_SMS,160);
							bool admin = isAdmin(phone_number);
							bool primaryUser = isPrimaryNumber(phone_number);
							bool alterUsr = isAlterNumber(phone_number);
							bool response_sms_processed_cmd = true;
							
							StringtoUpperCase(Received_SMS);
							
							//char passCode[19];
							//sprintf(passCode,"~%010lu%6lu",factory_settings_parameter_struct.DeviceID_ee,factory_settings_parameter_struct.dateCode);		//generate pass Phrase
							//
							//if(strstr(Received_SMS,passCode))							//check passCode exists
							//{
							//memmove(Received_SMS,Received_SMS+17,strlen(Received_SMS));		//discard passPhrase
							//admin = true;													//set admin as true as passCode matches
							//}

							if (admin || primaryUser || alterUsr)
							{
								if(StringstartsWith(Received_SMS,"#"))
								{
									memmove(Received_SMS, Received_SMS+1, strlen(Received_SMS));  //this will remove '#'
									response_sms_processed_cmd=false;
								}
								processOnSMS(Received_SMS,admin,response_sms_processed_cmd,alterUsr,phone_number);
							}
							else if(isM2MNumber(phone_number))
							{
								if(StringstartsWith(Received_SMS,"VMM01"))
								{
									setM2MVerify(true);
									getM2MNumber(phone_number);
									gsm_send_sms(phone_number,"VMR02");
								}
							}
							else if(factory_settings_parameter_struct.ENABLE_M2M && isM2MRemoteNumber(phone_number))
							{
								if(StringstartsWith(Received_SMS,"VMR02"))
								{
									setM2MRemoteVerified(true);
									saveM2MSettings(true);
									getActiveNumber(phone_number);
									gsm_send_sms(phone_number,"M2M TURNED ON");
								}
							}
							gsm_delete_all_sms();
						}


						if (!freezeIncomingCalls &&  (currentStatus == 'N' || currentStatus == 'R') && (currentCallStatus == 'N' || currentCallStatus == 'I')) //Ringing Incoming Call
						{
							if (gsm_responseLine_isRinging(response))
							{
								currentStatus = 'R';
								currentCallStatus = 'I';
								char incoming_caller[20]={0};
								inCall = true;
								
								bool new_call = false;
								for (uint8_t i=0;i<4;i++)
								{
									vTaskDelay(500/portTICK_PERIOD_MS);
									gsm_read_response_line(response,sizeof(response));
									if (gsm_responseLine_get_IncommingCallNo(response,incoming_caller))
									{
										new_call = true;
										callCutWait = xTaskGetTickCount();
										break;
									}
								}
								if (new_call)
								{
									new_call = false;
									if (checkNumber(incoming_caller))
									{
										acceptCall();
									}
									else
									{
										endCall();
									}
								}
							}
							else if (gsm_responseLine_isCallCut(response))
							{
								endCall();
							}
						}
						else if (!freezeIncomingCalls && currentStatus == 'I' && currentCallStatus == 'I') //IN CALL INCOMING CALL
						{
							if (gsm_responseLine_isCallCut(response))
							{
								endCall();
							}
							else if(gsm_responseLine_isNew_DTMF_Command_Received(response))
							{
								char *dtmf_tocken;
								dtmf_tocken = strtok(response,": ");
								dtmf_tocken = strtok(NULL,": ");
								RemoveSpaces(dtmf_tocken);
								processOnDTMF(dtmf_tocken);
							}
							else
							{
								playSoundAgain(response);
							}
						}
						else if ((currentStatus == 'N' || currentStatus == 'R') && currentCallStatus == 'O') // OUTGOING CALL
						{
							if (OutGoingcallState(response) == 'R')
							{
								callCutWait = xTaskGetTickCount();
								currentStatus = 'R';
								currentCallStatus = 'O';
							}
							else if (gsm_responseLine_isCallCut(response) || OutGoingcallState(response) == 'E') //
							{
								char t1 = actionType;
								endCall();
								checkRespSMS(t1);
							}
							else if (OutGoingcallState(response) == 'I')
							{
								callCutWait = xTaskGetTickCount();
								currentStatus = 'I';
								currentCallStatus = 'O';
								callAccepted = true;
								if (!m2mEvent)
								{
									playSound(actionType,true);
								}
							}
						}
						else if (currentStatus == 'I' && currentCallStatus == 'O') //IN CALL OUTGOING CALL
						{
							if (gsm_responseLine_isCallCut(response) || OutGoingcallState(response) == 'E')
							{
								endCall();
							}
							else if (gsm_responseLine_isNew_DTMF_Command_Received(response))
							{
								char *dtmf_tocken;
								dtmf_tocken = strtok(response,": ");
								dtmf_tocken = strtok(NULL,": ");
								RemoveSpaces(dtmf_tocken);
								processOnDTMF(dtmf_tocken);
							}
							else
							{
								playSoundAgain(response);
							}
						}
					}
				}
			}
		}
		else
		{
			boolGsm_config_flag			=false;
			boolOne_Time_Msg_Delete_Flag   =false;
			
			GSM_PWR_ON;
			vTaskDelay(3000);
			GSM_PWR_OFF;
			vTaskDelay(5000);
		}
	}
}
void start_gsm_service(void)
{
	xTaskCreate(vTask_GSM_service,NULL,(uint16_t)900,NULL,1,NULL);
}

bool busy(void)
{
	return (inCall /*|| inInterrupt*/);
}

bool checkNotInCall(void)
{
	return ( /*!sendCUSDResponse     &&*/  currentStatus=='N'
	&&  currentCallStatus=='N'  &&  obtainNewEvent
	&&  !freezeIncomingCalls && !busy());
}

bool gsm_checkSleepElligible(void)
{
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		return(!commandsAccepted  && checkNotInCall() && !m2mEventStaged && !eventStaged);
	}
	else
	{
		return(!commandsAccepted  && checkNotInCall() && !eventStaged);
	}
}
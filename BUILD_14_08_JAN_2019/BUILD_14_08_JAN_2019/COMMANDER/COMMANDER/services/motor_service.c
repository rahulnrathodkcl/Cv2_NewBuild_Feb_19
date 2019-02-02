#include "motor_service.h"

static SemaphoreHandle_t xADC_Semaphore=NULL;
static void vTask_MOTORCONTROL(void *params);
static void button_detect_pin_callback(void);

static void motor_feedback_callback(void);

static void vTask_10ms_Timer(void *params);
static void vTask_100ms_Timer(void *params);

static void Water_Level_Task(void *params);

volatile unsigned char ucharPhase_Seq_Check_Flag=1;//0=not check, 1=Check
volatile unsigned char ucharPhase_Seq_Err_Flag=0;//0=undefined, 1=error, 2=ok
volatile unsigned char ucharPhase_Seq_Err_Counter=0;
volatile unsigned char ucharPhase_Seq_Timer_Counter=0;
volatile unsigned char ucharVoltage_Detect_Timer_Counter=1;		//to set timer for new Voltage Detection
volatile unsigned char ucharCurrent_Detect_Flag=0;

unsigned char ucharPhase_1_Timer_Counter=0;

struct ac_module ac_instance;
struct rtc_module rtc_instance;
struct events_resource resource;


/*
simEventTemp[0] 		: _motor not started							N
simEventTemp[1] 		: _cannot turn off motor due to some problem	P
simEventTemp[2]		: _motor has turned off due to unknown reason.	U
simEventTemp[3]		: _single phasing has occured so motor off		F
simEventTemp[4]		: _AC power On									G
simEventTemp[5]		: _AC power off									L
simEventTemp[6]		: _motor has turned off due to power cut off.	C
simEventTemp[7]		: _motor has started  							S
simEventTemp[8]		: _motor has turned off							O
simEventTemp[9]		: lost AC power in 1 phase						A
*/


void readOverHeadWaterSensorState(bool *olow,bool *ohigh)
{
	*olow = port_pin_get_input_level(OVERHEAD_TANK_LL_PIN);
	//*omid = port_pin_get_input_level(OVERHEAD_TANK_ML_PIN);
	*ohigh = port_pin_get_input_level(OVERHEAD_TANK_HL_PIN);
}

void updateOverheadLevel(uint8_t level)
{
	overheadLevel=level;
}

uint8_t getOverHeadWaterSensorState(void)
{
	bool olow,omid,ohigh;
	readOverHeadWaterSensorState(&olow,&ohigh);
	uint8_t ans=0;
	if(!olow)
	{
		ans++;
		//if(!omid)
		{
			//ans++;
			if (!ohigh)
			{
				ans++;
			}
		}
	}
	return ans;
}

void overHeadWaterStatusOnCall(bool current)
{
	uint8_t temp = getOverHeadWaterSensorState();
	if(current)
	temp = overheadLevel;
	if(temp == OVERHEADHIGHLEVEL)
	{
		setMotorMGRResponse('V');
	}
	else if(temp == OVERHEADMIDLEVEL)
	{
		setMotorMGRResponse('X');
	}
	else if (temp == OVERHEADCRITICALLEVEL)
	{
		setMotorMGRResponse('W');
	}
}

void readWaterSensorState(bool *low,bool *mid,bool *high)
{
	*low = port_pin_get_input_level(UNDERGRUND_TANK_LL_PIN);
	*mid = port_pin_get_input_level(UNDERGRUND_TANK_ML_PIN);
	*high = port_pin_get_input_level(UNDERGRUND_TANK_HL_PIN);
}

void updateUndergroundLevel(uint8_t level)
{
	undergroundLevel=level;
}

uint8_t getWaterSensorState(void)
{
	bool l,m,h;
	readWaterSensorState(&l,&m,&h);
	uint8_t ans=0;

	if(!l)
	{
		ans++;
		if(!m)
		{
			ans++;
			if(!h)
			{
				ans++;
			}
		}
	}
	return ans;
}

void waterStatusOnCall(bool current)
{
	uint8_t temp = getWaterSensorState();
	if(current)
	temp = undergroundLevel;

	if(temp==CRITICALLEVEL)
	{
		setMotorMGRResponse('T');	//water level insufficient
	}
	else if(temp==LOWLEVEL)
	{
		setMotorMGRResponse('Q');	//water below 2nd sensor
	}
	else if(temp==MIDLEVEL)
	{
		setMotorMGRResponse('R');	//water below 1st sensor
	}
	else if(temp==HIGHLEVEL)
	{
		setMotorMGRResponse('E');	//well is full
	}
}

static void Water_Level_Task(void *params)
{
	UNUSED(params);
	
	struct port_config water_level_sensor_pin_config;
	port_get_config_defaults(&water_level_sensor_pin_config);
	
	water_level_sensor_pin_config.direction = PORT_PIN_DIR_INPUT;
	water_level_sensor_pin_config.input_pull = PORT_PIN_PULL_UP;
	
	port_pin_set_config(OVERHEAD_TANK_HL_PIN,	&water_level_sensor_pin_config);
	port_pin_set_config(OVERHEAD_TANK_ML_PIN,	&water_level_sensor_pin_config);
	port_pin_set_config(OVERHEAD_TANK_LL_PIN,	&water_level_sensor_pin_config);
	port_pin_set_config(UNDERGRUND_TANK_HL_PIN, &water_level_sensor_pin_config);
	port_pin_set_config(UNDERGRUND_TANK_ML_PIN, &water_level_sensor_pin_config);
	port_pin_set_config(UNDERGRUND_TANK_LL_PIN, &water_level_sensor_pin_config);
	
	undergroundLevel = MIDLEVEL;
	tempUndergroundLevel = MIDLEVEL;
	
	overheadLevel = OVERHEADMIDLEVEL;
	tempOverheadLevel = OVERHEADMIDLEVEL;
	tempWaterEventCount = 0;
	
	uint8_t j = 0;
	if (factory_settings_parameter_struct.ENABLE_GP)
	{
		j = 19;
	}
	else
	{
		j= 17;
	}
	for (uint8_t i=12;i<j;i++)
	{
		simEventTemp[i] = true;
	}
	simEvent[12] = 'I';
	simEvent[13] = 'D';
	simEvent[14] = 'H';
	simEvent[15] = 'E';
	simEvent[16] = 'Z';
	if (factory_settings_parameter_struct.ENABLE_GP)
	{
		simEvent[17] = 'V';
		simEvent[18] = 'W';
	}
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		m2mEvent_arr[0] = ME_CLEARED;
		m2mEvent_arr[1] = ME_CLEARED;
		
		mapTable[0] = 13;
		mapTable[1] = 15;
	}
	
	
	for (;;)
	{
		bool result=false;
		
		if (!(user_settings_parameter_struct.waterBypassAddress))
		{
			uint8_t uLevel;
			uint8_t oLevel;
			uLevel = getWaterSensorState();
			
			if (factory_settings_parameter_struct.ENABLE_GP)
			{
				oLevel =getOverHeadWaterSensorState();
			}
			
			if ((factory_settings_parameter_struct.ENABLE_GP == true)?
			((uLevel!=undergroundLevel && uLevel==tempUndergroundLevel) ||(oLevel!=overheadLevel && oLevel==tempOverheadLevel)):
			((uLevel!=undergroundLevel && uLevel==tempUndergroundLevel))
			)
			{
				tempWaterEventCount++;
				if(tempWaterEventCount>9)
				{
					operateOnWaterEvent();
					result=true;
				}
			}
			else
			{
				tempWaterEventCount=0;
			}
			tempUndergroundLevel=uLevel;
			if (factory_settings_parameter_struct.ENABLE_GP)
			{
				tempOverheadLevel=oLevel;
			}
		}
		else
		{
			vTaskDelay(5000/portTICK_PERIOD_MS);
		}
		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

void operateOnWaterEvent(void)
{
	uint8_t uLevel = tempUndergroundLevel;
	uint8_t oLevel = tempOverheadLevel;
	if(uLevel == undergroundLevel)
	{
		if (factory_settings_parameter_struct.ENABLE_GP)
		{
			return;
		}
		else
		{
			if(oLevel==overheadLevel)
			{
				return;
			}
		}
	}
	if (getMotorState())
	{
		if ((uLevel == CRITICALLEVEL) && (undergroundLevel>CRITICALLEVEL))	//no water in well
		{
			stopMotor(false,true,false);
			simEventTemp[12] = registerEvent('I');	//report To SIM Motor Off due to insufficient water level
			if (factory_settings_parameter_struct.ENABLE_M2M)
			{
				if ((m2m_Numbers_struct.m2mSetting) && (undergroundLevel>LOWLEVEL))
				{
					m2mEvent_arr[0] = ME_WAITREGISTER;
				}
			}
		}
		else if (uLevel==HIGHLEVEL && undergroundLevel<HIGHLEVEL)
		{
			if (factory_settings_parameter_struct.ENABLE_M2M)
			{
				if (m2m_Numbers_struct.m2mSetting)
				{
					m2mEvent_arr[1] = ME_WAITREGISTER;
				}
				else
				{
					simEventTemp[15] = registerEvent('E');	//report To SIM well is full.
				}
				if (user_settings_parameter_struct.preventOverFlowAddress)
				{
					stopMotor(false,true,false);
					simEventTemp[14] = registerEvent('H'); //report To SIM well is full, so stopped motor
				}
				else
				{
					simEventTemp[15] = registerEvent('E'); //report To SIM well is full.
				}
			}
		}
		else if ((uLevel==LOWLEVEL) && (undergroundLevel>LOWLEVEL))		// decrease in water level
		{
			if (factory_settings_parameter_struct.ENABLE_M2M)
			{
				if (m2m_Numbers_struct.m2mSetting)
				{
					m2mEvent_arr[0] = ME_WAITREGISTER;
				}
			}
			else
			{
				if (factory_settings_parameter_struct.NOLEVELCHANGECALL)
				{
					simEventTemp[13] = registerEvent('D'); //report To SIM water level is decrease..
				}
			}
		}
		else if (uLevel==MIDLEVEL && undergroundLevel<MIDLEVEL)		// increase in water level
		{
			simEventTemp[16] = registerEvent('Z'); //report To SIM water level is increasing..
		}
		if (factory_settings_parameter_struct.ENABLE_GP)
		{
			if(oLevel!=overheadLevel)
			{
				if(oLevel==OVERHEADHIGHLEVEL && overheadLevel<OVERHEADHIGHLEVEL)		////overhead tank is full
				{
					stopMotor(false,true,false);
					simEventTemp[17] = registerEvent('V'); //report To SIM Motor Off due to overhead tank full
				}
				else if (oLevel==OVERHEADCRITICALLEVEL && overheadLevel>OVERHEADCRITICALLEVEL)	// overhead tank empty.
				{
					simEventTemp[18] = registerEvent('W'); //report To SIM , overhead tank empty.
				}
			}
		}
	}
	else
	{
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			if(m2m_Numbers_struct.m2mSetting && uLevel<MIDLEVEL && undergroundLevel>=MIDLEVEL)	//level is decreased
			{
				m2mEvent_arr[0] = ME_WAITREGISTER;
			}
			else if (uLevel==HIGHLEVEL && undergroundLevel<HIGHLEVEL)			//well is full
			{
				if(m2m_Numbers_struct.m2mSetting)
				{
					m2mEvent_arr[1] = ME_WAITREGISTER;
				}
				else
				{
					simEventTemp[15] = registerEvent('E'); //report To SIM well is full.
				}
			}
		}
		else if(uLevel==HIGHLEVEL && undergroundLevel<HIGHLEVEL)		//well is full
		{
			simEventTemp[15] = registerEvent('E'); //report To SIM well is full.
		}
		else if (uLevel==MIDLEVEL && undergroundLevel<MIDLEVEL)		// underground level is increasing
		{
			if (factory_settings_parameter_struct.ENABLE_GP)
			{
				if (factory_settings_parameter_struct.DUAL_LEVEL)
				{
					if(oLevel<OVERHEADHIGHLEVEL)
					{
						if(user_settings_parameter_struct.autoStartAddress)			//autoStart is ON
						{
							triggerAutoStart();
						}
					}
				}
			}
			if (factory_settings_parameter_struct.DUAL_LEVEL)
			{
				if(user_settings_parameter_struct.autoStartAddress)			//autoStart is ON
				{
					triggerAutoStart();
				}
			}
		}
		if (factory_settings_parameter_struct.ENABLE_GP)
		{
			if (oLevel==OVERHEADCRITICALLEVEL && overheadLevel>OVERHEADCRITICALLEVEL && uLevel>CRITICALLEVEL) // overhead tank is empty, and underground not low
			{
				if(user_settings_parameter_struct.autoStartAddress)		//autoStart is ON
				{
					triggerAutoStart();
				}
				else
				{
					simEventTemp[18] = registerEvent('W'); //report To SIM overhead tank is empty.
				}
			}
		}
	}
	if (factory_settings_parameter_struct.ENABLE_GP)
	{
		updateOverheadLevel(oLevel);
	}
	updateUndergroundLevel(uLevel);
}

void Configure_ADC0(void)
{
	struct adc_config config;
	adc_get_config_defaults(&config);
	config.positive_input = ADC_POSITIVE_INPUT_PIN19;
	config.negative_input = ADC_NEGATIVE_INPUT_GND;
	config.reference      = ADC_REFERENCE_AREFA;//ADC_REFERENCE_INT1V;
	config.clock_source   = GCLK_GENERATOR_3;
	config.gain_factor    = ADC_GAIN_FACTOR_1X;
	config.resolution	= ADC_RESOLUTION_12BIT;
	
	config.clock_prescaler = ADC_CLOCK_PRESCALER_DIV64; //125kHz adc clock (8MHz/64)
	
	config.run_in_standby = true;
	
	adc_init(&adc_inst, ADC, &config);// Initialize the ADC
	adc_enable(&adc_inst);
}

uint32_t Read_ADC0(uint32_t adc_pin,uint16_t samples)
{
	adc_set_positive_input(&adc_inst, adc_pin);
	uint16_t current_value = 0;
	uint32_t total_value = 0;
	for (uint16_t uintLoop=0;uintLoop<samples;uintLoop++)
	{
		current_value = 0;
		adc_start_conversion(&adc_inst);
		while((adc_get_status(&adc_inst) & ADC_STATUS_RESULT_READY) != 1);
		adc_read(&adc_inst, &current_value);
		total_value+=current_value;
	}
	
	return (total_value/samples);
}


uint32_t Read_Voltage_ADC0(uint32_t adc_pin)
{
	adc_set_positive_input(&adc_inst, adc_pin);
	//read 500 samples
	uint16_t no_of_samples = 500;
	uint16_t samples_buffer[no_of_samples];
	for (uint16_t i=0;i<no_of_samples;i++)
	{
		adc_start_conversion(&adc_inst);
		while (adc_read(&adc_inst, &samples_buffer[i]) != STATUS_OK) {
		}
	}
	//arrange decending order
	uint16_t a,b,c;
	
	for (b = 0; b < no_of_samples; ++b)
	{
		for (c = b + 1; c < no_of_samples; ++c)
		{
			if (samples_buffer[b] < samples_buffer[c])
			{
				a = samples_buffer[b];
				samples_buffer[b] = samples_buffer[c];
				samples_buffer[c] = a;
			}
		}
	}
	
	return samples_buffer[5]; //0,1,2,3,4 are considered as voltage spikes
}

void autoSetCurrent(void)
{
	if(getMotorState() && !startSequenceOn && !starDeltaTimerOn && !stopSequenceOn && getAllPhaseState())
	{
		uint32_t ADCcurrent = Analog_Parameter_Struct.Motor_Current;
		//
		//if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
		//{
		//ADCcurrent = Read_ADC0(ADC_POSITIVE_INPUT_PIN16,200);
		//xSemaphoreGive(xADC_Semaphore);
		//}
		
		
		if(ADCcurrent<250)     //ADC VALUE FOR 2.5A
		{
			setCurrentDetection(false);
			setMotorMGRResponse('Y');		//ampere cleared
			return;
		}
		
		uint32_t tempUnder = ADCcurrent  * user_settings_parameter_struct.underloadPerAddress / 100;
		uint32_t tempOver = ADCcurrent  * user_settings_parameter_struct.overloadPerAddress / 100;

		setNormalLoadValue(ADCcurrent);
		setUnderloadValue(tempUnder);
		setOverloadValue(tempOver);
		setCurrentDetection(true);
		setMotorMGRResponse('K');		//ampere settings complete
	}
	else
	{
		setCurrentDetection(false);
		setMotorMGRResponse('Y');		//ampere cleared
	}
}

void speakAmpere(void)
{
	if(getMotorState())
	{
		char cTemp[8];
		
		uint32_t ADCcurrent = Analog_Parameter_Struct.Motor_Current_IntPart;
		
		//if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
		//{
		//
		//ADCcurrent = Read_ADC0(ADC_POSITIVE_INPUT_PIN16,200);
		//xSemaphoreGive(xADC_Semaphore);
		//ADCcurrent = (ADCcurrent*7225)/100000;
		//xSemaphoreGive(xADC_Semaphore);
		//}
		
		utoa(ADCcurrent, cTemp, 10);
		playRepeatedFiles(cTemp);
		return;
	}
	setMotorMGRResponse('-');
}

void PR2_ISR(void)
{
	if (ucharPhase_Seq_Check_Flag==1)
	{
		ucharPhase_1_Timer_Counter=0;
		delay_ms(5);
		volatile unsigned char ucharHigh_Flag=0,ucharLow_Flag=0,ucharHigh_To_Low_Flag=0,ucharLow_To_High_Flag=0;
		if (port_pin_get_input_level(PR2_PIN)==HIGH)
		{
			for (unsigned int uintLoop=0;uintLoop<120;uintLoop++)
			{
				delay_us(25);
				if ((port_pin_get_input_level(PR1_PIN)==HIGH)&&(ucharHigh_Flag==0))
				{
					ucharHigh_Flag=1;
					if (ucharLow_Flag==1)
					{
						ucharLow_To_High_Flag=1;
						break;
					}
				}
				if ((port_pin_get_input_level(PR1_PIN)==LOW)&&(ucharLow_Flag==0))
				{
					ucharLow_Flag=1;
					if (ucharHigh_Flag==1)
					{
						ucharHigh_To_Low_Flag=1;
					}
				}
				if (ucharHigh_To_Low_Flag==1)
				{
					// LCD takes data from structThreePhase_state, which should have latest sequence data.
					structThreePhase_state.u8t_phase_sequence_flag = THREEPHASE_OK;
					ucharPhase_Seq_Err_Flag=2;//0=undefined, 1=error, 2=ok
					ucharPhase_Seq_Err_Counter=0;
					ucharPhase_Seq_Check_Flag=0;
					break;
				}
				if ((port_pin_get_input_level(PR2_PIN)==LOW))
				{
					break;
				}
			}
		}
		if (++ucharPhase_Seq_Err_Counter>2)
		{
			ucharPhase_Seq_Err_Counter=0;
			structThreePhase_state.u8t_phase_sequence_flag = THREEPHASE_ERROR;
			ucharPhase_Seq_Err_Flag=1;//0=undefined, 1=error, 2=ok
			ucharPhase_Seq_Check_Flag=0;
		}
	}
}

static void vTask_10ms_Timer(void *params)
{
	TickType_t xLastExecutionGsm_Send_Time;
	xLastExecutionGsm_Send_Time = xTaskGetTickCount();
	//--------------------------------
	for( ;; )
	{
		vTaskDelayUntil(&xLastExecutionGsm_Send_Time, (10/portTICK_PERIOD_MS));
		if(ucharPhase_Seq_Check_Flag==1)
		{
			if(++ucharPhase_1_Timer_Counter>=20)
			{
				ucharPhase_1_Timer_Counter=0;
				structThreePhase_state.u8t_phase_sequence_flag = THREEPHASE_ERROR;
				ucharPhase_Seq_Err_Flag=1;//0=undefined, 1=error, 2=ok
				ucharPhase_Seq_Check_Flag=0;
			}
		}
	}
}

static void vTask_100ms_Timer(void *params)
{
	TickType_t xLastExecutionGsm_Send_Time;
	xLastExecutionGsm_Send_Time = xTaskGetTickCount();
	//--------------------------------
	for( ;; )
	{
		vTaskDelayUntil(&xLastExecutionGsm_Send_Time, (100/portTICK_PERIOD_MS));
		
		////////
		//Voltaqe Detect Timer Counter, Reset every 500ms to 0, which triggers the New Voltage Reading Acquisition from ADC.
		if(ucharVoltage_Detect_Timer_Counter++>4)
		{
			ucharVoltage_Detect_Timer_Counter=0;
			//Add Flag to enable detection of current, as the Voltage Detect Timer Counter won't be 0, as voltage reading takes 100ms Time.
			ucharCurrent_Detect_Flag=1;
			
		}
		/////////
		if (ucharPhase_Seq_Timer_Counter++>4)
		{
			ucharPhase_Seq_Timer_Counter=0;
			ucharPhase_Seq_Check_Flag=1;//0=not check, 1=Check
			ucharPhase_1_Timer_Counter=0;
			extint_chan_clear_detected(11);
		}
	}
}

//Function to check if new Voltage reading should be acquired from the ADC, by checking the timer Variable for reading Voltage with 0.
bool should_Detect_New_Voltage(void) {
	return (ucharVoltage_Detect_Timer_Counter == 0);
}

void detect_battery_voltage_and_percentage(void)
{
	if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
	{
		uint32_t bat_v = Read_ADC0(ADC_POSITIVE_INPUT_PIN7,200);
		Analog_Parameter_Struct.Battery_Voltage = (bat_v * 1457)/1000;
		uint8_t bat_per = 0;
		
		if (Analog_Parameter_Struct.Battery_Voltage <= 3300)
		{
			bat_per = 0;
		}
		else
		{
			bat_per = ((((float)Analog_Parameter_Struct.Battery_Voltage/1000)-3.3)*100)/0.9;
		}
		
		Analog_Parameter_Struct.Battery_percentage = bat_per;
		
		xSemaphoreGive(xADC_Semaphore);
	}
}

//Function to save the 3 phase voltage from ADC in to the structure, ADC values are filtered, and multiplied by factor here.
void detect_Three_Phase_Voltage(void) {
	
	if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
	{
		//int32_t adcRY = Read_ADC0(ADC_POSITIVE_INPUT_PIN19,2000);
		int32_t adcRY = Read_Voltage_ADC0(ADC_POSITIVE_INPUT_PIN19);
		adcRY = (adcRY-10);
		if (adcRY<0)
		{
			adcRY = 0;
		}
		else
		{
			adcRY = (((adcRY-10)*330)/1000);
			if (adcRY<0)
			{
				adcRY = 0;
			}
		}
		
		//int32_t adcYB = Read_ADC0(ADC_POSITIVE_INPUT_PIN18,2000);
		int32_t adcYB = Read_Voltage_ADC0(ADC_POSITIVE_INPUT_PIN18);
		adcYB = (adcYB-10);
		if (adcYB<0)
		{
			adcYB = 0;
		}
		else
		{
			adcYB = (((adcYB-10)*320)/1000);
			if (adcYB<0)
			{
				adcYB = 0;
			}
		}
		//int32_t adcBR =  Read_ADC0(ADC_POSITIVE_INPUT_PIN17,2000);
		int32_t adcBR = Read_Voltage_ADC0(ADC_POSITIVE_INPUT_PIN17);
		adcBR = (adcBR-12);
		if (adcBR<0)
		{
			adcBR = 0;
		}
		else
		{
			adcBR = (((adcBR-12)*325)/1000);
			if (adcBR<0)
			{
				adcBR = 0;
			}
		}
		
		
		Analog_Parameter_Struct.PhaseRY_Voltage = adcRY;
		Analog_Parameter_Struct.PhaseYB_Voltage = adcYB;
		Analog_Parameter_Struct.PhaseBR_Voltage = adcBR;
		
		set_Three_Phase_State_From_Voltage();
		xSemaphoreGive(xADC_Semaphore);
	}
}

//Function to set the Three Phase State from acquired voltage
void set_Three_Phase_State_From_Voltage(void) {
	
	uint8_t temp_phase_state = structThreePhase_state.u8t_phase_ac_state;		//save last AC Phase State, in case AC Phase State is going to change
	
	
	if ((Analog_Parameter_Struct.PhaseRY_Voltage < 40) &&
	(Analog_Parameter_Struct.PhaseYB_Voltage < 40) &&
	(Analog_Parameter_Struct.PhaseBR_Voltage < 40))				// if All phase volt, less than 40
	{
		structThreePhase_state.u8t_phase_ac_state = AC_OFF; //no phase is present, light is cut off
	}
	else if((abs(Analog_Parameter_Struct.PhaseRY_Voltage-Analog_Parameter_Struct.PhaseYB_Voltage)>user_settings_parameter_struct.singlePhasingVoltage) ||
	(abs(Analog_Parameter_Struct.PhaseYB_Voltage-Analog_Parameter_Struct.PhaseBR_Voltage)>user_settings_parameter_struct.singlePhasingVoltage) ||
	(abs(Analog_Parameter_Struct.PhaseBR_Voltage-Analog_Parameter_Struct.PhaseRY_Voltage)>user_settings_parameter_struct.singlePhasingVoltage))  // if diff betweeen any 2 phases > 80
	{
		structThreePhase_state.u8t_phase_ac_state = AC_2PH;//Single phasing Occured
	}
	else  //all Phase are present
	{
		structThreePhase_state.u8t_phase_ac_state = AC_3PH;
	}
	
	if (current_three_phase_state != structThreePhase_state.u8t_phase_ac_state)
	{
		last_three_phase_state  = 	temp_phase_state;								//assign saved temp AC Phase State to last_three_phase_state
		current_three_phase_state = structThreePhase_state.u8t_phase_ac_state;
		eventOccured = true;
	}
}

//Function to detect the Motor Current, From ADC, Average it (using ADC_0) , and store it in the Analog_Parameter_Struct
void detect_Motor_Current(void){
	if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
	{
		uint32_t ADCcurrent = Read_ADC0(ADC_POSITIVE_INPUT_PIN16,200);
		
		
		if(ADCcurrent>15)
		{
			ADCcurrent = abs(ADCcurrent - 15);
		}
		else if(ADCcurrent <= 15)
		{
			ADCcurrent = 0;
		}
		
		xSemaphoreGive(xADC_Semaphore);
		Analog_Parameter_Struct.Motor_Current_ADC_Value = ADCcurrent;				// does ADCcurrent here have ADC Value of Current ?
		//ADCcurrent = (ADCcurrent*7225)/1000;
		ADCcurrent = (ADCcurrent*3425)/1000;
		if(ADCcurrent<1200 && ADCcurrent!=0)
		{
			ADCcurrent = ADCcurrent + (((1200-ADCcurrent)*272)/1000);
		}
		Analog_Parameter_Struct.Motor_Current = ADCcurrent;
		Analog_Parameter_Struct.Motor_Current_IntPart = ADCcurrent/100;
		Analog_Parameter_Struct.Motor_Current_DecPart = ADCcurrent%100;
		ucharCurrent_Detect_Flag = 0;												//reset the flag, to disable current reading for next 500ms
	}
}

//Function to check if the New Current Reading should be read
bool should_Detect_New_Current(void){
	
	//todo : add all the conditions checks i.e. motor ON, current consumption ON etc. , and should get new reading every 500ms
	
	return (ucharCurrent_Detect_Flag == 1);
	
	//return (should_Detect_New_Voltage());
}


bool getACPowerState(void)
{
	//return false;
	return phaseAC;
}

void setACPowerState(bool state)
{
	phaseAC = state;
}

uint8_t getAllPhaseState(void)
{
	return allPhase;
	//if ((bool)user_settings_parameter_struct.bypassAddress)
	//{
		//if(!allPhase || getACPowerState())
		//{
			//return true;
		//}
		//else
		//{
			//return false;
		//}
	//}
	//else
	//{
		//return allPhase;
	//}
}

void setAllPhaseState(uint8_t state)
{
	allPhase = state;
}

bool getPhaseSequence()
{
	return vBoolPhaseSeq;
}

void setPhaseSequence(bool phaseSequence)
{
	vBoolPhaseSeq=phaseSequence;
}


bool getMotorState(void)
{
	return mFeedback;
}

void setMotorState(bool state)
{
	mFeedback = state;
	if (state)
	{
		MOTOR_ON_LED_ON;
	}
	else
	{
		MOTOR_ON_LED_OFF;
	}
}

bool getMotorState_from_pin(void)
{
	uint8_t p1;
	bool p2, p3, p4;
	readSensorState(&p1, &p2, &p3, &p4);
	setMotorState(p3);
	return p3;
}



void readSensorState(uint8_t *allPhase, bool *phaseSeq,bool *motor, bool *acPhase)
{
	*allPhase = structThreePhase_state.u8t_phase_ac_state;
	//*phaseSeq = structThreePhase_state.u8t_phase_sequence_flag;
	if(structThreePhase_state.u8t_phase_sequence_flag == THREEPHASE_OK)
	{
		*phaseSeq=true;
	}
	else
	{
		*phaseSeq = false;
	}
	
	//if ((structThreePhase_state.u8t_phase_ac_state == AC_3PH) && (structThreePhase_state.u8t_phase_sequence_flag == THREEPHASE_OK))
	//{
	//*p1 = true;
	//}
	//else
	//{
	//*p1 = false;
	//}

	*motor  = !(port_pin_get_input_level(PIN_MOTOR_FEEDBACK));
	
	uint8_t last_comparison = AC_CHAN_STATUS_UNKNOWN;
	
	last_comparison = ac_chan_get_status(&ac_instance,AC_CHAN_CHANNEL_0);
	vTaskDelay(500/portTICK_PERIOD_MS);
	last_comparison = ac_chan_get_status(&ac_instance,AC_CHAN_CHANNEL_0); //read again
	
	if (last_comparison & AC_CHAN_STATUS_POS_ABOVE_NEG)
	{
		isACpowerAvailable = true;
	}
	else
	{
		isACpowerAvailable = false;
	}
	
	*acPhase =  isACpowerAvailable;
}

void updateSensorState(uint8_t var3PhaseState, bool var3PhaseSeq, bool motorState, bool acPhaseState)
{
	setAllPhaseState(var3PhaseState); // allPhase = p1;
	setPhaseSequence(var3PhaseSeq);
	setMotorState(motorState); // mFeedback = p2;
	setACPowerState(acPhaseState); // phaseAC = p4;
	
	if(getAllPhaseState()==AC_3PH && getACPowerState())
	{
		bool tempPhaseSequence = true;									// init temp variable with default value as correct sequence

		if(user_settings_parameter_struct.detectPhaseSequence)			// if detection of Phase Sequence is enabled
		{
			tempPhaseSequence = getPhaseSequence();						// save the current phase sequence  in temp varialbe for further use
		}

		if(tempPhaseSequence)											// check if sequence is correct
		{
			THREEPHASE_OK_LED_ON;
		}
		else
		{
			THREEPHASE_OK_LED_OFF;
		}
		
		if((bool)user_settings_parameter_struct.autoStartAddress)
		{
			AUTO_ON_LED_ON;
		}
		else
		{
			AUTO_ON_LED_OFF;
		}
	}
	else
	{
		THREEPHASE_OK_LED_OFF;
		AUTO_ON_LED_OFF;
		startTimerOn=false;
	}
	
	//if (getACPowerState())
	//{
	//if (!(bool)(user_settings_parameter_struct.autoStartAddress))
	//{
	//AUTO_ON_LED_OFF;
	//}
	//else
	//{
	//AUTO_ON_LED_ON;
	//}
	//
	//if(getAllPhaseState()==AC_3PH && getPhaseSequence())
	//{
	//THREEPHASE_OK_LED_ON;
	//}
	//else
	//{
	//THREEPHASE_OK_LED_OFF;
	//}
	//}
	//else
	//{
	//AUTO_ON_LED_OFF;
	//}
	//
	//if (!getACPowerState() || !getAllPhaseState())
	//{
	//startTimerOn = false;
	//}
}

void resetAutoStart(bool setChange)
{
	if (!(bool)user_settings_parameter_struct.autoStartAddress)
	{
		startTimerOn=false;
		AUTO_ON_LED_OFF;
	}
	else if ((bool)user_settings_parameter_struct.autoStartAddress)
	{
		AUTO_ON_LED_ON;
		if (setChange)
		{
			triggerAutoStart();
		}
	}
}

void triggerAutoStart(void)
{
	if (!getMotorState())
	{
		if (getAllPhaseState() && getACPowerState())
		{
			startTimerOn = true;
			tempStartTimer = xTaskGetTickCount();
		}
	}
}

void operateOnEvent(void)
{
	uint8_t t3Phase;
	//bool t3Phase, tMotor, tacPhase;
	bool tPhaseSeq, tMotor, tacPhase;
	readSensorState(&t3Phase, &tPhaseSeq, &tMotor, &tacPhase);
	eventOccured = false;
	
	//todo: add current phase Sequence and previous phase sequence is equals check here
	if ((t3Phase == getAllPhaseState()) && (tMotor == getMotorState()) && (tacPhase == getACPowerState()))
	{
		return;
	}
	if (getMotorState())	//motorOn
	{
		if (t3Phase==AC_OFF && !tMotor && !tacPhase)	//acPower Cut Off
		{
			stopMotor(false,true,false);
			THREEPHASE_OK_LED_OFF;
			simEventTemp[6] = registerEvent('C'); //report To SIM Motor Off due to POWER CUT OFF
		}
		////////////////////////////////////////Unknown Motor Off Check ////////////////////////////////////////
		
		else if ((tacPhase && getACPowerState()) &&																										//AC PHASE PRESENT
		((user_settings_parameter_struct.detectSinglePhasing && t3Phase==AC_3PH && getAllPhaseState()==AC_3PH) ||								//IF SPP ON, 3 phase old and current is present
		(!user_settings_parameter_struct.detectSinglePhasing) && t3Phase>=AC_2PH && getAllPhaseState()>=AC_2PH) &&							//IF SPP OFF, 3 phase old and current is >= 2 phase
		(!tMotor))																																// AND MOTOR HAS TURNED OFF
		{
			unknownMotorOff();
		}
		///////////////////////// SINGLE PHASING CHECK /////////////////////////
		else if (user_settings_parameter_struct.detectSinglePhasing &&																						//SPP IS ON
		t3Phase==AC_2PH &&																														// Only 2 Phase Present
		tacPhase) ////single phasing occured																									// AC Phase is Present
		{
			tempSinglePhasingTimer = xTaskGetTickCount();
			singlePhasingTimerOn = true;
		}
	}
	else
	{
		if (tMotor)		// motor turn on manually
		{
			if (t3Phase==AC_3PH && tPhaseSeq && tacPhase)
			{
				if (startTimerOn)
				{
					startTimerOn = false;
				}
				THREEPHASE_OK_LED_ON;
				simEventTemp[7] = registerEvent('S');	//register To SIM Motor has started
			}
			else
			{
				stopMotor(false,true,false);
			}
		}
		else
		{
			waitStableLineOn = true;
			waitStableLineTimer = xTaskGetTickCount();
		}
	}
	updateSensorState(t3Phase,tPhaseSeq, tMotor,tacPhase);
}

uint8_t checkLineSensors(void)
{
	return structThreePhase_state.u8t_phase_ac_state;
}

void operateOnStableLine(void)
{
	waitStableLineOn = false;
	uint8_t temp = checkLineSensors();
	if (temp == AC_3PH)
	{
		bool tempPhaseSeq = true;													//Set Temp Phase Seq to True
		if(user_settings_parameter_struct.detectPhaseSequence)						// if Sequence Detection is needed
		{
			tempPhaseSeq = getPhaseSequence();										// set current phase seq to the temp variable
		}
		
		if(tempPhaseSeq)															// if correct phase seq than
		{
			THREEPHASE_OK_LED_ON;
			if (user_settings_parameter_struct.autoStartAddress)
			{
				triggerAutoStart();
			}
			else
			{
				if (user_settings_parameter_struct.dndAddress!=DND_LIGHT)			//DND IS OFF FOR ALL 3 PHASE LIGHT EVENTS, DND IS ON FOR SINGLE PHASING EVENTS
				{
					simEventTemp[4] = registerEvent('G');							//register TO SIM AC power ON
				}
			}
		}
		else																		// If Incorrect Phase Sequence than
		{
			THREEPHASE_OK_LED_OFF;
			//todo: change below statement from registering single Phase Event to NEW INCORRECT PHASE SEQUENCE EVENT
			simEventTemp[9] = registerEvent('A');								//incorrect sequence
		}
	}
	else if (temp == AC_2PH) //Got Power in 2 phase
	{
		THREEPHASE_OK_LED_OFF;
		if (user_settings_parameter_struct.dndAddress == DND_OFF &&					//DND IS OFF FOR ALL KIND OF EVENTS
		!user_settings_parameter_struct.detectSinglePhasing)					//SINGLE PHASING PROTECTION IS ON
		{
			simEventTemp[9] = registerEvent('A'); //register TO SIM 2 phase power ON
		}
	}
	else if (temp == AC_OFF)	//Lost Power in All Phase
	{
		THREEPHASE_OK_LED_OFF;
		if ((user_settings_parameter_struct.dndAddress == DND_OFF) ||												//DND IS OFF FOR ALL EVENTS
		(last_three_phase_state == AC_2PH && user_settings_parameter_struct.dndAddress == DND_OFF) ||			//PREVIOUSLY SINGLE PHASING , and DND OFF FOR ALL EVENTS
		(last_three_phase_state == AC_3PH && user_settings_parameter_struct.dndAddress != DND_LIGHT))			//PREVIOSULY 3 PHASE, and NOT ON FOR ALL EVENTS
		{
			simEventTemp[5] = registerEvent('L'); //register To SIM AC Power OFF
		}
	}
}

bool waitStableLineOver(void)
{
	return (waitStableLineOn && xTaskGetTickCount() - waitStableLineTimer >= (waitStableLineTime * 100));
}



void startMotor(bool commanded)
{
	startTimerOn = false;
	
	if (getACPowerState() &&																														//AC Phase is Presnet
			((getAllPhaseState()==AC_3PH) || (getAllPhaseState()==AC_2PH && !user_settings_parameter_struct.detectSinglePhasing)) &&				//3 phase is present, or SPP is OFF and 2 phase is present
			((user_settings_parameter_struct.detectSinglePhasing && getPhaseSequence()) || (!user_settings_parameter_struct.detectPhaseSequence)))	//Phase Sequnce Protection is ON and correct phase seq, or Phase Seq Protection is off
	{
		if (!getMotorState())
		{
			if (factory_settings_parameter_struct.ENABLE_WATER)
			{
				if(!(user_settings_parameter_struct.waterBypassAddress) && getWaterSensorState()==CRITICALLEVEL)
				{
					if (commanded)
					{
						setMotorMGRResponse('T');	//cannot start motor due to some problem
					}
					else
					{
						simEventTemp[0] = registerEvent('N');//register To SIM motor not started due to ANY REASON
					}
					return;
				}
				
				if (factory_settings_parameter_struct.ENABLE_GP)
				{
					if(!(user_settings_parameter_struct.waterBypassAddress) && getOverHeadWaterSensorState()==OVERHEADHIGHLEVEL)
					{
						if(commanded)
						{
							setMotorMGRResponse('V');	//cannot start motor as OverHead Tank Full.
						}
						else
						{
							simEventTemp[17] = registerEvent('V');//register To SIM motor not started due to ANY REASON
						}
						return;
					}
				}
			}

			STOP_RELAY_ON;
			START_RELAY_ON;
			//MOTOR_ON_LED_ON;
			tempStartSequenceTimer = xTaskGetTickCount();
			startSequenceOn = true;
			setMotorState(true);
			if (factory_settings_parameter_struct.ENABLE_CURRENT)
			{
				enableCurrentBuffer=false;
				lastCurrentReading=CR_NORMAL;
			}
			gotOnCommand = commanded;
		}
		else
		{
			if (commanded)
			{
				setMotorMGRResponse('+');		//motor is already on
			}
		}
	}
	else
	{
		if (commanded)
		{
			setMotorMGRResponse('N');	//cannot start motor due to some problem
		}
		else
		{
			simEventTemp[0] = registerEvent('N');//register To SIM motor not started due to ANY REASON
		}
	}
}

void stopMotor(bool commanded, bool forceStop,bool offButton)
{
	if (forceStop || getMotorState())
	{
		singlePhasingTimerOn = false;
		STOP_RELAY_OFF;
		tempStopSequenceTimer = xTaskGetTickCount();
		stopSequenceOn = true;
		setMotorState(false);
		gotOffCommand = commanded;
		offButtonPressed=offButton;
		if (factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			lastCurrentReading=CR_NORMAL;			//to make the current readings normal
		}
	}
	else
	{
		if (commanded)
		{
			setMotorMGRResponse('-');	//motor is already off
		}
	}
}

bool startMotorTimerOver(void)
{
	return (xTaskGetTickCount() - tempStartTimer >= (((unsigned long int)user_settings_parameter_struct.autoStartTimeAddress * 1000)));
}

void unknownMotorOff(void)
{
	// waitCheckACTimerOn = false;
	//report to SIM Motor Off due to Unknown Reason
	stopMotor(false,true,false);
	simEventTemp[2] = registerEvent('U');
}

bool singlePhasingTimerOver(void)
{
	return (singlePhasingTimerOn && xTaskGetTickCount() - tempSinglePhasingTimer > ((unsigned int)singlePhasingTime * 100));
}

void operateOnSinglePhasing(void)
{
	THREEPHASE_OK_LED_OFF;
	stopMotor(false,true,false);
	simEventTemp[3] = registerEvent('F');
}

void terminateStopRelay(void)
{
	if (stopSequenceOn && xTaskGetTickCount() - tempStopSequenceTimer > (stopSequenceTimerTime * 100))
	{
		//if ((bool)(user_settings_parameter_struct.autoStartAddress) && getACPowerState())
		//{
		//STOP_RELAY_ON;
		//}
		stopSequenceOn = false;
		if (!getMotorState_from_pin())		//motor has turned off
		{
			if (gotOffCommand)
			{
				gotOffCommand = false;
				setMotorMGRResponse('O');		//motor has stopped
			}
			else if(offButtonPressed)
			{
				offButtonPressed=false;
				simEventTemp[8] = registerEvent('O'); //register TO SIM motor has turned off
			}
		}
		else
		{
			if (gotOffCommand)
			{
				gotOffCommand = false;
				setMotorMGRResponse('P');		//cannot turn off motor
			}
			else
			{
				simEventTemp[1] = registerEvent('P');
			}
		}
		offButtonPressed=false;
	}
}

void terminateStarDeltaTimer(void)
{
	if(starDeltaTimerOn && xTaskGetTickCount() - tempStartSequenceTimer > ((unsigned long int)(user_settings_parameter_struct.starDeltaTimerAddress) *1000L))
	{
		START_RELAY_OFF;
		starDeltaTimerOn=false;
		if (factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			enableCurrentBuffer=true;
			tempStartSequenceTimer=xTaskGetTickCount();
		}
	}
}

void terminateStartRelay(void)
{
	if (startSequenceOn &&  xTaskGetTickCount() - tempStartSequenceTimer > (startSequenceTimerTime * 100))
	{
		if(((unsigned int)user_settings_parameter_struct.starDeltaTimerAddress *10) <= startSequenceTimerTime)
		{
			START_RELAY_OFF;
			tempStartSequenceTimer=xTaskGetTickCount();
			if (factory_settings_parameter_struct.ENABLE_CURRENT)
			{
				enableCurrentBuffer=true;
			}
		}
		else
		{
			starDeltaTimerOn=true;
		}
		startSequenceOn = false;
		bool motor = getMotorState_from_pin();
		if (gotOnCommand)
		{
			gotOnCommand = false;
			if (motor)
			{
				setMotorMGRResponse('S'); // motor has started
			}
			else
			{
				stopMotor(false,true,false);
				setMotorMGRResponse('N');	//cannot start motor due to some problem
			}
		}
		else
		{
			if (motor)
			{
				simEventTemp[7] = registerEvent('S');// ;//register To SIM Motor has started
			}
			else
			{
				stopMotor(false,true,false);
				simEventTemp[0] = registerEvent('N');//register To SIM motor not started due to ANY REASON
			}
		}
	}
}

void statusOnCall(void)
{
	char status[3];

	uint8_t b = checkLineSensors();
	if (b == AC_OFF)
	{
		status[0]='L';
		// sim1->setMotorMGRResponse('L');	//motor off, no light
	}
	else if (b == AC_2PH)	//power only in 2 phase
	{
		status[0]='A';
		// sim1->setMotorMGRResponse('A');
	}
	else if (b == AC_3PH)
	{
		bool temp = getMotorState_from_pin();
		if (temp)
		{
			status[0]='+';
			// sim1->setMotorMGRResponse('+');	//motor is on
		}
		else
		{
			status[0]='_';
			// sim1->setMotorMGRResponse('_');	//motor off, light on
		}
	}

	if(user_settings_parameter_struct.autoStartAddress)
	status[1]=')';
	else
	status[1]='[';

	status[2]='\0';
	playRepeatedFiles(status);
}

void setM2MEventState(uint8_t eventNo, uint8_t state)
{
	if(m2mEvent_arr[eventNo]==ME_SERVICING)
	{
		if(state==ME_NOTAVAILABLE)
		{
			state=ME_CLEARED;
			simEventTemp[mapTable[eventNo]]=false;	//regsiter relevant Normal Event
		}
	}
	m2mEvent_arr[eventNo]=state;
}

void M2MEventManager(void)
{
	uint8_t j=2;
	while(j--)
	{
		if(m2mEvent_arr[j]==ME_WAITREGISTER)
		{
			registerM2MEvent(j);
		}
	}
}

void SIMEventManager(void)
{
	uint8_t i = 0;
	if (factory_settings_parameter_struct.ENABLE_WATER)
	{
		if (factory_settings_parameter_struct.ENABLE_GP)
		{
			i = 19;
		}
		else
		{
			i = 17;
		}
	}
	else
	{
		if (factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			i = 14;
		}
		else
		{
			i = 12;
		}
	}
	
	while(i--)
	{
		if (!simEventTemp[i])
		simEventTemp[i] = registerEvent(simEvent[i]);
	}
}


void checkCurrentConsumption(void)
{
	if(startSequenceOn || stopSequenceOn || !getMotorState() || !(user_settings_parameter_struct.currentDetectionAddress) || starDeltaTimerOn)
	{
		return;
	}
	//|| ((xTaskGetTickCount()-lastCurrentReadingTime)<500))
	
	if(enableCurrentBuffer && xTaskGetTickCount()-tempStartSequenceTimer>30000)
	{
		enableCurrentBuffer=false;
	}
	
	//lastCurrentReadingTime=xTaskGetTickCount();
	
	uint32_t ADCcurrent = Analog_Parameter_Struct.Motor_Current;
	
	//if(xSemaphoreTake(xADC_Semaphore,portMAX_DELAY)== pdTRUE)
	//{
	//ADCcurrent = Read_ADC0(ADC_POSITIVE_INPUT_PIN16,200);
	//xSemaphoreGive(xADC_Semaphore);
	//}
	
	uint32_t temp = ADCcurrent;
	
	uint32_t overLoadDetectValue=12000;
	
	uint8_t temp2;
	
	if(enableCurrentBuffer && temp>(user_settings_parameter_struct.normalLoadAddress<<1))    //more than double  <<1 gives mulile of 2 value (double the orignal value)
	{
		temp2 = CR_OVER2;
		overLoadDetectValue=18000;
	}
	else if(!enableCurrentBuffer && temp>(user_settings_parameter_struct.normalLoadAddress<<1))			//more than double
	{
		temp2 = CR_OVER;
		overLoadDetectValue=overLoadDetectValue>>2;
	}
	else if(!enableCurrentBuffer && temp> (user_settings_parameter_struct.normalLoadAddress+(user_settings_parameter_struct.normalLoadAddress>>1))) // more than 1.5
	{
		temp2 = CR_OVER;
		overLoadDetectValue=overLoadDetectValue>>1;
	}
	else if (!enableCurrentBuffer && temp>user_settings_parameter_struct.overloadAddress)		// more than 1.25 to 1.5
	{
		temp2 = CR_OVER;
	}
	else if(temp < user_settings_parameter_struct.underloadAddress && !enableCurrentBuffer)		// only consider noLoad after 30 secs
	{
		temp2 = CR_UNDER;
		overLoadDetectValue=overLoadDetectValue>>2;
	}
	else
	{
		temp2= CR_NORMAL;
	}
	if(lastCurrentReading == temp2)
	{
		if(xTaskGetTickCount()-currentEventFilterTempTime>overLoadDetectValue)
		{
			if(temp2==CR_OVER)
			{
				stopMotor(false,true,false);
				simEventTemp[12] = registerEvent('B');			//register overload Event
			}
			else if(temp2==CR_UNDER)
			{
				stopMotor(false,true,false);
				simEventTemp[13] = registerEvent('J');			// register Underload Event
			}
		}
	}
	else
	{
		currentEventFilterTempTime = xTaskGetTickCount();
		lastCurrentReading=temp2;
	}
}

static void button_detect_pin_callback(void)
{
	buttonEventOccured = true;
}

static void vTask_MOTORCONTROL(void *params)
{
	UNUSED(params);
	
	Configure_ADC0();
	
	configure_ac();
	configure_rtc();
	configure_event();
	
	//////////////////////////////////////////////////////////////////////////
	gotOffCommand = false;
	gotOnCommand = false;

	//////////////////////////////////////////////////////////////////////////
	eventOccured = false;
	
	uint8_t last_comparison = AC_CHAN_STATUS_UNKNOWN;
	
	last_comparison = ac_chan_get_status(&ac_instance,AC_CHAN_CHANNEL_0);
	vTaskDelay(500/portTICK_PERIOD_MS);
	last_comparison = ac_chan_get_status(&ac_instance,AC_CHAN_CHANNEL_0); //read again
	
	if (last_comparison & AC_CHAN_STATUS_POS_ABOVE_NEG)
	{
		isACpowerAvailable = true;
	}
	else
	{
		isACpowerAvailable = false;
	}
	
	current_three_phase_state = AC_OFF;
	//////////////////////////////////////////////////////////////////////////
	
	startTimerOn = false;

	singlePhasingTime = 10;
	singlePhasingTimerOn = false;

	startSequenceTimerTime = 20;
	starDeltaTimerOn=false;
	startSequenceOn = false;

	stopSequenceTimerTime = 20;
	stopSequenceOn = false;
	
	setAllPhaseState(false); // allPhase = false;
	setMotorState(false);// mFeedback = false;
	setACPowerState(false);//  phaseAC = false;
	
	lastPressTime=0;
	lastButtonEvent=0;
	
	uint8_t i= 0;
	if (factory_settings_parameter_struct.ENABLE_CURRENT)
	{
		i=14;
	}
	else
	{
		i = 12;
	}
	while(i--)
	{
		simEventTemp[i] = true;
	}
	simEvent[0] = 'N';
	simEvent[1] = 'P';
	simEvent[2] = 'U';
	simEvent[3] = 'F';
	simEvent[4] = 'G';
	simEvent[5] = 'L';
	simEvent[6] = 'C';
	simEvent[7] = 'S';
	simEvent[8] = 'O';
	simEvent[9] = 'A';

	simEvent[10] = ')';		//AUTO ON EVENT
	simEvent[11] = '[';		//AUTO OFF EVENT
	
	if (factory_settings_parameter_struct.ENABLE_CURRENT)
	{
		simEvent[12] = 'B';		//Overload Event
		simEvent[13] = 'J';		//Underload EVENT
	}
	//////////////////////////////////////////////////////////////////////////
	
	resetAutoStart(true);
	eventOccured=true;
	//////////////////////////////
	for (;;)
	{

		if(!startSequenceOn && !stopSequenceOn)
		{
			uint8_t tempEventOccured=eventOccured;
			uint8_t tempButtonEventOccured=buttonEventOccured;
			
			if(tempEventOccured)
			{
				operateOnEvent();
			}
			if(tempButtonEventOccured)
			{
				operateOnButtonEvent();
			}
			if(lastButtonEvent)
			{
				buttonFilter();
			}
		}
		//// check if it is the time for new Voltage reading and if so than get new Voltage Reading.
		if(should_Detect_New_Voltage()) {
			detect_battery_voltage_and_percentage();
			detect_Three_Phase_Voltage();
		}
		////////
		
		// To check if new reading of motor current is needed, and get new reading, and update in Analog_Parameter_Struct
		if(should_Detect_New_Current()) {
			detect_Motor_Current();
			if (factory_settings_parameter_struct.ENABLE_CURRENT)
			{
				checkCurrentConsumption();
			}
		}
		///////
		if (waitStableLineOn && waitStableLineOver())
		{
			operateOnStableLine();
		}
		if(singlePhasingTimerOn)
		{
			bool b;
			if (structThreePhase_state.u8t_phase_ac_state != AC_3PH)
			{
				b = false;
			}
			if(!b)		//3 phase pin is low
			{
				if(singlePhasingTimerOver())
				{
					operateOnSinglePhasing();
				}
			}
			else
			{
				singlePhasingTimerOn=false;
			}
		}
		if (startTimerOn)
		{
			if (startMotorTimerOver())
			{
				startMotor(false);
			}
		}
		if (startSequenceOn)
		{
			terminateStartRelay();
		}
		if(starDeltaTimerOn)
		{
			terminateStarDeltaTimer();
		}
		if(stopSequenceOn)
		{
			terminateStopRelay();
		}
		
		SIMEventManager();
		if (factory_settings_parameter_struct.ENABLE_M2M)
		{
			M2MEventManager();
		}
	}
}

void start_motor_service(void)
{
	struct extint_chan_conf config_extint_chan_isr;
	extint_chan_get_config_defaults(&config_extint_chan_isr);
	config_extint_chan_isr.gpio_pin           = PIN_PB11A_EIC_EXTINT11;
	config_extint_chan_isr.gpio_pin_mux       = MUX_PB11A_EIC_EXTINT11;
	config_extint_chan_isr.gpio_pin_pull      = EXTINT_PULL_UP;
	config_extint_chan_isr.detection_criteria = EXTINT_DETECT_RISING;
	config_extint_chan_isr.wake_if_sleeping   = false;
	extint_chan_set_config(11, &config_extint_chan_isr);
	
	extint_register_callback(PR2_ISR,11,EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(11,EXTINT_CALLBACK_TYPE_DETECT);
	
	struct port_config pin_confg;
	port_get_config_defaults(&pin_confg);
	pin_confg.direction = PORT_PIN_DIR_INPUT;
	pin_confg.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PR1_PIN, &pin_confg);
	
	//Transferring the below statement from LCD_SERVICE to here, as it solves the problem of the device hanging.
	// Whenever xSemaphoreTake is executed on xADC_Semaphore in task other than which xADC_Semaphore is defined in, than the MCU hangs.
	vSemaphoreCreateBinary(xADC_Semaphore);
	
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	struct port_config config_pins;
	port_get_config_defaults(&config_pins);
	
	config_pins.direction = PORT_PIN_DIR_OUTPUT;
	config_pins.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(START_RELAY_PIN,&config_pins);
	port_pin_set_config(STOP_RELAY_PIN,&config_pins);
	port_pin_set_config(AUTO_ON_LED_PIN,&config_pins);
	port_pin_set_config(THREEPHASE_OK_LED_PIN,&config_pins);
	port_pin_set_config(MOTOR_ON_LED_PIN,&config_pins);
	
	AUTO_ON_LED_OFF;
	MOTOR_ON_LED_OFF;
	THREEPHASE_OK_LED_OFF;
	
	//config_pins.direction = PORT_PIN_DIR_INPUT;
	//config_pins.input_pull = PORT_PIN_PULL_UP;
	//
	//port_pin_set_config(PIN_MOTOR_FEEDBACK,&config_pins);
	
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	
	config_extint_chan.gpio_pin = MOTOR_FEEDBACK_EIC_PIN;
	config_extint_chan.gpio_pin_mux = START_BUTTON_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_BOTH;
	extint_chan_set_config(MOTOR_FEEDBACK_EIC_LINE, &config_extint_chan);
	
	extint_chan_enable_callback(MOTOR_FEEDBACK_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(motor_feedback_callback,MOTOR_FEEDBACK_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	
	//////////////////////////////////////////////////////////////////////////
	
	config_extint_chan.gpio_pin = START_BUTTON_EIC_PIN;
	config_extint_chan.gpio_pin_mux = START_BUTTON_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
	extint_chan_set_config(START_BUTTON_EIC_LINE, &config_extint_chan);
	
	extint_chan_enable_callback(START_BUTTON_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(button_detect_pin_callback,START_BUTTON_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	//////////////////////////////////////////////////////////////////////////
	config_extint_chan.gpio_pin = STOP_BUTTON_EIC_PIN;
	config_extint_chan.gpio_pin_mux = STOP_BUTTON_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
	extint_chan_set_config(STOP_BUTTON_EIC_LINE, &config_extint_chan);
	extint_chan_enable_callback(STOP_BUTTON_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(button_detect_pin_callback,STOP_BUTTON_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	//////////////////////////////////////////////////////////////////////////
	config_extint_chan.gpio_pin = AUTO_BUTTON_EIC_PIN;
	config_extint_chan.gpio_pin_mux = AUTO_BUTTON_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
	extint_chan_set_config(AUTO_BUTTON_EIC_LINE, &config_extint_chan);
	extint_chan_enable_callback(AUTO_BUTTON_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(button_detect_pin_callback,AUTO_BUTTON_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	//////////////////////////////////////////////////////////////////////////
	config_extint_chan.gpio_pin = LCD_SHOW_BUTTON_EIC_PIN;
	config_extint_chan.gpio_pin_mux = LCD_SHOW_BUTTON_EIC_MUX;
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_UP;
	config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
	extint_chan_set_config(LCD_SHOW_BUTTON_EIC_LINE, &config_extint_chan);
	extint_chan_enable_callback(LCD_SHOW_BUTTON_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
	extint_register_callback(button_detect_pin_callback,LCD_SHOW_BUTTON_EIC_LINE,EXTINT_CALLBACK_TYPE_DETECT);
	////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	
	xTaskCreate(vTask_10ms_Timer,NULL,configMINIMAL_STACK_SIZE, NULL,1,NULL);
	xTaskCreate(vTask_100ms_Timer,NULL,configMINIMAL_STACK_SIZE, NULL,1,NULL);
	
	xTaskCreate(Water_Level_Task,NULL,(uint16_t)700,NULL,1,NULL);
	
	xTaskCreate(vTask_MOTORCONTROL,NULL,(uint16_t)700,NULL,1,NULL);
}

bool motor_checkSleepElligible(void)
{
	uint8_t j = 0;
	if (factory_settings_parameter_struct.ENABLE_WATER)
	{
		if (factory_settings_parameter_struct.ENABLE_GP)
		{
			j = 19;
		}
		else
		{
			j = 17;
		}
	}
	else
	{
		if (factory_settings_parameter_struct.ENABLE_CURRENT)
		{
			j = 14;
		}
		else
		{
			j = 12;
		}
	}
	
	bool event=true;
	while(j--)
	{
		if(!simEventTemp[j])
		{
			event=false;
			break;
		}
	}
	if (factory_settings_parameter_struct.ENABLE_M2M)
	{
		if(event && m2mEvent_arr[0] == ME_WAITREGISTER || m2mEvent_arr[1]==ME_WAITREGISTER)
		{
			event = false;
		}
	}
	
	return (!getACPowerState() && !eventOccured && event && !waitStableLineOn && !singlePhasingTimerOn
	&& !startTimerOn && !startSequenceOn && !stopSequenceOn);
	
}


void configure_ac(void)
{
	struct ac_config conf_ac;
	struct ac_events conf_ac_events = {{0}};
	
	struct ac_chan_config conf_ac_channel;
	struct port_config pin_conf;
	
	ac_get_config_defaults(&conf_ac);
	conf_ac.run_in_standby[0] = true;
	conf_ac.dig_source_generator = GCLK_GENERATOR_6;
	ac_init(&ac_instance, AC, &conf_ac);
	
	conf_ac_channel.sample_mode = AC_CHAN_MODE_CONTINUOUS;
	conf_ac_channel.filter = AC_CHAN_FILTER_NONE;
	conf_ac_channel.enable_hysteresis = false;
	conf_ac_channel.output_mode = AC_CHAN_OUTPUT_INTERNAL;
	conf_ac_channel.positive_input = AC_CHAN_POS_MUX_PIN0;
	conf_ac_channel.negative_input = AC_CHAN_NEG_MUX_SCALED_VCC;
	/* Detect threshold 0.515625V */
	conf_ac_channel.vcc_scale_factor = 45;
	conf_ac_channel.interrupt_selection = AC_CHAN_INTERRUPT_SELECTION_TOGGLE;
	ac_chan_set_config(&ac_instance, AC_CHAN_CHANNEL_0, &conf_ac_channel);
	ac_chan_enable(&ac_instance, AC_CHAN_CHANNEL_0);
	
	conf_ac_events.on_event_sample[0] = true;
	ac_enable_events(&ac_instance ,&conf_ac_events);
	
	ac_enable(&ac_instance);
	
	ac_register_callback(&ac_instance, ac_detect_callback,AC_CALLBACK_COMPARATOR_0);
	ac_enable_callback(&ac_instance, AC_CALLBACK_COMPARATOR_0);
}

void configure_rtc(void)
{
	struct rtc_count_config conf_rtc_count;
	struct rtc_count_events conf_rtc_events = {0};
	
	rtc_count_get_config_defaults(&conf_rtc_count);
	conf_rtc_count.prescaler  = RTC_COUNT_PRESCALER_DIV_1;
	conf_rtc_count.mode       = RTC_COUNT_MODE_16BIT;
	conf_rtc_count.continuously_update =  true;
	rtc_count_init(&rtc_instance, RTC, &conf_rtc_count);
	rtc_count_set_period(&rtc_instance, 10);
	conf_rtc_events.generate_event_on_overflow = true;
	
	rtc_count_enable_events(&rtc_instance, &conf_rtc_events);
	rtc_count_enable(&rtc_instance);
}

void ac_detect_callback(struct ac_module *const module_inst)
{
	eventOccured = true;
}

static void motor_feedback_callback(void)
{
	eventOccured = true;
}

void configure_event(void)
{
	struct events_config conf_event;
	events_get_config_defaults(&conf_event);
	conf_event.generator = EVSYS_ID_GEN_RTC_OVF;
	conf_event.edge_detect = EVENTS_EDGE_DETECT_NONE;
	conf_event.path       = EVENTS_PATH_ASYNCHRONOUS;
	
	events_allocate(&resource, &conf_event);
	events_attach_user(&resource, EVSYS_ID_USER_AC_SOC_0);
}

void operateOnButtonEvent(void)
{
	buttonEventOccured=false;
	if (START_BUTTON_INPUT_COMES)
	{
		lastPressTime=xTaskGetTickCount();
		lastButtonEvent=BTNEVENTSTART;
	}
	else if (STOP_BUTTON_INPUT_COMES)
	{
		lastPressTime=xTaskGetTickCount();
		lastButtonEvent=BTNEVENTSTOP;
	}
	//else if (!(factory_settings_parameter_struct.ENABLE_GP) && !(factory_settings_parameter_struct.ENABLE_CURRENT))
	else if (AUTO_BUTTON_INPUT_COMES)
	{
		lastPressTime=xTaskGetTickCount();
		lastButtonEvent=BTNEVENTAUTO;
	}
	else if(LCDSHOW_BUTTON_INPUT_COMES)
	{
		lastPressTime= xTaskGetTickCount();
		lastButtonEvent=BTNEVENTLCDSHOW;
	}
}

void buttonFilter(void)
{
	if(lastButtonEvent>0 && xTaskGetTickCount() - lastPressTime > 70)
	{
		if(lastButtonEvent==BTNEVENTSTART && START_BUTTON_INPUT_COMES)
		{
			lastButtonEvent=0;
			startMotor(false);
		}
		else if(lastButtonEvent==BTNEVENTSTOP && STOP_BUTTON_INPUT_COMES)
		{
			lastButtonEvent=0;
			stopMotor(false,false,true);
		}
		else if(lastButtonEvent==BTNEVENTAUTO && AUTO_BUTTON_INPUT_COMES)
		{
			lastButtonEvent=0;
			saveAutoStartSettings(!((bool)user_settings_parameter_struct.autoStartAddress));  //set AutoStart to True in EEPROM
			resetAutoStart(true);
			if(user_settings_parameter_struct.autoStartAddress)
			{
				simEventTemp[10] = registerEvent(')');
			}
			else
			{
				simEventTemp[11] = registerEvent('[');
			}
		}
		else if(lastButtonEvent==BTNEVENTLCDSHOW && LCDSHOW_BUTTON_INPUT_COMES)
		{
			lastButtonEvent=0;
			setDisplayPause(!varPauseDisplay);
		}
		else
		{
			lastButtonEvent=0;
		}
	}
}


void setDisplayPause(bool value)
{
	varPauseDisplay=value;
}
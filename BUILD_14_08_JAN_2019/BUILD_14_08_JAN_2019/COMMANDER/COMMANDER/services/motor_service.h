
#ifndef MOTOR_SERVICE_H_
#define MOTOR_SERVICE_H_

#include "gsm_service.h"
#include "eeprom_driver.h"


//OverHead Tank
#define OVERHEAD_TANK_HL_PIN	PIN_PB15
#define OVERHEAD_TANK_ML_PIN	PIN_PA12
#define OVERHEAD_TANK_LL_PIN	PIN_PA13

//Under Ground Tank
#define UNDERGRUND_TANK_HL_PIN	PIN_PA14
#define UNDERGRUND_TANK_ML_PIN	PIN_PA15
#define UNDERGRUND_TANK_LL_PIN	PIN_PB23

#define OVERHEADCRITICALLEVEL	0x00
#define OVERHEADMIDLEVEL		0x01
#define OVERHEADHIGHLEVEL		0x02

#define HIGHLEVEL				0x03
#define MIDLEVEL				0x02
#define LOWLEVEL				0x01
#define CRITICALLEVEL			0x00

#define ME_CLEARED				0x00
#define ME_WAITREGISTER			0x01
#define ME_SERVICING			0x02
#define ME_NOTAVAILABLE			0x03

uint32_t lastCurrentReadingTime;
uint32_t currentEventFilterTempTime;
bool enableCurrentBuffer;
uint8_t lastCurrentReading;

bool gotOffCommand;
bool gotOnCommand;
bool offButtonPressed;

volatile uint8_t allPhase;
volatile bool mFeedback;
volatile bool phaseAC;
volatile bool vBoolPhaseSeq;

bool startTimerOn;
uint32_t tempStartTimer;

bool singlePhasingTimerOn;
uint32_t tempSinglePhasingTimer;
uint8_t singlePhasingTime;

bool startSequenceOn;
bool starDeltaTimerOn;
uint32_t tempStartSequenceTimer;
uint8_t startSequenceTimerTime;

bool stopSequenceOn;
uint32_t tempStopSequenceTimer;
uint8_t stopSequenceTimerTime;

bool waitStableLineOn;
uint32_t waitStableLineTimer;
uint8_t waitStableLineTime;

volatile uint32_t lastPressTime;
volatile uint8_t lastButtonEvent;

uint8_t tempWaterEventCount;
uint8_t undergroundLevel;
uint8_t tempUndergroundLevel;

volatile uint8_t overheadLevel;
volatile uint8_t tempOverheadLevel;

bool simEventTemp[19];
char simEvent[19];

//////////////////////////////////////////////////////////////////////////
volatile bool varPauseDisplay;
//////////////////////////////////////////////////////////////////////////
uint8_t m2mEvent_arr[2];
uint8_t mapTable[2];
//////////////////////////////////////////////////////////////////////////

volatile bool eventOccured;
volatile bool buttonEventOccured;

volatile bool waterEventOccured;

//////////////////////////////////////////////////////////////////////////
volatile bool isACpowerAvailable;

volatile uint8_t current_three_phase_state;
volatile uint8_t last_three_phase_state;

//////////////////////////////////////////////////////////////////////////
#define  PR1_PIN  PIN_PB10
#define  PR2_PIN  PIN_PB11
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct ThreePhase_state
{
	volatile uint8_t u8t_phase_sequence_flag;   ////0=UNDEFINED;1=ERROR;2=OK
	//volatile bool boolRPhaseAvailable;
	//volatile bool boolYPhaseAvailable;
	//volatile bool boolBPhaseAvailable;
	volatile uint8_t u8t_phase_ac_state;
	
}structThreePhase_state;

#define THREEPHASE_UNDEFINED 0
#define THREEPHASE_ERROR	 1
#define THREEPHASE_OK		 2
//////////////////////////////////////////////////////////////////////////

#define AC_3PH 3
#define AC_2PH 2
#define AC_1PH 1
#define AC_OFF 0

//////////////////////////////////////////////////////////////////////////

#define START_RELAY_PIN						PIN_PB13
#define START_RELAY_ON						port_pin_set_output_level(START_RELAY_PIN,HIGH)
#define START_RELAY_OFF						port_pin_set_output_level(START_RELAY_PIN,LOW)

#define STOP_RELAY_PIN						PIN_PB12
#define STOP_RELAY_ON						port_pin_set_output_level(STOP_RELAY_PIN,HIGH)
#define STOP_RELAY_OFF						port_pin_set_output_level(STOP_RELAY_PIN,LOW)

#define AUTO_ON_LED_PIN						PIN_PB08
#define AUTO_ON_LED_ON						port_pin_set_output_level(AUTO_ON_LED_PIN,LOW)
#define AUTO_ON_LED_OFF						port_pin_set_output_level(AUTO_ON_LED_PIN,HIGH)

#define THREEPHASE_OK_LED_PIN				PIN_PB07
#define THREEPHASE_OK_LED_ON				port_pin_set_output_level(THREEPHASE_OK_LED_PIN,LOW)
#define THREEPHASE_OK_LED_OFF				port_pin_set_output_level(THREEPHASE_OK_LED_PIN,HIGH)

#define MOTOR_ON_LED_PIN					PIN_PA05
#define MOTOR_ON_LED_ON						port_pin_set_output_level(MOTOR_ON_LED_PIN,LOW)
#define MOTOR_ON_LED_OFF					port_pin_set_output_level(MOTOR_ON_LED_PIN,HIGH)

#define START_BUTTON_PIN					PIN_PB06
#define START_BUTTON_INPUT_COMES			!(port_pin_get_input_level(START_BUTTON_PIN))
#define START_BUTTON_INPUT_NOT_COMES		port_pin_get_input_level(START_BUTTON_PIN)


#define STOP_BUTTON_PIN						PIN_PB05
#define STOP_BUTTON_INPUT_COMES			   !(port_pin_get_input_level(STOP_BUTTON_PIN))
#define STOP_BUTTON_INPUT_NOT_COMES		   port_pin_get_input_level(STOP_BUTTON_PIN)

#define AUTO_BUTTON_PIN						PIN_PA28
#define AUTO_BUTTON_INPUT_COMES			   !(port_pin_get_input_level(AUTO_BUTTON_PIN))
#define AUTO_BUTTON_INPUT_NOT_COMES			port_pin_get_input_level(AUTO_BUTTON_PIN)

#define LCDSHOW_BUTTON_PIN					PIN_PB09
#define LCDSHOW_BUTTON_INPUT_COMES			!(port_pin_get_input_level(LCDSHOW_BUTTON_PIN))
#define LCDSHOW_BUTTON_INPUT_NOT_COMES		port_pin_get_input_level(LCDSHOW_BUTTON_PIN)


//////////////////////////////////////////////////////////////////////////
#define START_BUTTON_EIC_PIN				PIN_PB06A_EIC_EXTINT6
#define STOP_BUTTON_EIC_PIN					PIN_PB05A_EIC_EXTINT5
#define AUTO_BUTTON_EIC_PIN					PIN_PA28A_EIC_EXTINT8
#define LCD_SHOW_BUTTON_EIC_PIN				PIN_PB09A_EIC_EXTINT9

#define START_BUTTON_EIC_LINE				6
#define STOP_BUTTON_EIC_LINE				5
#define AUTO_BUTTON_EIC_LINE				8
#define LCD_SHOW_BUTTON_EIC_LINE			9

#define START_BUTTON_EIC_MUX				MUX_PB06A_EIC_EXTINT6
#define STOP_BUTTON_EIC_MUX					MUX_PB05A_EIC_EXTINT5
#define AUTO_BUTTON_EIC_MUX					MUX_PA28A_EIC_EXTINT8
#define LCD_SHOW_BUTTON_EIC_MUX				MUX_PB09A_EIC_EXTINT9
//////////////////////////////////////////////////////////////////////////

#define PIN_MOTOR_FEEDBACK					PIN_PB14
#define MOTOR_FEEDBACK_EIC_PIN				PIN_PB14A_EIC_EXTINT14
#define MOTOR_FEEDBACK_EIC_LINE				14
#define START_BUTTON_EIC_MUX				MUX_PB14A_EIC_EXTINT14


//////////////////////////////////////////////////////////////////////////

#define BTNEVENTSTART	1
#define BTNEVENTSTOP	2
#define BTNEVENTAUTO	3
#define BTNEVENTLCDSHOW 4


#define CR_OVER2 2
#define CR_OVER 1
#define CR_UNDER 2
#define CR_NORMAL 0


//////////////////////////////////////////////////////////////////////////
struct adc_module adc_inst;
//////////////////////////////////////////////////////////////////////////

struct Analog_Parameter
{
	volatile uint32_t PhaseRY_Voltage;
	volatile uint32_t PhaseYB_Voltage;
	volatile uint32_t PhaseBR_Voltage;
	
	volatile uint32_t Motor_Current_ADC_Value;
	volatile uint32_t Motor_Current;
	volatile uint16_t Motor_Current_IntPart;
	volatile uint16_t Motor_Current_DecPart;
	
	volatile uint32_t Battery_Voltage;
	volatile uint8_t Battery_percentage;
	
}Analog_Parameter_Struct;


void readOverHeadWaterSensorState(bool *olow,bool *ohigh);
void updateOverheadLevel(uint8_t level);
uint8_t getOverHeadWaterSensorState(void);
void overHeadWaterStatusOnCall(bool current);
void readWaterSensorState(bool *low,bool *mid,bool *high);
void updateUndergroundLevel(uint8_t level);
uint8_t getWaterSensorState(void);
void waterStatusOnCall(bool current);

void operateOnWaterEvent(void);

void Configure_ADC0(void);
uint32_t Read_ADC0(uint32_t adc_pin,uint16_t samples);
uint32_t Read_Voltage_ADC0(uint32_t adc_pin);
void autoSetCurrent(void);
void speakAmpere(void);
void PR2_ISR(void);

bool should_Detect_New_Voltage(void);
void set_Three_Phase_State_From_Voltage(void);
void detect_Motor_Current(void);
bool should_Detect_New_Current(void);
void detect_Three_Phase_Voltage(void);
void detect_battery_voltage_and_percentage(void);
bool getACPowerState(void);
void setACPowerState(bool state);
uint8_t getAllPhaseState(void);
void setAllPhaseState(uint8_t state);
bool getPhaseSequence(void);
void setPhaseSequence(bool phaseSequence);
bool getMotorState(void);
void setMotorState(bool state);
bool getMotorState_from_pin(void);
void readSensorState(uint8_t *var3phaseState, bool *var3PhaseSequence, bool *motorState, bool *acPhaseState);
void updateSensorState(uint8_t var3PhaseState, bool var3PhaseSequence, bool motorState, bool acPhaseState);
void resetAutoStart(bool setChange);
void triggerAutoStart(void);
void operateOnEvent(void);
uint8_t checkLineSensors(void);
void operateOnStableLine(void);
bool waitStableLineOver(void);
void startMotor(bool commanded);
void stopMotor(bool commanded, bool forceStop,bool offButton);
bool startMotorTimerOver(void);
void unknownMotorOff(void);
bool singlePhasingTimerOver(void);
void operateOnSinglePhasing(void);
void terminateStopRelay(void);
void terminateStarDeltaTimer(void);
void terminateStartRelay(void);
void statusOnCall(void);
void setM2MEventState(uint8_t eventNo, uint8_t state);
void M2MEventManager(void);
void SIMEventManager(void);
void checkCurrentConsumption(void);

void start_motor_service(void);

bool motor_checkSleepElligible(void);

void configure_ac(void);
void ac_detect_callback(struct ac_module *const module_inst);
void configure_rtc(void);
void configure_event(void);

void buttonFilter(void);
void operateOnButtonEvent(void);

void setDisplayPause(bool);

#endif /* MOTOR_SERVICE_H_ */
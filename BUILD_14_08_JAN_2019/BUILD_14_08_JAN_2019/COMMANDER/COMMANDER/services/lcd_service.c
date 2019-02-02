#include "lcd_service.h"

static void lcd_displaying_task(void *params);
static QueueHandle_t xfour_Second_Queue;
static TimerHandle_t four_Second_timeout_timer=NULL;
static void four_second_timer_callback(TimerHandle_t timer);

static void lcd_displaying_task(void *params)
{
	UNUSED(params);
	
	lcd_in_sleep = false;
	
	LCD_PWR_CONFIG();
	LCD_PWR_EN();
	vTaskDelay(500);
	
	LCD_init();
	
	uint8_t screen=1;
	
	bool four_sec_timer_is_active = true;
	bool two_sec_timer_is_active = false;
	
	uint8_t time=0;
	
	byte Network_0[8]={
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000
	};
	
	byte Network_1[8]={
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B11111,
		0B11111
	};
	byte Network_2[8]={
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B11111,
		0B11111,
		0B11111
	};
	byte Network_3[8]={
		0B00000,
		0B00000,
		0B00000,
		0B00000,
		0B11111,
		0B11111,
		0B11111,
		0B11111
	};
	byte Network_4[8]={
		0B00000,
		0B00000,
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111
	};

	byte Network_5[8]={
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111,
		0B11111
	};
	
	LCD_Create_Custom_createChar(0,Network_0);
	LCD_Create_Custom_createChar(1,Network_1);
	LCD_Create_Custom_createChar(2,Network_2);
	LCD_Create_Custom_createChar(3,Network_3);
	LCD_Create_Custom_createChar(4,Network_4);
	LCD_Create_Custom_createChar(5,Network_5);
	
	LCD_clear();
	LCD_setCursor(0,0);
	lcd_printf(" KRISHNA  SMART ");
	LCD_setCursor(0,1);
	lcd_printf("   TECHNOLOGY   ");
	vTaskDelay(3000);
	
	LCD_clear();
	LCD_setCursor(0,0);
	lcd_printf("  SOFTWARE VER  ");
	LCD_setCursor(0,1);
	lcd_printf("%s",VERSION_NO);
	vTaskDelay(2000);
	
	four_Second_timeout_timer = xTimerCreate(NULL,(1 * 4000 / portTICK_PERIOD_MS), pdTRUE, NULL, four_second_timer_callback);
	xTimerStart( four_Second_timeout_timer, 0 );
	
	
	for (;;)
	{
		if (lcd_in_sleep)
		{
			lcd_in_sleep = false;
			LCD_PWR_EN();
			vTaskDelay(100);
			LCD_init();
		}
		switch(screen)
		{
			case  1:
			{
				LCD_setCursor(0,0);
				lcd_printf("VRY   VYB   VBR ");
				LCD_setCursor(0,1);
				lcd_printf("%03lu   ",(Analog_Parameter_Struct.PhaseRY_Voltage));
				lcd_printf("%03lu   ",(Analog_Parameter_Struct.PhaseYB_Voltage));
				lcd_printf("%03lu ",(Analog_Parameter_Struct.PhaseBR_Voltage));
				break;
			}
			case  2:
			{
				LCD_setCursor(0,0);
				lcd_printf("MOTOR CURRENT:  ");
				LCD_setCursor(0,1);
				lcd_printf("%03lu.%02lu            ",(Analog_Parameter_Struct.Motor_Current_IntPart),(Analog_Parameter_Struct.Motor_Current_DecPart));
				break;
			}
			
			case 3:
			{
				LCD_setCursor(0,0);
				lcd_printf("3 PHASE SEQ:");
				if (structThreePhase_state.u8t_phase_sequence_flag == THREEPHASE_OK)
				{
					lcd_printf(" OK ");
				}
				else
				{
					lcd_printf(" ERR");
				}
				LCD_setCursor(0,1);
				lcd_printf("PHASE STATE:");
				if (structThreePhase_state.u8t_phase_ac_state == AC_3PH)
				{
					lcd_printf(" 3PH ");
				}
				else if(structThreePhase_state.u8t_phase_ac_state == AC_2PH)
				{
					lcd_printf(" 2PH ");
				}
				else
				{
					lcd_printf(" OFF");
				}
				break;
			}
			case 4:
			{
				LCD_setCursor(0,0);
				lcd_printf("O-LEVEL : ");
				if (overheadLevel == OVERHEADHIGHLEVEL)
				{
					lcd_printf("HIGH  ");
				}
				else if (overheadLevel == OVERHEADMIDLEVEL)
				{
					lcd_printf("MID   ");
				}
				else if (overheadLevel == OVERHEADCRITICALLEVEL)
				{
					lcd_printf("LOW   ");
				}
				LCD_setCursor(0,1);
				lcd_printf("U-LEVEL : ");
				if (undergroundLevel == CRITICALLEVEL)
				{
					lcd_printf("CRTCL ");
				}
				else if (undergroundLevel == LOWLEVEL)
				{
					lcd_printf("LOW   ");
				}
				else if (undergroundLevel == MIDLEVEL)
				{
					lcd_printf("MID   ");
				}
				else if (undergroundLevel == HIGHLEVEL)
				{
					lcd_printf("HIGH  ");
				}
				break;
			}
			case 5:
			{
				LCD_setCursor(0,0);
				lcd_printf("BatteryPer: %u%% ",Analog_Parameter_Struct.Battery_percentage);
				LCD_setCursor(0,1);
				lcd_printf("SIGNAL : ");
				LCD_setCursor(9,1);

				for (uint8_t i=0;i<=Signal_Strength;i++)
				{
					LCD_write(i);
				}
				
				lcd_printf("       ");
				break;
			}
		}
		if (xQueueReceive(xfour_Second_Queue,&time,0))
		{
			xTimerChangePeriod( four_Second_timeout_timer, 4000/portTICK_PERIOD_MS, portMAX_DELAY);
			if(varPauseDisplay==false)
			{
				screen++;
			}
		}
		
		if (screen>5)
		{
			screen=1;
		}
		
		vTaskDelay(500);
	}
}


void start_lcd_service(void)
{
	
	xfour_Second_Queue=xQueueCreate(1,sizeof(uint8_t));	
	xTaskCreate(lcd_displaying_task,NULL,(uint16_t)400,NULL,1,NULL);
	
}

static void four_second_timer_callback(TimerHandle_t timer)
{
	uint8_t ucharfour_Second=1;
	xQueueSendFromISR(xfour_Second_Queue,&ucharfour_Second,0);
}
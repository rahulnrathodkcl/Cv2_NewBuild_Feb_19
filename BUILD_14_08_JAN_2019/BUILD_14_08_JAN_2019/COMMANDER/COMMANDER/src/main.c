#include <asf.h>
#include "eeprom_driver.h"
#include "gsm_service.h"
#include "motor_service.h"
#include "lcd_service.h"

#define sleepWaitTime 18000

//#define sleepWaitTime 2000

bool checkSleepElligible(void);
static void vTask_sleep_manager(void *params);

void gotoSleep(void);

bool initSleepSeqeunce=false;
uint32_t tempSleepWait=0;

int main (void)
{
	system_init();
	
	/* Disable digital interfaces to unused peripherals */
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBA,
	PM_APBAMASK_PAC0 | PM_APBAMASK_WDT);
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBB,
	PM_APBBMASK_PAC1 | PM_APBBMASK_DSU);
	system_apb_clock_clear_mask(SYSTEM_CLOCK_APB_APBC,
	PM_APBCMASK_PAC2 | PM_APBCMASK_AC | PM_APBCMASK_DAC);
	
	/* Disable NVM low power mode during sleep due to lockups (device errata) */
	NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
	
	system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);
	
	system_interrupt_enable_global();
	
	//irq_initialize_vectors();
	//cpu_irq_enable();
	delay_init();
	init_eeprom();
	
	
	
	start_lcd_service();
	start_gsm_service();
	start_motor_service();
	
	xTaskCreate(vTask_sleep_manager,NULL,(uint16_t)100,NULL,1,NULL);
	
	vTaskStartScheduler();
	
	for (;;)
	{
	}
}


bool checkSleepElligible(void)
{
	return (/*!turnOffTimerOn && */!getACPowerState() && motor_checkSleepElligible() && gsm_checkSleepElligible());
}

static void vTask_sleep_manager(void *params)
{
	UNUSED(params);
	for (;;)
	{
		if (checkSleepElligible())
		{
			if(!initSleepSeqeunce)
			{
				tempSleepWait=xTaskGetTickCount();
				initSleepSeqeunce=true;
			}
			else if(initSleepSeqeunce && xTaskGetTickCount()-tempSleepWait>sleepWaitTime)
			{
				uint8_t cnt=10;
				bool led=false;
				do
				{
					led=!led;
					if(led)
					THREEPHASE_OK_LED_ON;
					else
					THREEPHASE_OK_LED_OFF;

					tempSleepWait=xTaskGetTickCount();
					while(xTaskGetTickCount()-tempSleepWait<200)
					{}
				}while(--cnt);
				
				gotoSleep();
			}
		}
		else
		{
			initSleepSeqeunce=false;
		}
	}
}

void gotoSleep(void)
{
	LCD_PWR_DIS();
	lcd_in_sleep = true;
	system_sleep();
}
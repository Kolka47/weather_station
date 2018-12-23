#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "pcd8544/pcd8544.h"  	// lcd lib
#include "bmp180/bmp180.h"		//bmp180 lib

#define LIGHT (1<<PD5)
#define BUZZER (1<<PC1)
#define BUZZER_1 PORTC |= BUZZER;
#define BUZZER_0 PORTC &= ~BUZZER;
#define BUZZER_TOGG PORTC ^= BUZZER;


uint8_t EEMEM xyz;


volatile uint8_t flag_1s, tim0_flag;
volatile uint8_t msw1_cnt, msw2_cnt, msw1_flag, msw2_flag, msw2_long_flag;
volatile uint8_t activity_cnt; 	//flag set after any activity, zero after 5 seconds
volatile uint8_t alarm_flag;
volatile uint16_t alarm_cnt;


int main(void)
{
	//uint8_t abc;

	//abc = eeprom_read_byte(&xyz);

	// structure for actual time
	typedef struct {
		uint16_t second, minute, hour, day, month;
		uint16_t year;
	}t_time;

	t_time watch1;
	watch1.second =0;// eeprom_read_byte(&time_tab_ee[0]);
	watch1.minute =0;// eeprom_read_byte(&time_tab_ee[1]);
	watch1.hour =0;// eeprom_read_byte(&time_tab_ee[2]);
	watch1.day =1;// eeprom_read_byte(&time_tab_ee[3]);
	watch1.month =1;// eeprom_read_byte(&time_tab_ee[4]);
	watch1.year =2018;// ((uint16_t)eeprom_read_byte(&time_year_ee[0])<<8) + eeprom_read_byte(&time_year_ee[1]);

	t_time alarm1;
	alarm1.minute = 0;
	alarm1.hour = 0;

	uint8_t alarm_onoff = 0;

	uint16_t menu = 0;

	static uint16_t *temp_time;

	DDRD |= LIGHT;
	PORTD |= LIGHT;

	DDRC |= BUZZER;


	//========================= MSW PORTS =======================================

		DDRD &= ~(1<<PD3)|(1<<PD2);		//inputs
		PORTD |= (1<<PD3)|(1<<PD2);		//internal pull up to vcc

	//===========================================================================



	//=============== INITIALIZE TIMER0 IN OVERFLOW MODE ========================

		TCCR0 |= (1<<CS01)|(1<<CS00);	//prescale fcpu/64
		TIMSK |= (1<<TOIE0);		//enable tim0_ovf interrupt
		TCNT0 = 125;		//interrupt every 1ms
	//===========================================================================


	//===============  INITIALIZE TIMERA2 IN ASYNCH MODE ========================

		ASSR |= (1<<AS2);				//wlaczenie trybu asynchronicznego tiemra2
		TCCR2 |= (1<<CS22)|(1<<CS20);				//preskaler 128
		TIMSK |= (1<<TOIE2);
	//=============================================================================

		sei();	//enable global interrupt


 alarm_flag = 0;

    Lcd_Init();  // inicjacja LCD
    Lcd_Clr();
    Lcd_Upd();

    bmp180_init();	//initbmp180

    // WHILE
	while(1)
	{

		// light up lcd after any activity
		if(activity_cnt <5)
		{
			PORTD |= LIGHT;
		}else PORTD &= ~LIGHT;


		// after wake up every 1s count real time
		if(flag_1s)
		{

			if(activity_cnt <5) activity_cnt++;

			flag_1s = 0;

			watch1.second++;
			if(watch1.second == 60)
			{
				watch1.second = 0;
				watch1.minute++;

				if(watch1.minute == 60)
				{
					watch1.minute = 0;
					watch1.hour++;

					if(watch1.hour == 24)
					{
						watch1.hour = 0;
						watch1.day++;

						if(watch1.month < 8 )	//for months 1-7 odd months have 31 days
						{
							if(watch1.month %2 == 1)
							{
								if(watch1.day == 32)
								{
									watch1.day = 1;
									watch1.month++;
								}
							}else if(watch1.month %2 == 0 && watch1.month != 2)
							{
								if(watch1.day == 31)
								{
									watch1.day = 1;
									watch1.month++;
								}
							}else if(watch1.month == 2)
							{
								if(watch1.year %4 == 0)
								{
									if(watch1.day == 30)
									{
										watch1.day = 1;
										watch1.month++;
									}
								}else
								{
									if(watch1.day == 29)
									{
										watch1.day = 1;
										watch1.month++;
									}
								}
							}
						}else if(watch1.month >= 8)	// months 8-12 even months have 31 days
						{
							if(watch1.month %2 == 0)
							{
								if(watch1.day == 32)
								{
									watch1.day = 1;
									watch1.month++;
								}
							}else
							{
								if(watch1.day == 31)
								{
									watch1.day = 1;
									watch1.month++;
								}
							}
						}
						if(watch1.month == 13)
						{
							watch1.month = 1;
							watch1.year++;
						}
					}
				}
			}

			if(alarm_onoff)
			{
				if(watch1.minute == alarm1.minute && watch1.hour == alarm1.hour && watch1.second == 0) alarm_flag = 1;
			}

		}


		//main program switch
		switch(menu)
		{

			// menu 0 - display time and waether conditions
			case 0:

				Lcd_Clr();

				bmp180_ut();
				bmp180_up();

				Lcd_Locate(10,3);
				Lcd_Str("Tin");
				Lcd_Locate(52,3);
				Lcd_Str("Tout");

				//display temperature
				Lcd_Locate(0,4);
				Lcd_Int(bmp180_temp() /10, DEC );
				Lcd_Char('.');
				Lcd_Int(bmp180_temp() %10, DEC);
				Lcd_Char(127);
				Lcd_Char('C');

				//display pressure
				Lcd_Locate(21,5);
				Lcd_Int(bmp180_press() / 100, DEC);
				Lcd_Str("hPa");

				// display date
				Lcd_Locate(12,0);
				if(watch1.day < 10 ) Lcd_Int(0, DEC);
				else Lcd_Int(watch1.day/10, DEC);
				Lcd_Int(watch1.day % 10, DEC);
				Lcd_Char('.');
				if(watch1.month < 10 ) Lcd_Int(0, DEC);
				else Lcd_Int(watch1.month/10, DEC);
				Lcd_Int(watch1.month % 10, DEC);
				Lcd_Char('.');
				Lcd_Int(watch1.year, DEC);

				//display time
				Lcd_Locate(12,1);
				if(watch1.hour < 10 ) Lcd_BigChar(0);
				else Lcd_BigChar(watch1.hour/10);
				Lcd_BigChar(watch1.hour % 10);
				Lcd_BigChar(10);	// :
				if(watch1.minute < 10 ) Lcd_BigChar(0);
				else Lcd_BigChar(watch1.minute/10);
				Lcd_BigChar(watch1.minute % 10);
				Lcd_Locate(72,1);
				if(watch1.second < 10 ) Lcd_Int(0, DEC);
				else Lcd_Int(watch1.second/10, DEC);
				Lcd_Int(watch1.second % 10, DEC);

				//display alarm on off
				if(alarm_onoff == 1)
				{
					Lcd_Locate(78,0);
					Lcd_Char('A');
				}
				Lcd_Upd();


				// after msw1 pressed go to next menu
				if(msw1_flag)
				{
					msw1_flag = 0;
					menu = 1;

					Lcd_Locate(0,0);
					Lcd_Int(menu, DEC);
				}
				if(msw2_flag)
				{
					msw2_flag = 0;

					if(alarm_flag)
					{
						alarm_flag = 0;
						BUZZER_0;
					}
				}


			break;


			// menu 1 - set time and date
			case 1:

				Lcd_Clr();
				Lcd_Locate(18,1);
				Lcd_Str("SET TIME");
				Lcd_Locate(37,3);
				Lcd_BigChar(11);
				Lcd_Upd();

				if(msw1_flag)
				{
					msw1_flag = 0;
					menu = 2;

					Lcd_Locate(0,0);
					Lcd_Int(menu, DEC);
				}
				if(msw2_flag)
				{
					msw2_flag = 0;
					menu = 10;

					temp_time = &watch1.minute;
				}

			break;

			// menu 1 - 10 - set minutes
			case 10:

				Lcd_Clr();
				Lcd_Locate(36,1);
				Lcd_BigChar(10);	//:
				Lcd_Locate(48,1);
				if(*temp_time < 10) Lcd_BigChar(0);
				else Lcd_BigChar(*temp_time/10);
				Lcd_BigChar(*temp_time%10);
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					if(*temp_time < 59) (*temp_time)++;
					else *temp_time = 0;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 0) (*temp_time)--;
						else *temp_time = 59;
					}else
					{
						msw2_flag = 0;

						watch1.minute = *temp_time;
						watch1.second = 0;
						menu = 11;

						temp_time = &watch1.hour;
					}
				}
			break;


			// menu 1 - 11 - set hours
			case 11:

				Lcd_Clr();
				Lcd_Locate(36,1);
				Lcd_BigChar(10);	//:
				Lcd_Locate(12,1);
				if(*temp_time < 10) Lcd_BigChar(0);
				else Lcd_BigChar(*temp_time/10);
				Lcd_BigChar(*temp_time%10);
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					if(*temp_time < 23) (*temp_time)++;
					else *temp_time = 0;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 0) (*temp_time)--;
						else *temp_time = 23;
					}else
					{
						msw2_flag = 0;

						watch1.hour = *temp_time;
						watch1.second = 0;
						menu = 12;

						temp_time = &watch1.day;
					}
				}
			break;

			// menu 1 - 12 - set day
			case 12:

				Lcd_Clr();
				Lcd_Locate(24,0);
				Lcd_Char('.');	// .
				Lcd_Locate(12,0);
				if(*temp_time < 10) Lcd_Int(0, DEC);
				else Lcd_Int(*temp_time/10, DEC);
				Lcd_Int(*temp_time%10, DEC);
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					if(*temp_time < 31) (*temp_time)++;
					else *temp_time = 1;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 1) (*temp_time)--;
						else *temp_time = 31;
					}else
					{
						msw2_flag = 0;

						watch1.day = *temp_time;
						watch1.second = 0;
						menu = 13;

						temp_time = &watch1.month;
					}
				}
			break;


			// menu 1 - 13 set month
			case 13:

				Lcd_Clr();
				Lcd_Locate(30,0);
				if(*temp_time < 10) Lcd_Int(0, DEC);
				else Lcd_Int(*temp_time/10, DEC);
				Lcd_Int(*temp_time%10, DEC);
				Lcd_Char('.');	// .
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					if(*temp_time < 12) (*temp_time)++;
					else *temp_time = 1;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 1) (*temp_time)--;
						else *temp_time = 12;
					}else
					{
						msw2_flag = 0;

						watch1.month = *temp_time;
						watch1.second = 0;
						menu = 14;

						temp_time = &watch1.year;
					}
				}


			break;

			// menu 1 - 14 set year
			case 14:

				Lcd_Clr();
				Lcd_Locate(48,0);
				Lcd_Int(*temp_time, DEC);
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					if(*temp_time < 2099) (*temp_time)++;
					else *temp_time = 2000;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 2000) (*temp_time)--;
						else *temp_time = 2099;
					}else
					{
						msw2_flag = 0;

						watch1.year = *temp_time;

						if( watch1.year % 4 == 0 && watch1.month == 2)
						{
							if (watch1.day > 29) watch1.day = 29;
						}
						else if( watch1.month < 8 && watch1.month % 2 == 0 && watch1.month != 2)
						{
							if (watch1.day > 30) watch1.day = 30;
						}
						else if( watch1.month >=8 && watch1.month % 2 == 1)
						{
							if (watch1.day > 30) watch1.day = 30;
						}
						else if(watch1.month == 2)
						{
							if (watch1.day > 28) watch1.day = 28;
						}

						watch1.second = 0;
						menu = 0;
					}
				}


			break;


			// menu 2 - set alarm
			case 2:

				Lcd_Clr();
				Lcd_Locate(15,1);
				Lcd_Str("SET ALARM");
				Lcd_Locate(37,3);
				Lcd_BigChar(12);
				Lcd_Upd();

				if(msw1_flag)
				{
					msw1_flag = 0;
					menu = 0;

					Lcd_Locate(0,0);
					Lcd_Int(menu, DEC);
				}
				if( msw2_flag)
				{
					msw2_flag = 0;
					menu = 20;

				}
			break;

			case 20:

				Lcd_Clr();
				Lcd_Locate(27,1);
				Lcd_Str("ALARM");

				if( alarm_onoff)
				{
					Lcd_Locate(36,3);
					Lcd_Str("ON");
				}else
				{
					Lcd_Locate(33,3);
					Lcd_Str("OFF");
				}
				Lcd_Upd();

				if(msw2_flag)
				{
					msw2_flag = 0;

					alarm_onoff = alarm_onoff == 1 ? 0 : 1;
				}

				if(msw1_flag)
				{
					msw1_flag = 0;

					menu = alarm_onoff == 0 ? 0 : 21;

					temp_time = &alarm1.minute;
				}

			break;

			case 21:

				Lcd_Clr();
				Lcd_Locate(36,1);
				Lcd_BigChar(10);	//:
				Lcd_Locate(48,1);
				if(*temp_time < 10) Lcd_BigChar(0);
				else Lcd_BigChar(*temp_time/10);
				Lcd_BigChar(*temp_time%10);
				Lcd_Upd();

				if( msw2_flag)
				{
					msw2_flag = 0;

					if( *temp_time < 59) (*temp_time)++;
					else *temp_time = 0;
				}

				if( msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 0) (*temp_time)--;
						else *temp_time = 59;
					}else
					{
						msw2_flag = 0;

						alarm1.minute = *temp_time;
						menu = 22;

						temp_time = &alarm1.hour;
					}
				}

			break;


			case 22:

				Lcd_Clr();
				Lcd_Locate(36,1);
				Lcd_BigChar(10);	//:
				Lcd_Locate(12,1);
				if(*temp_time < 10) Lcd_BigChar(0);
				else Lcd_BigChar(*temp_time/10);
				Lcd_BigChar(*temp_time%10);
				Lcd_Upd();

				if( msw2_flag)
				{
					msw2_flag = 0;

					if( *temp_time < 23) (*temp_time)++;
					else *temp_time = 0;
				}

				if( msw1_flag)
				{
					msw1_flag = 0;

					if(msw2_long_flag)
					{
						if(*temp_time > 0) (*temp_time)--;
						else *temp_time = 23;
					}else
					{
						msw2_flag = 0;

						alarm1.hour = *temp_time;
						menu = 0;

					}
				}

			break;


		}





	}
	// WHILE
}



// Timer0 interrupt every 1ms
ISR(TIMER0_OVF_vect)
{
	TCNT0 = 125;

	//alarm
	if( alarm_flag )
	{
		alarm_cnt++;

		if(alarm_cnt < 100 || (alarm_cnt >= 200 && alarm_cnt < 300) || (alarm_cnt >= 400 && alarm_cnt < 500) )
		{
			BUZZER_1
		}else BUZZER_0;

		if(alarm_cnt == 1000) alarm_cnt = 0;
	}



	// check if msw1 is pressed
	if(!(PIND & (1<<PD3)))
	{
		if(msw1_cnt < 11) msw1_cnt++;

		if(msw1_cnt == 10)
		{
			msw1_flag = 1;
			activity_cnt = 0;
		}

	}else msw1_cnt = 0;

	// check if msw2 is pressed
	if(!(PIND & (1<<PD2)))
	{
		if(msw2_cnt < 11) msw2_cnt++;

		if(msw2_cnt == 10)
		{
			msw2_flag = 1;
			msw2_long_flag = 1;
			activity_cnt = 0;
		}

	}else
	{
		msw2_cnt = 0;
		msw2_long_flag = 0;
	}

}



// asynchronous timer2 int
ISR(TIMER2_OVF_vect)
{
	TCNT2 = 0;

	flag_1s = 1;
}


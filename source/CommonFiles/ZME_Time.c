#include "ZME_Includes.h"
#include "ZME_Time.h"



WORD daysToMonth[12]={0,31,59,90,120,151,181,212,243,273,304,334};

BYTE daysInMonth(BYTE month, WORD year)
{
	if(month == 2)
		return IS_LEAP_YEAR(year) ? 29 : 28;
	
	if(month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	
	return 31;
}
DWORD makeTimeStamp(WORD year,
				   BYTE month,
				   BYTE day,
				   BYTE hour,
				   BYTE minute,
				   BYTE second)
{


	
	return 	 (((DWORD)DELTA_YEAR(year))*SECONDS_IN_YEAR) +
					((((DWORD)year - ORIGIN_LEAP_YEAR) >> 2)+  //COUNT_PREV_LEAP_YEARS(year) +
						DAYS_TO_MONTH(month,year) +
						day-1 )*SECONDS_IN_DAY +	
					(((DWORD)hour) * SECONDS_IN_HOUR) +
					(((DWORD)minute) * 60) +
					second;				   
}

void decodeTimeStamp(DWORD timestamp,
					 WORD * year,
					 BYTE * month,
					 BYTE * day,
					 BYTE * hour,
					 BYTE * minute,
					 BYTE * second)
{
	DWORD rem_seconds, dseconds;
	DWORD  days;
	BYTE  day_c;	
	
	*year = (WORD)(timestamp / SECONDS_IN_YEAR) + ORIGIN_YEAR;
	rem_seconds = timestamp % SECONDS_IN_YEAR;
	
	dseconds = ((((DWORD)*year - ORIGIN_LEAP_YEAR) >> 2))*SECONDS_IN_DAY;
	
	if((((DWORD)*year - ORIGIN_LEAP_YEAR) % 4) == 0)
		dseconds	-= SECONDS_IN_DAY;
	
	if(dseconds > rem_seconds)
	{
		dseconds -= SECONDS_IN_YEAR;
		(*year)--;
		if((((DWORD)*year - ORIGIN_LEAP_YEAR) % 4) == 0)
			dseconds -=  SECONDS_IN_DAY;
		
	}
	
	rem_seconds -= dseconds;
	
	days = rem_seconds /  SECONDS_IN_DAY;
	rem_seconds = rem_seconds %  SECONDS_IN_DAY;
	

	*month = 1;

	while(days > (day_c = daysInMonth(*month, *year)))
	{	
		days -= day_c;
		(*month)++;
	}
	
	
	*day 		= days + 1;	
	
	
	
	*hour 	= rem_seconds / SECONDS_IN_HOUR;
	rem_seconds = rem_seconds % SECONDS_IN_HOUR;
	*minute = rem_seconds / 60;
	*second = rem_seconds % 60;
	  	
	
}
					 
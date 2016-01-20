#ifndef ZME_TIME
#define ZME_TIME

#define ORIGIN_YEAR 				1970

//  Код для вычесления високосных лет примитивен, но должен правильно отрабатывать
//  для диапазона лет  [2000;2096]  
#define ORIGIN_LEAP_YEAR 			1968
#define DELTA_YEAR(N)				(N-ORIGIN_YEAR)
#define COUNT_PREV_LEAP_YEARS_D(N)	(N/4)
#define COUNT_PREV_LEAP_YEARS(N)	(COUNT_PREV_LEAP_YEARS_D(N-ORIGIN_LEAP_YEAR))
#define IS_LEAP_YEAR_D(N)		    ((N%4) == 0)
#define IS_LEAP_YEAR(N)				((((DWORD)N - ORIGIN_LEAP_YEAR) % 4) == 0) ///IS_LEAP_YEAR_D(N-ORIGIN_LEAP_YEAR))
#define DAYS_IN_YEAR				365
#define SECONDS_IN_HOUR				3600
#define SECONDS_IN_DAY				((DWORD)24*SECONDS_IN_HOUR)
#define SECONDS_IN_YEAR				((DWORD)DAYS_IN_YEAR*SECONDS_IN_DAY)
#define DAYS_TO_MONTH_D(M,Y)	 	((IS_LEAP_YEAR_D(Y) && M > 2) ?  daysToMonth[M-1]+1: daysToMonth[M-1])
#define DAYS_TO_MONTH(M,Y)	 		((IS_LEAP_YEAR(Y) && M > 2) ?  daysToMonth[M-1]+1: daysToMonth[M-1])
extern WORD daysToMonth[12];//={0,31,59,90,120,151,181,212,243,273,304,334};


DWORD makeTimeStamp(WORD year,
				   BYTE month,
				   BYTE day,
				   BYTE hour,
				   BYTE minute,
				   BYTE second);
				   

void decodeTimeStamp(DWORD timestamp,
					 WORD * year,
					 BYTE * month,
					 BYTE * day,
					 BYTE * hour,
					 BYTE * minute,
					 BYTE * second);
#endif
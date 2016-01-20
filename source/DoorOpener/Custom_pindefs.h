// Pin definitions for Z-wave.me remote K-fob

#ifdef ZW050x

//INT0
#define Button0pinPort          P1
#define Button0pinSHADOW        P1Shadow
#define Button0pinDIR           P1DIR
#define Button0pinSHADOWDIR     P1ShadowDIR
#define Button0pinDIR_PAGE      P1DIR_PAGE
#define Button0pin              0

//INT1
#define Button1pinPort          P1
#define Button1pinSHADOW        P1Shadow
#define Button1pinDIR           P1DIR
#define Button1pinSHADOWDIR     P1ShadowDIR
#define Button1pinDIR_PAGE      P1DIR_PAGE
#define Button1pin              1

//SCK
#define RedLEDpinPort          P2
#define RedLEDpinSHADOW        P2Shadow
#define RedLEDpinDIR           P2DIR
#define RedLEDpinSHADOWDIR     P2ShadowDIR
#define	RedLEDpinDIR_PAGE      P2DIR_PAGE
#define RedLEDpin              4

//ZEROX
#define Relay0pinPort          P3
#define Relay0pinSHADOW        P3Shadow
#define Relay0pinDIR           P3DIR
#define Relay0pinSHADOWDIR     P3ShadowDIR
#define	Relay0pinDIR_PAGE      P3DIR_PAGE
#define Relay0pin              7

//TRIAC
#define Relay1pinPort          P3
#define Relay1pinSHADOW        P3Shadow
#define Relay1pinDIR           P3DIR
#define Relay1pinSHADOWDIR     P3ShadowDIR
#define	Relay1pinDIR_PAGE      P3DIR_PAGE
#define Relay1pin              6

#else

#error Not implemented yet

#endif
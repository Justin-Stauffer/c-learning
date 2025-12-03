Compiled by EWARM 6.21

----------------------------------------------- -------------------------------------------------------------------------------------------
|	Examples:				|				Action							    |
 ----------------------------------------------- -------------------------------------------------------------------------------------------
| "LPC-P1343_Blinking_Led_Polling"		|	Lead "LPC-P1343's" on-board "LED0" to blink with frequency ~ 1Hz.	            |
|						|	Time interval is set via software delay.					    |
 ----------------------------------------------- -------------------------------------------------------------------------------------------
| "LPC-P1343_Blinking_Led_Interrupt"		|	Lead "LPC-P1343's" on-board "LED0" to blink with frequency ~ 4Hz.	            |
|						|	Time interval is set via CT32B0 capture/compare interrupt.			    |
 ----------------------------------------------- -------------------------------------------------------------------------------------------
| "LPC-P1343_LEDs_Running_Light"		|	Make "running light" over "LPC-P1343's" on-board leds "LED0-LED7".	            |
 ----------------------------------------------- -------------------------------------------------------------------------------------------
|"LPC-P1343_LEDs_Running_Light&Buttons"		|	Control "running light" speed! BUT1 decrease it while BUT2 increase it.	            |
|						|	Buttons are recognized like pressed via port external interrupt.		    |
|						|	USBC led indicate when BUT1 or BUT2 is pressed like toggle it's own state.	    |
 ----------------------------------------------- -------------------------------------------------------------------------------------------
|"LPC-P1343_LEDs_Running_Light&Buttons_Scan"	|	Control "running light" speed! BUT1 decrease it while BUT2 increase it.	            |
|						|	Buttons are recognized like pressed via scanning process at CT16B0 interrupt.	    |
|						|	USBC led indicate when BUT1 or BUT2 is pressed like toggle it's own state.	    |
 ----------------------------------------------- ------------------------------------------------------------------------------------------- 
|						|	Open Terminal with settings: 9600-bits/sec, 8-data bits, No parity, 1 Stop bit	    |
|						|	Press any key, and you have to see in terminal next menu:			    |
|   "LPC-P1343_VirtualComPort_LEDs&Buttons"	|											    |	
|						|		***************************************************************		    |
|						|		*** Welcom to OLIMEX LPC-P1343_VirtualComPort demo program! ***		    |
|						|		***************************************************************		    |
|						|		*  Valid Commands are: LEDx_0 or LEDx_1, wher x=0..7;         *		    |
|						|		*    Please Send Command or press any board's button!         *		    |
|						|		***************************************************************		    |
|						|											    |
|						|	Make your choice and enjoy!:)							    |
--------------------------------------------------------------------------------------------------------------------------------------------	


2018/09/07

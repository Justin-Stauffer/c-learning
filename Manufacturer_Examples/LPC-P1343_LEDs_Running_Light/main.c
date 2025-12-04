/**********************************************************************************/
/*    Demo program for:								  */
/*	  Board: LPC-P1343              					  */
/*    Manufacture: OLIMEX                                                   	  */
/*	  COPYRIGHT (C) 2010							  */
/*    Designed by: Engineer Penko T. Bozhkov                                      */
/*    Module Name    :  main module                                               */
/*    File   Name    :  main.c                                                    */
/*    Revision       :  initial                                                   */
/*    Date           :  16.03.2010                                                */
/*    Built with IAR Embedded Workbench Version: 5.41                             */
/**********************************************************************************/
#include  <nxp/iolpc1343.h>
#include  <intrinsics.h>

// Functions Prototypes:
void Delay(volatile unsigned long cycles);
void Init_System_Clock(unsigned long Desired_Sysem_Clock, char Clock_Source);
void Init_PLL(unsigned long PLL_Fclkin, char PLL_Clock_Source, unsigned long PLL_Fclkout);
void init_devices(void);
void main(void);
void Write_LEDs_Port(volatile unsigned long vLEDs_Port);
void LEDs_Running_Light(unsigned char Direction, unsigned long Speed);

// Definitions:
#define LED0_ON   GPIO3DATA &= ~0x00000001;  
#define LED0_OFF  GPIO3DATA |= 0x00000001;
#define LED1_ON   GPIO3DATA &= ~0x00000002;  
#define LED1_OFF  GPIO3DATA |= 0x00000002;
#define LED2_ON   GPIO3DATA &= ~0x00000004;  
#define LED2_OFF  GPIO3DATA |= 0x00000004;
#define LED3_ON   GPIO3DATA &= ~0x00000008;  
#define LED3_OFF  GPIO3DATA |= 0x00000008;
#define LED4_ON   GPIO2DATA &= ~0x00000010;  
#define LED4_OFF  GPIO2DATA |= 0x000000010;
#define LED5_ON   GPIO2DATA &= ~0x00000020;  
#define LED5_OFF  GPIO2DATA |= 0x000000020;
#define LED6_ON   GPIO2DATA &= ~0x00000040;  
#define LED6_OFF  GPIO2DATA |= 0x000000040;
#define LED7_ON   GPIO2DATA &= ~0x00000080;  
#define LED7_OFF  GPIO2DATA |= 0x000000080;


/**********************************************************************************/
/*  Function name: Delay                                                          */
/*  	Parameters                                                                */
/*          Input   :  cycles                                                     */
/*          Output  :  No	                                                  */
/*	Action: Simple delay							  */
/**********************************************************************************/
void Delay(volatile unsigned long cycles){
  while(cycles){cycles--;}
}


/**********************************************************************************/
/*  Function name: Init_PLL                                                       */
/*  	Parameters                                                                */
/*          Input   :  PLL_Fclkin, PLL_Clock_Source, PLL_Fclkout                  */
/*          Output  :  No	                                                  */
/*	Action: Adjust PLL clock out       					  */
/**********************************************************************************/
void Init_PLL(unsigned long PLL_Fclkin, char PLL_Clock_Source, unsigned long PLL_Fclkout){
  unsigned long MSEL = 0x00000000;
  unsigned long PSEL = 0x00000000;
  
  // 1. Power Down PLL before initialization
  PDRUNCFG |= 0x00000080;       // Set SYSPLL_PD
  // 2. Select the System PLL clock source:
  SYSPLLCLKUEN &= ~0x00000001;  // No change 
  if(PLL_Clock_Source == 'I')     { SYSPLLCLKSEL = 0x00000000;  }     // IRC oscillator
  else if(PLL_Clock_Source == 'E'){ SYSPLLCLKSEL = 0x00000001;  }     // System oscillator
  else if(PLL_Clock_Source == 'W'){ SYSPLLCLKSEL = 0x00000002;  }     // WDT oscillator
  else                            { return;                     }     // Error in function call
  SYSPLLCLKUEN |= 0x00000001;   // Update clock source 
  // Definitions are changed! SYSPLLCLKUEN is really SYSPLLUEN in datasheet!
  // 3. Specify the PLL input clock frequency Fclkin.
  // PLL_Fclkin = 12000000MHz -> Internal RC oscillator (IRC), 12MHz/1% or Exernal Qrystall oscillator
  // 4. Boost system PLL to desired Fcco clock: 192MHz and then divide by M to obtain desired clock
  // FCCO - Frequency of the Current Controlled Oscillator (CCO); 156 to 320 MHz.
  SYSPLLCTRL |= 0x00000020; // Set PSEL to 01, i.e. 1 then Fcco = PLL_Fclkout * (2*PSEL) = 192MHz
  // PSEL = 192MHz / 2*PLL_Fclkout = 96MHz/PLL_Fclkout
  PSEL = 96000000/PLL_Fclkout;
  if(PSEL < 2)      { SYSPLLCTRL &= ~0x00000060; }  // PSEL = 00, P = 1
  else if(PSEL < 4) { SYSPLLCTRL |=  0x00000020; }  // PSEL = 01, P = 2
  else if(PSEL < 8) { SYSPLLCTRL |=  0x00000040; }  // PSEL = 10, P = 4
  else              { SYSPLLCTRL |=  0x00000060;  } // PSEL = 10, P = 8
  
  // 5. Calculate MSEL to obtain the desired output frequency Fclkout with (MSEL + 1) = Fclkout / Fclkin.
  MSEL = PLL_Fclkout/PLL_Fclkin;
  MSEL = ((MSEL & 0x000001F) - 1);
  // 6. Write MSEL
  SYSPLLCTRL |= MSEL;
  // 7. Enable the main system PLL by clearing bit 7 in PDRUNCFG
  PDRUNCFG &= ~0x00000080;       // Set SYSPLL_PD
  // 8. Then wait until the PLL clock is locked. SYSPLLSTAT bit LOCK
  while(!(SYSPLLSTAT & 0x000000001)){}  //If 1 -> PLL locked
}


/**********************************************************************************/
/*  Function name: InitSystemClock                                                */
/*  	Parameters                                                                */
/*          Input   :  desired_clock, clock_source                                */
/*          Output  :  No	                                                  */
/*	Action: Initialize PLL to desired System Clock value.                     */
/*              LPC13xx operate at CPU frequencies of up to 72 MHz!		  */
/**********************************************************************************/
void Init_System_Clock(unsigned long Desired_Sysem_Clock, char Clock_Source){
  // Is Desired_Sysem_Clock out of range?
  if(Desired_Sysem_Clock > 72000000){
    //  CPU max frequency is 72 MHz! 
    // So return to default Internal RC oscillator (IRC), 12MHz/1% 
    PDRUNCFG &= ~0x00000002;      // RC_PD = 0 -> IRC is Powered
    MAINCLKUEN &= ~0x00000001;    // No change
    MAINCLKSEL = 0x00000000;      // Main clock source select register -> IRC oscillator
    MAINCLKUEN |= 0x00000001;     // Update clock source
    return;
  } 
  // Internal or External Oscillator?  
  if(Clock_Source == 'I'){ 
    // Then Internal RC oscillator (IRC), 12MHz/1% is selected as  clock source:
    // 1.By default IRC is Powered but we will do this again
    PDRUNCFG &= ~0x00000002;      // RC_PD = 0 -> IRC is Powered
    // 2. Init PLL
    // PLL_Fclkin is Internal RC oscillator (IRC), 12MHz/1%
    Init_PLL(12000000, Clock_Source, Desired_Sysem_Clock);
  }
  else if(Clock_Source == 'E'){
    // Then System Oscillator(External crystall oscillator) is selected as  clock source:
    // 1. Init System Oscillator 
    PDRUNCFG |= 0x00000020;      // SYSOSC_PD = 1 -> System oscillator Powered Down
    SYSOSCCLTRL = 0x00000000;      // Oscillator is not bypassed, 1 - 20 MHz frequency range
    // Error in definitions! SYSOSCCLTRL is really SYSOSCCTRL in datasheet!
    PDRUNCFG &= ~0x00000020;      // SYSOSC_PD = 0 -> System oscillator is Powered
    // 2. Init PLL
    // PLL_Fclkin is External Crystal oscillator 12.000MHz
    Init_PLL(12000000, Clock_Source, Desired_Sysem_Clock);
  }
  else{
    // Exit, no correct source is selected
    // So return to default Internal RC oscillator (IRC), 12MHz/1% 
    PDRUNCFG &= ~0x00000002;      // RC_PD = 0 -> IRC is Powered
    MAINCLKUEN &= ~0x00000001;    // No change
    MAINCLKSEL = 0x00000000;      // Main clock source select register -> IRC oscillator
    MAINCLKUEN |= 0x00000001;     // Update clock source
    return;   
  }
  
  // 3. Set System Clock Devider
  SYSAHBCLKDIV |= 0x00000001;   // System Clock Divide by 1
  // 4. Set Main clock source select register MAINCLKSEL to PLL out
  // Main clock source update enable register bit<0> Must be toggled from LOW to HIGH for the update to take effect.
  MAINCLKUEN &= ~0x00000001;    // No change
  MAINCLKSEL = 0x00000003;      // Main clock source select register -> System PLL clock out
  MAINCLKUEN |= 0x00000001;     // Update clock source
  
}


/**********************************************************************************/
/*  Function name: init_devices	                                                  */
/*  	Parameters                                                                */
/*          Input   :  No			     		                  */
/*          Output  :  No                                      	            	  */
/*	Action: Initialize all used LPC1343 peripheral devices. 	  	  */
/**********************************************************************************/
void init_devices(void){
  // 1. Disable interrupts during initialization process
  __disable_interrupt();
  
  // 2.Select System Clock
  // By default settings are this:
  // main system clock -> from internal RC oscillator(IRC) 12MHz/1% accuracy, no dividers
  Init_System_Clock(72000000,'E');
  
  // 3.Ports initialization
  // LED<0..3>
  IOCON_PIO3_0 &= 0x0000000F; // Function PIO3_0; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_1 &= 0x0000000F; // Function PIO3_1; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_2 &= 0x0000000F; // Function PIO3_2; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_3 &= 0x0000000F; // Function PIO3_3; Disable pull-up/down; Disable Hysteresis
  GPIO3DATA |= 0x0000000F;    // PIO3_0 in state "1" if output
  GPIO3DIR |= 0x0000000F;     // GPIO data direction register: 1 -> Outpt, 0 -> Input
  // LED<4..7>
  IOCON_PIO2_4 &= 0x0000000F; // Function PIO2_4; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_5 &= 0x0000000F; // Function PIO2_5; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_6 &= 0x0000000F; // Function PIO2_6; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_7 &= 0x0000000F; // Function PIO2_7; Disable pull-up/down; Disable Hysteresis
  GPIO2DATA |= 0x000000F0;    // PIO2_0 in state "1" if output
  GPIO2DIR |= 0x000000F0;     // GPIO data direction register: 1 -> Outpt, 0 -> Input
  
  // 4.Peripherals initialization 
  // Not requred here

  // 5.Enable interrupts. Do this at the END of the initialization!!!!!!!!
  __enable_interrupt();
}

/**********************************************************************************/
/*  Function name: main	                                                  	  */
/*  	Parameters                                                                */
/*          Input   :  No			     		                  */
/*          Output  :  No                                      	            	  */
/*	Action: Manage all reqested tasks.                    	  		  */
/**********************************************************************************/
void main(void){
  init_devices();
  while(1){  
    LEDs_Running_Light('L', 1000000);
    //LEDs_Running_Light('R', 1000000);
  }
}


/**********************************************************************************/
/*  Function name: Write_vLEDs_Port                                               */
/*  	Parameters                                                                */
/*          Input   :  vLEDs_Port                                                 */
/*          Output  :  No	                                                  */
/*	Action: Write to virtual vLEDs_Port					  */
/**********************************************************************************/
void Write_LEDs_Port(unsigned long vLEDs_Port){
  unsigned long temp = 0;
  // Write LED<0..3>
  temp = (vLEDs_Port & 0x0000000F);       // Take LED<0..3> state
  temp = (GPIO3DATA & 0xFFFFFFF0) | temp; // Change only LED<0..3> state
  GPIO3DATA = temp;                       // Write LED<0..3>
  // Write LED<4..7>
  temp = (vLEDs_Port & 0x000000F0);       // Take LED<4..7> state
  temp = (GPIO2DATA & 0xFFFFFF0F) | temp; // Change only LED<4..7> state
  GPIO2DATA = temp;                       // Write LED<4..7>
}


/**********************************************************************************/
/*  Function name: LEDs_Running_Light                                             */
/*  	Parameters                                                                */
/*          Input   :  Direction                                                  */
/*          Output  :  No	                                                  */
/*	Action: Do Running_Light application over LED<0..7>     		  */
/**********************************************************************************/
void LEDs_Running_Light(unsigned char Direction, unsigned long Speed){
  unsigned long shift_counter = 0x000000FF;
  unsigned char i = 0;
 
  if(Direction == 'L'){
    // Left to Rigth, i.e. LED0 -> LED7
    shift_counter = 0x000000FF;
    for(i=0;i<16;i++){
      Write_LEDs_Port(shift_counter);
      if(i<8)       { shift_counter = shift_counter << 1; }
      else if(i==8) { shift_counter = shift_counter | 0x00000001;}
      else          { shift_counter = (shift_counter << 1) | 0x00000001; }
      Delay(Speed);
    }
  } 
  else if(Direction == 'R'){
    // Rigth to Left , i.e. LED7 -> LED0
    shift_counter = 0xFFFF00FF;
    for(i=0;i<16;i++){
      Write_LEDs_Port(shift_counter);
      shift_counter = shift_counter >> 1;
      Delay(Speed);
    }
  }
}


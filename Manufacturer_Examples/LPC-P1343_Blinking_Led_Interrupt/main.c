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
#define LED0_Chk  (GPIO3DATA & 0x00000001)
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

#define System_Clock  72000000


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


/*************************************************************************
 * Function Name: NVIC_IntEnable
 * Parameters: IntNumber - Interrup number
 * Return: void
 *
 * Description: Enable interrup at NVIC
 *
 *
 *************************************************************************/
void NVIC_IntEnable(unsigned long IntNumber)
{
volatile unsigned long * pNVIC_SetEn = &SETENA0;

  //assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0; // (IntNumber = NVIC_CT32B0 = 59) - (NVIC_WAKE_UP0 = 16) = 43(NVIC_WAKE_UP27)

  pNVIC_SetEn += IntNumber/32;
  *pNVIC_SetEn = (1UL<<(IntNumber%32));

}


/*************************************************************************
 * Function Name: NVIC_IntDisable
 * Parameters: IntNumber - Interrup number
 * Return: void
 *
 * Description: Disables interrup at NVIC
 *
 *
 *************************************************************************/
void NVIC_IntDisable(unsigned int IntNumber)
{
volatile unsigned long * pNVIC_ClrEn = &CLRENA0;

  //assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0;

  pNVIC_ClrEn += IntNumber/32;
  *pNVIC_ClrEn = (1UL<<(IntNumber%32));

}


/*************************************************************************
 * Function Name: NVIC_ClrPend
 * Parameters: IntNumber - Interrup number
 * Return: void
 *
 * Description:Clear pending interrupt at NVIC
 *
 *
 *************************************************************************/
void NVIC_ClrPend(unsigned int IntNumber)
{
volatile unsigned long * pNVIC_ClrPend = &CLRPEND0;

  //assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0;

  pNVIC_ClrPend += IntNumber/32;
  *pNVIC_ClrPend = (1UL<<(IntNumber%32));

}


/*************************************************************************
 * Function Name: NVIC_IntPri
 * Parameters: IntNumber - Interrup number, Interrupt Priority
 * Return: void
 *
 * Description:Sets Interrupt priority
 *
 *
 *************************************************************************/
void NVIC_IntPri(unsigned long IntNumber, unsigned int Priority)
{
//volatile Int8U * pNVIC_IntPri = (Int8U *)&IP0;
  volatile unsigned int * pNVIC_IntPri = (unsigned int *)&IP0;

  //assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0; // (IntNumber = NVIC_CT32B0 = 59) - (NVIC_WAKE_UP0 = 16) = 43(NVIC_WAKE_UP27)
  pNVIC_IntPri += IntNumber;
  *pNVIC_IntPri = Priority;
}


/**********************************************************************************/
/*  Function name: CT32B0_Init	                                                  */
/*  	Parameters                                                                */
/*          Input   :  Match_Level		     		                  */
/*          Output  :  No                                      	            	  */
/*	Action: Initialize CT32B0 timer.                         	  	  */
/**********************************************************************************/
void CT32B0_Init(unsigned long Match_Level_Divider)
{
  // Enable CT32B0 input clock
  SYSAHBCLKCTRL_bit.CT32B0 = 1; // AHBCLKCTRL is register name in datasheet
  // CT32B0 clock input is connected to #defined System_Clock
  // CT32B0 Operating Mode 
  TMR32B0TCR_bit.CE = 0;    // counting  disable
  TMR32B0TCR_bit.CR = 1;    // Counter Reset
  TMR32B0TCR_bit.CR = 0;    // release Counter Reset
  TMR32B0CTCR_bit.CTM = 0;  // Counter Timer Mode: every rising PCLK edge
  TMR32B0MCR_bit.MR0I = 1;  // Enable Interrupt on MR0
  TMR32B0MCR_bit.MR0R = 1;  // Enable reset on MR0
  TMR32B0MCR_bit.MR0S = 0;  // Disable stop on MR0
  // Set CT32B0 period
  TMR32B0PR = 0;            // Prescale Register (PR). Set devision ratio
  TMR32B0PC = 0;            // Prescale Counter (PC).
  TMR32B0MR0 = (System_Clock/(SYSAHBCLKDIV))/(Match_Level_Divider);
  // CT32B0 interrupt
  TMR32B0IR_bit.MR0INT = 1;  // clear pending interrupt, i.e. MR0 Interrupt flag
  TMR32B0TCR_bit.CE = 1;     // counting Enable
  // Enable NVIC TMR0 Interrupt
  NVIC_IntEnable(NVIC_CT32B0);
  NVIC_IntPri(NVIC_CT32B0,16);
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
  Init_System_Clock(System_Clock,'E');
  
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
  // 4.1. CT32B0
  CT32B0_Init(4);

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
    // Wait interrupt to occure
  }
}


/**********************************************************************************/
/*  Function name: CT32B0_IRQHandler                                              */
/*  	Parameters                                                                */
/*          Input   :  No                                                         */
/*          Output  :  No	                                                  */
/*	Action: Counter CT32B0 interrupt subroutine				  */
/**********************************************************************************/
void CT32B0_IRQHandler(void)
{
  // Toggle LED0
  if(LED0_Chk){ LED0_ON;  }
  else{         LED0_OFF; }
  // clear interrupt flag
  TMR32B0IR_bit.MR0INT = 1;
  /**/
  NVIC_ClrPend(NVIC_CT32B0);
}


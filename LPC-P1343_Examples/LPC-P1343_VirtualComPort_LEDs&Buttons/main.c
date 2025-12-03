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
#include "includes.h"
#include <yfuns.h>


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
#define USBC_ON   GPIO0DATA &= ~0x00000040;
#define USBC_OFF  GPIO0DATA |= 0x00000040;
#define USBC_Tog  GPIO0DATA ^= 0x00000040;
#define USBC_Chk  (GPIO0DATA & 0x00000040)
#define BUT1_Chk  (GPIO2DATA & 0x00000200)
#define BUT2_Chk  (GPIO1DATA & 0x00000010)

// Global variables
unsigned long delay_time = 1000000;
unsigned char Buttons_Scan_Counter = 0;
unsigned char Previous_Detected_Button = 0;
unsigned char Current_Detected_Button = 0;

Int8U BUT1_Command[30] =  "\n\r BUT1 is pressed!";
Int8U BUT2_Command[30] =  "\n\r BUT2 is pressed!";

/*variable for clitical section entry control*/
Int32U CriticalSecCntr;
volatile Int32U Ticks;
/** external data **/

//** internal functions **/

/** public functions **/
/*************************************************************************
 * Function Name: NVIC_IntEnable
 * Parameters: IntNumber - Interrup number
 * Return: void
 *
 * Description: Enable interrup at NVIC
 *
 *
 *************************************************************************/
void NVIC_IntEnable(Int32U IntNumber)
{
volatile unsigned long * pNVIC_SetEn = &SETENA0;

  assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0;

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
void NVIC_IntDisable(Int32U IntNumber)
{
volatile unsigned long * pNVIC_ClrEn = &CLRENA0;

  assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
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
void NVIC_ClrPend(Int32U IntNumber)
{
volatile unsigned long * pNVIC_ClrPend = &CLRPEND0;

  assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0;

  pNVIC_ClrPend += IntNumber/32;
  *pNVIC_ClrPend = (1UL<<(IntNumber%32));

}

/*************************************************************************
 * Function Name: NVIC_ClrPend
 * Parameters: IntNumber - Interrup number, Interrupt Priority
 * Return: void
 *
 * Description:Sets Interrupt priority
 *
 *
 *************************************************************************/
void NVIC_IntPri(Int32U IntNumber, Int8U Priority)
{
volatile Int8U * pNVIC_IntPri = (Int8U *)&IP0;

  assert((NVIC_WAKE_UP0 <= IntNumber) && (NVIC_PIO_0 >= IntNumber));
  IntNumber -= NVIC_WAKE_UP0;
  pNVIC_IntPri += IntNumber;
  *pNVIC_IntPri = Priority;
}

/*************************************************************************
 * Function Name: InitClock
 * Parameters: clock, AHB devider
 * Return: void
 *
 * Description: Initialize PLL to desired clock and  AHB devider
 *              Sys clock is Sys PLL output
 *              
 *
 *************************************************************************/
void InitClock(Int32U clock, Int32U ahbdiv)
{
  /*Sys Oscilator Enable*/
  SYSOSCCLTRL = (MAIN_OSC_FREQ>(20MHZ))?(0x2):(0x0);
  /*Power Up SYS Oscilator*/
  PDRUNCFG_bit.SYSOSC_PD = 0;
  /*Enable Internal RC oscilator*/
  PDRUNCFG_bit.IRC_PD = 0;
  /*Select internal RC oscilator for
    Sys clock source*/
  MAINCLKUEN = 0;
  MAINCLKSEL = 0;
  MAINCLKUEN = 1;
  /*Configure SYS PLL*/
  /*Power Down SYS PLL*/
  PDRUNCFG_bit.SYSPLL_PD = 1;
  /*Select Sys Oscilator for
    SYS PLL source*/
  SYSPLLCLKUEN = 0;
  SYSPLLCLKSEL = 1;
  SYSPLLCLKUEN = 1;
  /*Calc M*/
  Int32U m = clock/MAIN_OSC_FREQ - 1;
  
  assert(m<32);
  /*Configure PLL frequency*/
  SYSPLLCTRL =  (m)    /*MSEL*/
             |  (0<<5) /*PSEL = 1*/
             |  (0<<7) /*DIRECT = 0*/
             |  (0<<8); /*BYPASS=0*/
             
  /*Power Up PLL*/
  PDRUNCFG_bit.SYSPLL_PD = 0;
  /*Set Sys AHB Clock devider*/
  SYSAHBCLKDIV_bit.DIV = ahbdiv;
  /*Wain until PLL locks*/
  while(!(SYSPLLSTAT_bit.LOCK));
  /*Select Sys PLL Output for
    Sys clock source*/
  MAINCLKUEN = 0;
  MAINCLKSEL = 3;
  MAINCLKUEN = 1;
}

/*************************************************************************
 * Function Name: SYS_GetMainClk
 * Parameters: none
 * Return: Int32U
 *
 * Description: return Main Clock [Hz]
 *
 *************************************************************************/
Int32U SYS_GetMainClk(void)
{
Int32U Clk;
  switch(MAINCLKSEL_bit.SEL)
  {
    case 0:
       Clk = I_RC_OSC_FREQ;
    break;
    case 1:
      Clk = MAIN_OSC_FREQ;
    break;
    case 2:
      Clk = WDT_OSC_FREQ;
    break;
    case 3:
      switch(SYSPLLCLKSEL_bit.SEL)
      {
        case 0:
          Clk = I_RC_OSC_FREQ;
        break;
        case 1:
          Clk = MAIN_OSC_FREQ;
        break;
        case 2:
          Clk = WDT_OSC_FREQ;
        break;
      
        default:
          Clk = 0;
        break;  
      }
    
      if(!SYSPLLCTRL_bit.BYPASS)
        Clk *= (SYSPLLCTRL_bit.MSEL+1);
    break;
    
    default:
      Clk = 0;
    break;  
  }
  return Clk;
}

#if 0
/*************************************************************************
 * Function Name: __write
 * Parameters: Low Level cahracter output
 *
 * Return:
 *
 * Description:
 *
 *************************************************************************/
size_t __write(int Handle, const unsigned char * Buf, size_t Bufsize)
{
size_t nChars = 0;

  for (/*Empty */; Bufsize > 0; --Bufsize)
  {
    while( !U0LSR_bit.THRE );  //Wait
    U0THR = * Buf++;
    ++nChars;
  }
  return nChars;
}
/*************************************************************************
 * Function Name: __read
 * Parameters: Low Level cahracter input
 *
 * Return:
 *
 * Description:
 *
 *************************************************************************/
size_t __read(int handle, unsigned char * buffer, size_t size)
{
  int nChars = 0;

  /* This template only reads from "standard in", for all other file
   * handles it returns failure. */
  if (handle != _LLIO_STDIN)
  {
    return _LLIO_ERROR;
  }

  for (/* Empty */; size > 0; --size)
  {
    int c = MyLowLevelGetchar();
    if (c < 0)
      break;

    *buffer++ = c;
    ++nChars;
  }

  return nChars;
}
#endif
/*************************************************************************
 * Function Name: Dly100us
 * Parameters: void *arg
 * Return: void
 *
 * Description: Delay [100us]
 *		
 *************************************************************************/
void Dly100us(Int32U Dly)
{
volatile Int32U  Dly100;
  for(;Dly;Dly--)
    for(Dly100 = 550; Dly100; Dly100--);
}


/*************************************************************************
 * Function Name: GpioInit
 * Parameters: void
 * Return: void
 *
 * Description: Reset all GPIO pins to default: primary function
 *
 *************************************************************************/
void GpioInit(void)
{
  // Set to inputs
  GPIO0DIR = \
  GPIO1DIR = \
  GPIO2DIR = \
  GPIO3DIR = 0;

  // Reset all GPIO pins to default primary function
  IOCON_PIO2_6 = \
  IOCON_PIO2_0 = \
  IOCON_RESET_PIO0_0 = \
  IOCON_PIO0_1 = \
  IOCON_PIO1_8 = \
  IOCON_PIO0_2 = \
  IOCON_PIO2_7 = \
  IOCON_PIO2_8 = \
  IOCON_PIO2_1 = \
  IOCON_PIO0_3 = \
  IOCON_PIO1_9 = \
  IOCON_PIO2_4 = \
  IOCON_PIO2_5 = \
  IOCON_PIO0_6 = \
  IOCON_PIO0_7 = \
  IOCON_PIO2_9 = \
  IOCON_PIO2_10 = \
  IOCON_PIO2_2 = \
  IOCON_PIO0_8 = \
  IOCON_JTAG_TCK_PIO0_10 = \
  IOCON_PIO2_11 = \
  IOCON_PIO3_0 = \
  IOCON_PIO3_1 = \
  IOCON_PIO2_3 = \
  IOCON_PIO3_2 = \
  IOCON_PIO1_5 = \
  IOCON_PIO3_3 = 0x50;

  IOCON_PIO1_10 = \
  IOCON_JTAG_TDI_PIO0_11 = \
  IOCON_JTAG_TMS_PIO1_0 = \
  IOCON_JTAG_TDO_PIO1_1 = \
  IOCON_JTAG_nTRST_PIO1_2 = \
  IOCON_SWD_PIO1_3 = \
  IOCON_PIO1_4 = \
  IOCON_PIO1_11 = 0xD0;
  
  IOCON_PIO0_4 = \
  IOCON_PIO0_5 = \
  IOCON_SCKLOC = 0x00;
  
  // LED<0..3>
  IOCON_PIO3_0 &= 0x0000003F; // Function PIO3_0; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_1 &= 0x0000003F; // Function PIO3_1; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_2 &= 0x0000003F; // Function PIO3_2; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO3_3 &= 0x0000003F; // Function PIO3_3; Disable pull-up/down; Disable Hysteresis
  GPIO3DATA |= 0x0000000F;    // PIO3_0 in state "1" if output
  GPIO3DIR |= 0x0000000F;     // GPIO data direction register: 1 -> Outpt, 0 -> Input
  // LED<4..7>
  IOCON_PIO2_4 &= 0x0000003F; // Function PIO2_4; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_5 &= 0x0000003F; // Function PIO2_5; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_6 &= 0x0000003F; // Function PIO2_6; Disable pull-up/down; Disable Hysteresis
  IOCON_PIO2_7 &= 0x0000003F; // Function PIO2_7; Disable pull-up/down; Disable Hysteresis
  GPIO2DATA |= 0x000000F0;    // PIO2_0 in state "1" if output
  GPIO2DIR |= 0x000000F0;     // GPIO data direction register: 1 -> Outpt, 0 -> Input
  
}
/*************************************************************************
 * Function Name: TMR0_IRQHandler
 * Parameters: none
 *
 * Return: none
 *
 * Description: Timer 0 interrupt handler
 *
 *************************************************************************/
void CT32B0_IRQHandler (void)
{
  ++Ticks;
  // clear interrupt
  TMR32B0IR_bit.MR0INT = 1;
  /**/
  NVIC_ClrPend(NVIC_CT32B0);
}

void CT32B0_Init(Int32U tps)
{
  Ticks = 0;
  // Enable TIM0 clocks
  SYSAHBCLKCTRL_bit.CT32B0 = 1;

  // Init Time0
  TMR32B0TCR_bit.CE = 0;     // counting  disable
  TMR32B0TCR_bit.CR = 1;     // set reset
  TMR32B0TCR_bit.CR = 0;     // release reset
  TMR32B0CTCR_bit.CTM = 0;   // Timer Mode: every rising PCLK edge
  TMR32B0MCR_bit.MR0I = 1;   // Enable Interrupt on MR0
  TMR32B0MCR_bit.MR0R = 1;   // Enable reset on MR0
  TMR32B0MCR_bit.MR0S = 0;   // Disable stop on MR0
  // set timer 0 period
  TMR32B0PR = 0;
  TMR32B0MR0 = (SYS_GetMainClk()/(SYSAHBCLKDIV))/(tps);
  // init timer 0 interrupt
  TMR32B0IR_bit.MR0INT = 1;  // clear pending interrupt
  TMR32B0TCR_bit.CE = 1;     // counting Enable
   /*Enable NVIC TMR0 Interrupt*/
  NVIC_IntEnable(NVIC_CT32B0);
  NVIC_IntPri(NVIC_CT32B0,16);
}

void CT32B0_Stop(void)
{
  TMR32B0TCR_bit.CE = 0;     // counting  disable

  NVIC_IntDisable(NVIC_CT32B0);

  SYSAHBCLKCTRL_bit.CT32B0 = 0; // disable clock
}


/**********************************************************************************/
/*  Function name: CT16B0_Init	                                                  */
/*  	Parameters                                                                */
/*          Input   :  Match_Level		     		                  */
/*          Output  :  No                                      	            	  */
/*	Action: Initialize CT16B0 timer.                         	  	  */
/**********************************************************************************/
void CT16B0_Init(unsigned long Match_Level)
{
  // Enable CT16B0 input clock
  SYSAHBCLKCTRL_bit.CT16B0 = 1; // AHBCLKCTRL is register name in datasheet
  // CT16B0 clock input is connected to #defined System_Clock
  // CT16B0 Operating Mode 
  TMR16B0TCR_bit.CE = 0;    // counting  disable
  TMR16B0TCR_bit.CR = 1;    // Counter Reset
  TMR16B0TCR_bit.CR = 0;    // release Counter Reset
  TMR16B0CTCR_bit.CTM = 0;  // Counter Timer Mode: every rising PCLK edge
  TMR16B0MCR_bit.MR0I = 1;  // Enable Interrupt on MR0
  TMR16B0MCR_bit.MR0R = 1;  // Enable reset on MR0
  TMR16B0MCR_bit.MR0S = 0;  // Disable stop on MR0
  // Set CT16B0 period
  TMR16B0PR = 16;           // Prescale Register (PR). Set devision ratio
  TMR16B0PC = 0;            // Prescale Counter (PC).
  TMR16B0MR0 = Match_Level;
  // CT16B0 interrupt
  TMR16B0IR_bit.MR0INT = 1;  // clear pending interrupt, i.e. MR0 Interrupt flag
  TMR16B0TCR_bit.CE = 1;     // counting Enable
  // Enable NVIC TMR0 Interrupt
  NVIC_IntEnable(NVIC_CT16B0);
  NVIC_IntPri(NVIC_CT16B0,15);
}


#define MOUSE_DELTA           1
/*************************************************************************
 * Function Name: main
 * Parameters: none
 *
 * Return: none
 *
 * Description: main
 *
 *************************************************************************/
int main(void)
{
Int8U Buffer[100];
Int8U Command_Buffer[6];
Int8U Error_Command[50] = "\n\r Error! Requested Command is not valid! \n\r";
Int8U Ok_Command[50] =    "\n\r Ok! Command is executed!\n\r";
Int8U Initial_Menu[] =    "\f\r***************************************************************\n\r*** Welcom to OLIMEX LPC-P1343_VirtualComPort demo program! ***\n\r***************************************************************\n\r*  Valid Commands are: LEDx_0 or LEDx_1, wher x=0..7;         *\n\r*    Please Send Command or press any board's button!         *\n\r***************************************************************\n\r";

//pInt8U pBuffer;
Int32U Size;
unsigned char char_counter = 0;
unsigned char USB_First_Connection_Detected = 1;
//Int32U TranSize;

Boolean CdcConfigureStateHold;

#if CDC_DEVICE_SUPPORT_LINE_CODING > 0
//CDC_LineCoding_t CDC_LineCoding;
//UartLineCoding_t UartLineCoding;
#endif // CDC_DEVICE_SUPPORT_LINE_CODING > 0

#if CDC_DEVICE_SUPPORT_LINE_STATE > 0

UartLineEvents_t      UartLineEvents;

SerialState_t   SerialState;
#endif // CDC_DEVICE_SUPPORT_LINE_STATE > 0

  /*Init clock*/
  InitClock(132MHZ, 2);
  /*Enable GPIO Clock*/
  SYSAHBCLKCTRL_bit.GPIO = 1;
  /*Enable IOCON Clock*/
  SYSAHBCLKCTRL_bit.IOCON = 1;
  /*Set all pins as input ports*/
  GpioInit();
   /*Init LED Ports*/
  LED1_DIR |= LED1_MASK;
  LED2_DIR |= LED2_MASK;
  /*Turn off LED1*/
  LED_OFF(LED1_PORT);
  /*Turn off LED2*/
  LED_OFF(LED2_PORT);
  CT16B0_Init(0x5FFF);

  
  // Init USB
  UsbCdcInit();

  __enable_interrupt();
  // Soft connection enable
  USB_ConnectRes(TRUE);
  
  CdcConfigureStateHold = !IsUsbCdcConfigure();
  
  while(1)
  {
  Start:
    //Command_Buffer
    if (IsUsbCdcConfigure())
    {
      //Read Data from USB
      Size = UsbCdcRead(Buffer,sizeof(Buffer)-1);
      if(Size)
      {
        if(USB_First_Connection_Detected){
          while(!UsbCdcWrite(Initial_Menu,393));  // Send Start Menue
          USB_First_Connection_Detected = 0;
          goto Start;
        }
        // If there is received character:
        Command_Buffer[char_counter] = Buffer[0];
        char_counter++;
        if(char_counter > 5){
          char_counter = 0;
          // Check for valid command
          if((Command_Buffer[0] != 'L') | (Command_Buffer[1] != 'E') | (Command_Buffer[2] != 'D')| (Command_Buffer[4] != '_')){
            // Then command is not valid! Send error message to USB!
            while(!UsbCdcWrite(Error_Command,50));
          }
          else{
            // If it is valid, then check LED number and desired state
            switch(Command_Buffer[3]){
            case '0':
              if(Command_Buffer[5] == '1'){ LED0_ON;}
              else                        { LED0_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '1':
              if(Command_Buffer[5] == '1'){ LED1_ON;}
              else                        { LED1_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '2':
              if(Command_Buffer[5] == '1'){ LED2_ON;}
              else                        { LED2_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '3':
              if(Command_Buffer[5] == '1'){ LED3_ON;}
              else                        { LED3_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '4':
              if(Command_Buffer[5] == '1'){ LED4_ON;}
              else                        { LED4_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '5':
              if(Command_Buffer[5] == '1'){ LED5_ON;}
              else                        { LED5_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '6':
              if(Command_Buffer[5] == '1'){ LED6_ON;}
              else                        { LED6_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            case '7':
              if(Command_Buffer[5] == '1'){ LED7_ON;}
              else                        { LED7_OFF;}
              while(!UsbCdcWrite(Ok_Command,50)); 
              break;
            default:
              while(!UsbCdcWrite(Error_Command,50));
              break;
            }
          }  
        }
          
        // Data to USB
        //while(!UsbCdcWrite(Buffer,Size));
      }
    }
  } 
}


/**********************************************************************************/
/*  Function name: CT16B0_IRQHandler                                              */
/*  	Parameters                                                                */
/*          Input   :  No                                                         */
/*          Output  :  No	                                                  */
/*	Action: Counter CT16B0 interrupt subroutine				  */
/**********************************************************************************/
void CT16B0_IRQHandler(void)
{ 
  // Via buttons scanning we escape Delay(10000); time delay,
  // which is required because of high Core frequency!
  // If delay is not used Buttons can not be recognized correctly! 
  // So this is clever method to find when button is pressed!
  
    // Read Current Button State
  if(!BUT1_Chk)     {Current_Detected_Button = 1;} // BUT1 is pressed
  else if(!BUT2_Chk){Current_Detected_Button = 2;} // BUT2 is pressed
  else              {Current_Detected_Button = 0; Buttons_Scan_Counter = 0;}  // Not found pressed Button
  // Recognize which is pressed
  if(Current_Detected_Button != 0){
    if(Current_Detected_Button != Previous_Detected_Button){
      // Button first detection
      Previous_Detected_Button = Current_Detected_Button;
      Buttons_Scan_Counter = 0;
    }
    else{
      // Button next detection
      Buttons_Scan_Counter++;
      if(Buttons_Scan_Counter == 20){
        Buttons_Scan_Counter = 0;
        // Then Current_Detected_Button is really pressed
        switch(Current_Detected_Button ){
        case 1: // BUT1 is pressed
          // Send message
          while(!UsbCdcWrite(BUT1_Command,30));
          Current_Detected_Button = 0;
          break;
        case 2: // BUT2 is pressed
           // Send message
          while(!UsbCdcWrite(BUT2_Command,30));
          Current_Detected_Button = 0;
          break;
        }
      } 
    }
  }
  // clear interrupt flag
  TMR16B0IR_bit.MR0INT = 1;
  NVIC_ClrPend(NVIC_CT16B0); // Some peripherals must clear and NVIC flag! 
}


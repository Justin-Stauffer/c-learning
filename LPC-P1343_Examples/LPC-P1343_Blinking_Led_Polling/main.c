/**********************************************************************************/
/*    Demo program for:								  */
/*	  Board: LPC-P1343              					  */
/*    Manufacture: OLIMEX                                                   	  */
/*	  COPYRIGHT (C) 2010							  */
/*    Designed by: Engineer Penko T. Bozhkov                                      */
/*    Module Name    :  main module                                               */
/*    File   Name    :  main.c                                                    */
/*    Revision       :  initial                                                   */
/*    Date           :  21.04.2010                                                */
/*    Built with IAR Embedded Workbench Version: 5.41                             */
/**********************************************************************************/
#include  <nxp/iolpc1343.h>
#include  <intrinsics.h>

// Functions Prototypes:
void Delay(volatile unsigned long cycles);
void init_devices(void);
void main(void);

// Definitions:
#define LED0_ON   GPIO3DATA &= ~0x00000001;  
#define LED0_OFF  GPIO3DATA |= 0x00000001;


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
  
  // 3.Ports initialization
  IOCON_PIO3_0 &= 0x0000000F; // Function PIO3_0; Disable pull-up/down; Disable Hysteresis
  GPIO3DATA |= 0x00000001;    // PIO3_0 in state "1" if output
  GPIO3DIR |= 0x00000001;     // GPIO data direction register: 1 -> Outpt, 0 -> Input
  
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
    LED0_ON;
    Delay(1000000);
    LED0_OFF;
    Delay(1000000);
  }
}



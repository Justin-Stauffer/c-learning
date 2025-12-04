########################################################################
#
#                           VirtualCom.eww
#
# $Revision: 31547 $
#
########################################################################

DESCRIPTION
===========
  This example project shows how to use the IAR Embedded Workbench for ARM
 to develop code for IAR-LPC1768-SK board.
   It implements USB CDC (Communication Device Class) device and install
 it like a Virtual COM port. The UART1 is used for physical implementation
 of the RS232 port.


COMPATIBILITY
=============
   The example project is compatible with IAR-LPC1768-SK evaluation board.
   By default, the project is configured to use the J-Link JTAG interface.

CONFIGURATION
=============

   After power-up the controller get clock from internal RC oscillator that
  is unstable and may fail with J-Link auto detect. tIn this case adaptive 
  clocking should be used. The adaptive clock can be select from menu:
  Project->Options..., section Debugger->J-Link/J-Trace  JTAG Speed - Adaptive.

   Make sure that the following jumpers are correctly configured on the
  IAR-LPC1768-SK evaluation board:

  Jumpers:
   PWR_SEL     - depending of power source
   USBD-       - USB Device (1-2)  
   USBD+       - USB Device (1-2)  
   RST_E       - unfilled
   ISP_E       - unfilled

GETTING STARTED
===============

  1) Start the IAR Embedded Workbench for ARM.

  2) Select File->Open->Workspace...
     Open the following workspace:

     <installation-root>\arm\examples\NXP\
     LPC17xx\IAR-LPC-1768-SK\VirtualCom\VirtualCom.eww

  3) Connect a serial cable between a Host COM port and the IAR LPC-1768-SK
     RS232_1 connector and start a terminal emulator program on the Host.
     Configure the PC COM port are same like sittings of the Virual COM port.

  3) Run the program.

  4) The first time the device is connected to the computer, Windows will
     load the driver for identified device. The Virtual COM port driver is
     in the $PROJ_DIR$\VirCOM Driver XP\.

/*************************************************************************
 *
 *    Used with ICCARM and AARM.
 *
 *    File name   : usb_hw.c
 *    Description : usb module (HAL)
 *
 *    History :
 *    1. Date        : April 8, 2008
 *       Author      : Stanimir Bonev
 *       Description : Create
 *        USB framework rev. 2 light version with DMA transfer support
 *
 *    $Revision: 33388 $
 **************************************************************************/
#define USB_HW_GLOBAL
#include "arm_comm.h"
#include "usb_hw.h"
#include "board.h"

extern void NVIC_IntEnable(Int32U IntNumber);
extern void NVIC_IntDisable(Int32U IntNumber);
extern void NVIC_ClrPend(Int32U IntNumber);
extern void NVIC_IntPri(Int32U IntNumber, Int8U Priority);
extern Int32U SYS_GetMainClk(void);

static volatile UsbDevStat_t USB_DevStatus;

static const UsbStandardEpDescriptor_t USB_CtrlEpDescr0 =
{
  sizeof(UsbStandardEpDescriptor_t),
  UsbDescriptorEp,
  UsbEpOut(CTRL_ENP_OUT>>1),
  {(Int8U)UsbEpTransferControl | (Int8U)UsbEpSynchNoSynchronization | (Int8U)UsbEpUsageData},
  Ep0MaxSize,
  0
};

static const UsbEP_ExtData_t USB_CtrlEpExtDescr0 =
{
  0
};

static const UsbStandardEpDescriptor_t USB_CtrlEpDescr1 =
{
  sizeof(UsbStandardEpDescriptor_t),
  UsbDescriptorEp,
  UsbEpIn(CTRL_ENP_IN>>1),
  {(Int8U)UsbEpTransferControl | (Int8U)UsbEpSynchNoSynchronization | (Int8U)UsbEpUsageData},
  Ep0MaxSize,
  0
};

static const UsbEP_ExtData_t USB_CtrlEpExtDescr1 =
{
  0
};

static const Boolean UsbEpDoubleBuffType[] =
{
  FALSE,  // OUT 0
  FALSE,  // IN 0
  FALSE,  // OUT 1
  FALSE,  // IN 1
  FALSE,  // OUT 2
  FALSE,  // IN 2
  TRUE,   // OUT 3
  TRUE,   // IN 3
  TRUE,  // OUT 4
  TRUE,  // IN 4
};

#if  USB_SOF_EVENT > 0
Int32U  USB_SofNumbHold;
#endif

#if USB_DMA_DD_MAX_NUMB > 0

#pragma segment="USB_DMA_RAM"
#pragma location="USB_DMA_RAM"
#pragma data_alignment=128
__no_init pUSB_DmaDesc_t USB_DDCA[ENP_MAX_NUMB];

#pragma location="USB_DMA_RAM"
__no_init USB_DmaDesc_t USB_DmaDesc[USB_DMA_DD_MAX_NUMB];
pUSB_DmaDesc_t DmaFree[USB_DMA_DD_MAX_NUMB];

#if USB_DMA_ID_MAX_NUMB > 0
#pragma location="USB_DMA_RAM"
__no_init DmaIsoPacket_t USB_DmaIsoDesc[USB_DMA_ID_MAX_NUMB];
pDmaIsoPacket_t DmaIsoFree[USB_DMA_ID_MAX_NUMB];
#endif // USB_DMA_ID_MAX_NUMB > 0
#endif // USB_DMA_DD_MAX_NUMB > 0

/*************************************************************************
 * Function Name: USB_Cmd
 * Parameters:  Int16U Command, Int8U Data
 *
 * Return: Int32U - command result
 *
 * Description: Implement commands transmit to USB Engine
 *
 *************************************************************************/
static
Int32U USB_Cmd (Int16U Command, Int8U Data)
{
#if __CORE__ < 7
Int32U Save;
#endif // __CORE__ < 7
Int32U tmp = 0;
  // Disable interrupt and save current state of the interrupt flags
  #if __CORE__ < 7
    ENTR_CRT_SECTION(Save);
  #else
    ENTR_CRT_SECTION();
  #endif // __CORE__ < 7
  USBDevIntClr = bmUSB_CommDataFullInterrupt | bmUSB_CommRegEmptyInterrupt;
  // Load command in USB engine
  USBCmdCode = ((Command&0xFF) << 16) + USB_CMD_WR;
  // Wait until command is accepted
  while ((USBDevIntSt & bmUSB_CommRegEmptyInterrupt) == 0);
  // clear Command reg. empty interrupt
  USBDevIntClr = bmUSB_CommRegEmptyInterrupt;
  // determinate next phase of the command
  switch (Command)
  {
  case CMD_USB_SET_ADDRESS:
  case CMD_USB_CFG_DEV:
  case CMD_USB_SET_MODE:
  case CMD_USB_SET_DEV_STAT:
    USBCmdCode = (Data << 16) + USB_DATA_WR;
    while ((USBDevIntSt & bmUSB_CommRegEmptyInterrupt) == 0);
    break;
  case CMD_USB_RD_FRAME_NUMB:
  case CMD_USB_RD_TEST_REG:
    USBCmdCode = (Command << 16) + USB_DATA_RD;
    while ((USBDevIntSt & bmUSB_CommDataFullInterrupt) == 0);
    USBDevIntClr = bmUSB_CommDataFullInterrupt;
    tmp = USBCmdData;
    USBCmdCode = (Command << 16) + USB_DATA_RD;
    while ((USBDevIntSt & bmUSB_CommDataFullInterrupt) == 0);
    tmp |= USBCmdData << 8;
    break;
  case CMD_USB_GET_DEV_STAT:
  case CMD_USB_GET_ERROR:
  case CMD_USB_RD_ERROR_STAT:
  case CMD_USB_CLR_BUF:
    USBCmdCode = (Command << 16) + USB_DATA_RD;
    while ((USBDevIntSt & bmUSB_CommDataFullInterrupt) == 0);
    tmp = USBCmdData;
    break;
  default:
    switch (Command & 0x1E0)
    {
    case CMD_USB_SEL_EP:
    case CMD_USB_SEL_CLR_INT_EP:
      USBCmdCode = (Command << 16) + USB_DATA_RD;
      while ((USBDevIntSt & bmUSB_CommDataFullInterrupt) == 0);
      tmp = USBCmdData;
      break;
    case CMD_USB_SET_EP_STAT:
      USBCmdCode = (Data << 16) + USB_DATA_WR;
      while ((USBDevIntSt & bmUSB_CommRegEmptyInterrupt) == 0);
      break;
    }
    break;
  }
  // restore the interrupt flags
  #if __CORE__ < 7
    EXT_CRT_SECTION(Save);
  #else
    EXT_CRT_SECTION();
  #endif // __CORE__ < 7
  return(tmp);
}

/*************************************************************************
 * Function Name: USB_EpIntrClr
 * Parameters: USB_Endpoint_t EndPoint
 *
 * Return: Int8U
 *
 * Description: Clear the EP interrupt flag and return the current EP status
 *
 *************************************************************************/
static
Int8U USB_EpIntrClr(USB_Endpoint_t EndPoint)
{
volatile Int32U TO = 100;
Int8U Tmp;
#if __CORE__ < 7
Int32U Save;
#endif // __CORE__ < 7
  // Disable interrupt and save current state of the interrupt flags
  #if __CORE__ < 7
    ENTR_CRT_SECTION(Save);
  #else
    ENTR_CRT_SECTION();
  #endif // __CORE__ < 7

  Tmp = USB_Cmd(CMD_USB_SEL_CLR_INT_EP | EndPoint,0);
  USBDevIntClr = 1 << (EndPoint + 1);

  #if __CORE__ < 7
    EXT_CRT_SECTION(Save);
  #else
    EXT_CRT_SECTION();
  #endif // __CORE__ < 7
  return(Tmp);
}

/*************************************************************************
 * Function Name: USB_HwInit
 * Parameters: none
 *
 * Return: none
 *
 * Description: Init USB
 *
 *************************************************************************/
void USB_HwInit(void)
{
  
  // Init SOF number hold
  #if  USB_SOF_EVENT > 0
  USB_SofNumbHold = 0;
  #endif

  /*Init USB clock. Use USBPLL as USP clock source*/
  
  /*Enable USB PLL*/
  PDRUNCFG_bit.USBPLL_PD = 1;
  
  /*USB Devider*/
  USBCLKDIV = 1;
  /*Select Sys Oscilator for
    USB PLL source*/
  USBPLLCLKUEN = 0;
  USBPLLCLKSEL = 1;
  USBPLLCLKUEN = 1;
  /*Calc M*/
  Int32U m = 48MHZ/MAIN_OSC_FREQ - 1;
  
  assert(m<32);
  /*Configure PLL frequency*/
  USBPLLCTRL =  (m)    /*MSEL*/
             |  (0<<5) /*PSEL = 1*/
             |  (0<<7) /*DIRECT = 0*/
             |  (0<<8); /*BYPASS=0*/
             
  /*Power Up PLL*/
  PDRUNCFG_bit.USBPLL_PD = 0;
  /*Wain until PLL locks*/
  while(!(USBPLLSTAT_bit.LOCK));

  /*Select Main Clock as USB Clock soruce (USB PLL not used)*/
  USBCLKUEN = 0;
  USBCLKSEL = 0;
  USBCLKUEN = 1;
  
  /*Enable USB PCLK*/
  SYSAHBCLKCTRL_bit.USB_REG = 1;
  
  /*Power up USB PHY*/
  PDRUNCFG_bit.USBPAD_PD = 0;
  
  /*USB IO assign*/
  
  /*Select Connect and Vbus pin function
    and disable Pull Up/Down resistors*/
  IOCON_PIO0_3 = 1;
  IOCON_PIO0_6 = 1;

  // Disconnect device
  USB_ConnectRes(FALSE);

  // enable suspend mode AP_CLK = 0
  USB_Cmd(CMD_USB_SET_MODE,0);    // init to default value

  // Set address 0
  USB_SetDefAdd();
  // clear all pending interrupts
  USBDevIntClr = 0xFFFFFFFF;
  /*All Interrupts routed to the IRQ*/
  USBDevFIQSel = 0;
  // Init controls endpoints
  USB_HwReset();
  // Init Device status
  UsbSetDevState(UsbDevStatusUnknow);
  // Init Device state var
  USB_DevStatus.Data = USB_Cmd(CMD_USB_GET_DEV_STAT,0);

  // Enable USB interrupts
  // USB interrupt connect to VIC
  // USB interrupt enable
  NVIC_IntEnable(NVIC_USB_IRQ);
  NVIC_IntPri(NVIC_USB_IRQ,USB_INTR_PRIORITY);
}

/*************************************************************************
 * Function Name: USB_HwReset
 * Parameters: none
 *
 * Return: none
 *
 * Description: Reset USB engine
 *
 *************************************************************************/
void USB_HwReset (void)
{
  // Disable all endpoint interrupts
  USBDevIntEn = 0;
  // USB_Configure
  USB_Configure(FALSE);
  // Control EP Init
  USB_RealizeEp(&USB_CtrlEpDescr0,&USB_CtrlEpExtDescr0,TRUE);
  USB_RealizeEp(&USB_CtrlEpDescr1,&USB_CtrlEpExtDescr1,TRUE);


  // Enable Device interrupts
  USBDevIntEn |=  bmUSB_DevStatusInterrupt |(USB_SOF_EVENT ? bmUSB_FrameInterrupt : 0);
}

/*************************************************************************
 * Function Name: USB_RealizeEp
 * Parameters: const UsbStandardEpDescriptor_t * pEP_Desc,
 *             const UsbEP_ExtData_t * pUsbEP_ExtData,
 *             Boolean Enable
 *
 * Return: USB_ErrorCodes_t
 *
 * Description: Enable or disable an endpoint
 *
 *************************************************************************/
USB_ErrorCodes_t USB_RealizeEp(const UsbStandardEpDescriptor_t * pEP_Desc,
                               const UsbEP_ExtData_t * pUsbEP_ExtData,
                               Boolean Enable)
{
USB_Endpoint_t EP = (USB_Endpoint_t)USB_EpLogToPhysAdd(pEP_Desc->bEndpointAddress);
Int32U Mask = (1 << (EP + 1));

  if (Enable)
  {
    // Set EP status
    EpCnfg[EP].Status  = NOT_READY;
    // Init EP flags
    EpCnfg[EP].Flags = 0;
    EpCnfg[EP].bDMA_Transfer  = pUsbEP_ExtData->DMA_Transfer;
    EpCnfg[EP].bDoubleBuffered = UsbEpDoubleBuffType[EP];
    // Set endpoint type
    EpCnfg[EP].EpType = (UsbEpTransferType_t)pEP_Desc->bmAttributes.TransferType;
    // Init EP max packet size
    EpCnfg[EP].MaxSize = pEP_Desc->wMaxPacketSize;

    if (EP & 1)
    {
      EpCnfg[EP].AvbBuff = EpCnfg[EP].bDoubleBuffered + 1;
    }
    else
    {
      EpCnfg[EP].AvbBuff = 0;
    }

    if(0 == EpCnfg[EP].bDMA_Transfer)
    {
      // Enable endpoint interrupt
      USBDevIntEn |= Mask;
    }
    
    USB_Cmd(CMD_USB_SET_EP_STAT,0x00);
  }
  else
  {
    USBDevIntEn &= ~Mask;
    EpCnfg[EP].MaxSize = 0;
    USB_Cmd(CMD_USB_SET_EP_STAT,0x00);
  }
  return(USB_OK);
}

/*************************************************************************
 * Function Name: USB_SetAdd
 * Parameters: Int32U DevAdd - device address between 0 - 127
 *
 * Return: none
 *
 * Description: Set device address
 *
 *************************************************************************/
void USB_SetAdd(Int32U DevAdd)
{
  USB_Cmd(CMD_USB_SET_ADDRESS,DevAdd | 0x80);
  USB_Cmd(CMD_USB_SET_ADDRESS,DevAdd | 0x80);
}

/*************************************************************************
 * Function Name: USB_ConnectRes
 * Parameters: Boolean Conn
 *
 * Return: none
 *
 * Description: Connect USB
 *
 *************************************************************************/
void USB_ConnectRes (Boolean Conn)
{
  USB_Cmd(CMD_USB_SET_DEV_STAT, (Conn ? bmUSB_Connect : 0));
}

/*************************************************************************
 * Function Name: USB_Configure
 * Parameters: Boolean Configure
 *
 * Return: none
 *
 * Description: Configure device
 *              When Configure != 0 enable all Realize Ep
 *
 *************************************************************************/
void USB_Configure (Boolean Configure)
{
  USB_Cmd(CMD_USB_CFG_DEV,Configure);
}

#if USB_REMOTE_WAKEUP != 0
/*************************************************************************
 * Function Name: USB_WakeUp
 * Parameters: none
 *
 * Return: none
 *
 * Description: Wake up USB
 *
 *************************************************************************/
void USB_WakeUp (void)
{
  /*Enable USB PCLK*/
  SYSAHBCLKCTRL_bit.USB_REG = 1;
  /*Power up USB PHY*/
  PDRUNCFG_bit.USBPAD_PD = 0;
  for( int i = 1000 ; i ; i--);

  USB_Cmd(CMD_USB_SET_DEV_STAT,bmUSB_Connect);
}
#endif // USB_REMOTE_WAKEUP != 0

/*************************************************************************
 * Function Name: USB_GetDevStatus
 * Parameters: USB_DevStatusReqType_t Type
 *
 * Return: Boolean
 *
 * Description: Return USB device status
 *
 *************************************************************************/
Boolean USB_GetDevStatus (USB_DevStatusReqType_t Type)
{
  switch (Type)
  {
  case USB_DevConnectStatus:
    return(USB_DevStatus.Connect);
  case USB_SuspendStatus:
    return(USB_DevStatus.Suspend);
  case USB_ResetStatus:
    return(USB_DevStatus.Reset);
  }
  return(FALSE);
}

/*************************************************************************
 * Function Name: USB_SetStallEP
 * Parameters: USB_Endpoint_t EndPoint, Boolean Stall
 *
 * Return: none
 *
 * Description: The endpoint stall/unstall
 *
 *************************************************************************/
void USB_SetStallEP (USB_Endpoint_t EP, Boolean Stall)
{
Boolean CurrStallStatus;

  USB_GetStallEP(EP,&CurrStallStatus);
  if(CurrStallStatus != Stall)
  {
    if ((EP & 1) && !Stall)
    {
      EpCnfg[EP].AvbBuff = EpCnfg[EP].bDoubleBuffered+1;
    }
    else
    {
      EpCnfg[EP].AvbBuff = 0;
    }
    USB_Cmd(CMD_USB_SET_EP_STAT | EP, (Stall ? bmUSB_EpStall : 0));
  }
}

/*************************************************************************
 * Function Name: USB_StallCtrlEP
 * Parameters: none
 *
 * Return: none
 *
 * Description: Stall both direction of the CTRL EP
 *
 *************************************************************************/
void USB_StallCtrlEP (void)
{
  EpCnfg[CTRL_ENP_OUT].AvbBuff = 0;
  EpCnfg[CTRL_ENP_IN].AvbBuff  = EpCnfg[CTRL_ENP_IN].bDoubleBuffered + 1;
  USB_Cmd(CMD_USB_SET_EP_STAT | CTRL_ENP_OUT, bmUSB_EpCondStall);
}

/*************************************************************************
 * Function Name: USB_GetStallEP
 * Parameters: USB_Endpoint_t EndPoint, pBoolean pStall
 *
 * Return: none
 *
 * Description: Get stall state of the endpoint
 *
 *************************************************************************/
void USB_GetStallEP (USB_Endpoint_t EndPoint, pBoolean pStall)
{
  *pStall = (USB_Cmd(CMD_USB_SEL_EP | EndPoint, 0) & bmUSB_EpStallStatus) != 0;
}

/*************************************************************************
 * Function Name: USB_EP_IO
 * Parameters: USB_Endpoint_t EndPoint
 *
 * Return: none
 *
 * Description: Endpoint Write (IN)
 *
 *************************************************************************/
void USB_EP_IO(USB_Endpoint_t EP)
{
Int32U Data, Count;

  if(EpCnfg[EP].Status != BEGIN_SERVICED &&
     EpCnfg[EP].Status != NO_SERVICED)
  {
    volatile int dummu = 0;
    return;
  }
  if(EP & 1)
  {
      Count = EpCnfg[EP].Size - EpCnfg[EP].Offset;

      while(EpCnfg[EP].AvbBuff)
      {
        if(Count == 0 && !EpCnfg[EP].bZeroPacket)
        {
          USB_Cmd(CMD_USB_SEL_EP | EP, 0);
          USB_Cmd(CMD_USB_VAL_BUF, 0);
          break;
        }

        // Set Status
        EpCnfg[EP].Status = BEGIN_SERVICED;
        // Get data size
        if(EpCnfg[EP].bZeroPacketPossible && Count == EpCnfg[EP].MaxSize)
        {
          EpCnfg[EP].bZeroPacketPossible = 0;
          EpCnfg[EP].bZeroPacket = 1;
        }

        Count = MIN(Count,EpCnfg[EP].MaxSize);
        Int32U Offset = EpCnfg[EP].Offset;
        EpCnfg[EP].Offset += Count;
        // Convert EP physical address to logical and set write enable bit
        USBCtrl = ((EP << 1) & 0x3C) | bmUSB_CtrlWrEna;
        USBTxPLen = Count;
        // Write data to SIE buffer
        if(Count)
        {
          do
          {
            assert(USBCtrl_bit.WR_EN);
            Data = *(EpCnfg[EP].pBuffer+Offset++);
            if (--Count)
            {
              Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<8;
              if (--Count)
              {
                Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<16;
                if (--Count)
                {
                  Data |= (Int32U)(*(EpCnfg[EP].pBuffer+Offset++))<<24;
                  --Count;
                }
              }
            }
            USBTxData = Data;
          }
          while (Count);
          assert(!USBCtrl_bit.WR_EN);
        }
        else
        {
          EpCnfg[EP].bZeroPacket = 0;
          do
          {
            USBTxData = 0;
          }
          while (USBCtrl_bit.WR_EN);
        }

        USBCtrl = 0;

        --EpCnfg[EP].AvbBuff;
        USB_Cmd(CMD_USB_SEL_EP | EP, 0);
        USB_Cmd(CMD_USB_VAL_BUF, 0);
        Count = EpCnfg[EP].Size - EpCnfg[EP].Offset;
      }

      if(!EpCnfg[EP].bZeroPacket && !Count)
      {
        EpCnfg[EP].Status = COMPLETE;
        //call callback function
        if(EpCnfg[EP].pFn)
        {
          ((void(*)(USB_Endpoint_t))EpCnfg[EP].pFn)(EP);
        }
      }
  }
  else
  {
      while(EpCnfg[EP].AvbBuff)
      {
        // Convert EP physical address to logical and set read enable bit
        USBCtrl = ((EP << 1) & 0x3C) | bmUSB_CtrlRdEna;
        // while (USBDevIntSt_bit.RxENDPKT == 0);
        // Get data size
        Count = USBRxPLen_bit.PKT_LNGTH;
        if(Count > (EpCnfg[EP].Size - EpCnfg[EP].Offset))
        {
          EpCnfg[EP].Status = BUFFER_OVERRUN;
          EpCnfg[EP].Size = EpCnfg[EP].Offset;
          break;
        }
        else if (Count < EpCnfg[EP].MaxSize)
        {
          EpCnfg[EP].Status = BUFFER_UNDERRUN;
          EpCnfg[EP].Size = EpCnfg[EP].Offset + Count;
        }
        else
        {
          EpCnfg[EP].Status = BEGIN_SERVICED;
        }

        Int32U Offset = EpCnfg[EP].Offset;
        EpCnfg[EP].Offset += Count;

        // Read data from SIE buffer
        do
        {
          //assert(RCVEPKTLEN_bit.DV);
          Data = USBRxData;
          // because zero lenght packe is possible
          if(Count)
          {
            *(EpCnfg[EP].pBuffer+Offset++) = Data;
            if (--Count)
            {
              Data >>= 8;
              *(EpCnfg[EP].pBuffer+Offset++) = Data;
              if (--Count)
              {
                Data >>= 8;
                *(EpCnfg[EP].pBuffer+Offset++) = Data;
                if (--Count)
                {
                  --Count;
                  Data >>= 8;
                  *(EpCnfg[EP].pBuffer+Offset++) = Data;
                }
              }
            }
          }
        }
        while (Count);
        assert(!USBRxPLen_bit.DV);

        USBCtrl = 0;
        --EpCnfg[EP].AvbBuff;
        USB_Cmd(CMD_USB_SEL_EP | EP, 0);
        if(USB_Cmd(CMD_USB_CLR_BUF, 0) & bmUSB_PacketOverWritten)
        {
          EpCnfg[EP].Status = SETUP_OVERWRITE;
          return;
        }
        if (!(Count = (EpCnfg[EP].Size - EpCnfg[EP].Offset)))
        {
          EpCnfg[EP].Status = COMPLETE;
          break;
        }
      }

      if (EpCnfg[EP].Status != BEGIN_SERVICED && EpCnfg[EP].Status != NO_SERVICED)
      {
        //call callback function
        if(EpCnfg[EP].pFn)
        {
          ((void(*)(USB_Endpoint_t))EpCnfg[EP].pFn)(EP);
        }
      }
  }
}

/*************************************************************************
 * Function Name: USB_EpLogToPhysAdd
 * Parameters: Int8U EpLogAdd
 *
 * Return: USB_Endpoint_t
 *
 * Description: Convert the logical to physical address
 *
 *************************************************************************/
USB_Endpoint_t USB_EpLogToPhysAdd (Int8U EpLogAdd)
{
USB_Endpoint_t Address = (USB_Endpoint_t)((EpLogAdd & 0x0F)<<1);
  if(EpLogAdd & 0x80)
  {
    ++Address;
  }
  return(Address);
}

#if USB_SOF_EVENT > 0
/*************************************************************************
 * Function Name: USB_GetFrameNumb
 * Parameters: none
 *
 * Return: Int32U
 *
 * Description: Return current value of SOF number
 *
 *************************************************************************/
Int32U USB_GetFrameNumb (void)
{
  return(USB_SofNumbHold);
}
#endif

/*************************************************************************
 * Function Name: USB_StatusPhase
 * Parameters: Boolean In
 *
 * Return: none
 *
 * Description: Prepare status phase
 *
 *************************************************************************/
void USB_StatusPhase (Boolean In)
{
  if(In)
  {
    USB_IO_Data(CTRL_ENP_IN,NULL,0,NULL);
  }
}

/*************************************************************************
 * Function Name: USBDev_IRQHandler
 * Parameters: none
 *
 * Return: none
 *
 * Description: USB interrupt subroutine
 *
 *************************************************************************/
void USBIRQ_IRQHandler (void)
{
Int32U Val;

  USB_INTR_ENTRY_HOOK();

//  if (USBINTS_bit.USB_INT_REQ_LP)
  {
  #if USB_ERROR_EVENT > 0
    // USB engine error interrupt
    if(USBDevIntSt_bit.ERR_INT)
    {
      USBDevIntClr = bmUSB_ErrorInterrupt;
      USB_ERR_HOOK(USB_Cmd(CMD_USB_RD_ERROR_STAT,0));
    }
  #endif
  #if USB_SOF_EVENT > 0 && 1 != USB_DEV_PRIORITY
    // Frame interrupt
    if(USBDevIntSt_bit.FRAME)
    {
      USBDevIntClr = bmUSB_FrameInterrupt;
    #if USB_SOF_FRAME_NUMB > 0
      USB_FRAME_HOOK(USB_Cmd(CMD_USB_RD_FRAME_NUMB,0));
    #else
      USB_FRAME_HOOK(0);
    #endif
    }
  #endif
    // Device Status interrupt
    if(USBDevIntSt_bit.DEV_STAT)
    {
      // Clear Device status interrupt
      USBDevIntClr = bmUSB_DevStatusInterrupt;
      // Get device status
      USB_DevStatus.Data = USB_Cmd(CMD_USB_GET_DEV_STAT,0);
      // Device connection status
      if(USB_DevStatus.ConnectChange)
      {
        UsbDevConnectCallback(USB_DevStatus.Connect);
      }
      // Device suspend status
      if(USB_DevStatus.SuspendChange)
      {
        UsbDevSuspendCallback(USB_DevStatus.Suspend);
      }
      // Device reset
      if(USB_DevStatus.Reset)
      {
        USB_HwReset();
        UsbDevResetCallback();
      }
    }

    {
      Int32U Mask = bmUSB_EP0Interrupt;
      USB_Endpoint_t EP = CTRL_ENP_OUT;
      Int32U EpIntr;
      EpIntr  = USBDevIntSt ;
      EpIntr &= USBDevIntEn ;
      EpIntr &= (0xFF<<1);

    #if 2 == USB_DEV_PRIORITY
      EpIntr &= ~USB_EP_PRIORITY; // clear high priority EPs interrutp flags
    #endif // 2 == USB_DEV_PRIORITY
      while(EpIntr)
      {
        if(EpIntr & Mask)
        {
          EpIntr &= ~Mask;
          
          Val = USB_EpIntrClr(EP) ^ ((EP & 1)?((1<<6)|(1<<5)):0);
          if (!EpCnfg[EP].bDoubleBuffered)
          {
            Val &= ~(1<<6);
          }
          switch (Val & ((1<<6)|(1<<5)))
          {
          case 0:
            EpCnfg[EP].AvbBuff = 0;
            break;
          case (1<<5):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<6):
            EpCnfg[EP].AvbBuff = 1;
            break;
          case (1<<5) | (1<<6):
            EpCnfg[EP].AvbBuff = 2;
            break;
          }

          if(Val & bmUSB_EpSetupPacket)
          {
            // only ctrl EP can receive setup packets
            assert(EP == CTRL_ENP_OUT);
            EpCnfg[CTRL_ENP_IN].AvbBuff  = 1;
            // init IO to receive Setup packet
            USB_IO_Data(CTRL_ENP_IN,NULL,(Int32U)-1,NULL);
            USB_IO_Data(CTRL_ENP_OUT,UsbEp0SetupPacket.Data,sizeof(UsbSetupPacket_t),NULL);

            // reset EP IO ctrl
            if (UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host)
            {
              USB_StatusHandler(CTRL_ENP_OUT);
            }

            USB_SetupHandler();

            if(EpCnfg[CTRL_ENP_OUT].Status == STALLED)
            {
              USB_StallCtrlEP();
            }
          }
          else
          {
            if(EP == CTRL_ENP_OUT)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbDevice2Host &&
                 EpCnfg[CTRL_ENP_OUT].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_OUT].pFn)(CTRL_ENP_OUT);
                break;
              }
            }
            else if(EP == CTRL_ENP_IN)
            {
              if(UsbEp0SetupPacket.mRequestType.Dir == UsbHost2Device &&
                 EpCnfg[CTRL_ENP_IN].pFn)
              {
                ((void(*)(USB_Endpoint_t))EpCnfg[CTRL_ENP_IN].pFn)(CTRL_ENP_IN);
                break;
              }
            }
            assert(EpCnfg[EP].AvbBuff <= (EpCnfg[EP].bDoubleBuffered + 1));
            USB_EP_IO(EP);
          }          
        }
        
        EP++;
        Mask = 1<<(EP+1);
      }
    }
  }

  USB_INTR_EXIT_HOOK();
  NVIC_ClrPend(NVIC_USB_IRQ);
}

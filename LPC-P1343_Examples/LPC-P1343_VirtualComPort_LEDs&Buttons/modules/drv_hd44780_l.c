/*************************************************************************
 *
 *    Used with ICCARM and AARM.
 *
 *    (c) Copyright IAR Systems 2009
 *
 *    File name   : drv_hd44780_l.c
 *    Description : I/O function
 *
 *    History :
 *    1. Date        : February 5, 2009
 *       Author      : Stanimir Bonev
 *       Description : Create
 *
 *    $Revision: 31547 $
**************************************************************************/
#include "drv_hd44780_l.h"

/*************************************************************************
 * Function Name: HD44780_IO_Init
 * Parameters: none
 * Return: none
 * Description: Init IO ports directions and level
 *
 *************************************************************************/
void HD44780_IO_Init (void)
{
 
  LCD_RS_DIR |= LCD_RS;
  LCD_E_DIR |= LCD_E;

  LDC_DATA_DIR |= LCD_DATA_MASK<<LCD_DATA_SHIFT;

#if HD4780_WR > 0
  LCD_RW_DIR |= LCD_RW;
  LCD_RW_LOW();
#endif // HD4780_WR > 0

  HD44780SetPD();
}

/*************************************************************************
 * Function Name: HD44780SetPD
 * Parameters: none
 * Return: none
 * Description: Set power down
 *
 *************************************************************************/
void HD44780SetPD (void)
{
  // RS Output - Low
  LCD_RS_LOW();
  // E  Output - Low
  LCD_E_LOW();
  // Data lines Low
  LCD_DATA_PORT = LCD_DATA_MASK << LCD_DATA_SHIFT;
#if HD4780_WR > 0
  // WR Output - Low
  LCD_RW_LOW();
#endif // HD4780_WR > 0
}

/*************************************************************************
 * Function Name: HD44780SetPU
 * Parameters: none
 * Return: none
 * Description: Set power up
 *
 *************************************************************************/
void HD44780SetPU (void)
{
  HD44780_BUS_DLY();
  // RS Output - Low
  LCD_RS_LOW();
  // E  Output - Low
  LCD_E_LOW();
#if HD4780_WR > 0
  // WR Output - Low
  LCD_RW_HIGH();
#endif // HD4780_WR > 0
}

/*************************************************************************
 * Function Name: HD44780WrIO
 * Parameters: Int32U Data
 * Return: none
 * Description: Write to HD44780 I/O
 *
 *************************************************************************/
void HD44780WrIO (Int32U Data)
{

  // Write Data
#if HD4780_WR > 0
  LCD_RW_LOW();
  
  LDC_DATA_DIR |= LCD_DATA_MASK<<LCD_DATA_SHIFT;
  
#endif

  LCD_E_HIGH();
  LCD_DATA_PORT = ( Data  & LCD_DATA_MASK) << LCD_DATA_SHIFT;
 
  HD44780_BUS_DLY();
  LCD_E_LOW();
}

#if HD4780_WR > 0
/*************************************************************************
 * Function Name: HD44780RdIO
 * Parameters: none
 * Return: Int8U
 * Description: Read from HD44780 I/O
 *
 *************************************************************************/
Int8U HD44780RdIO (void)
{
Int8U Data;
  // Set Direction
  LDC_DATA_DIR &= ~(LCD_DATA_MASK<<LCD_DATA_SHIFT);

  LCD_RW_HIGH();
  // Read Data
  LCD_E_HIGH();
  HD44780_BUS_DLY();
  Data = LCD_DATA_PORT>>LCD_DATA_SHIFT;
  LCD_E_LOW();
  return Data;
}
#endif

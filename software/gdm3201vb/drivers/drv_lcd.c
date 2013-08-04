/******************** (C) DoWell工作室 ********************
 * 文件名  ：main.c
 * 描述    ：LCD FSMC 应用函数库。
 *           实现的功能：清屏、画点、划线、显示数字、字符串、图片、汉字         
 * 实验平台：符合FSMC接口地址线为A23线的所有开发板
 * 库版本  ：ST3.5.0
**********************************************************************************/
#include "drv_lcd.h"

//横屏和竖屏宏定义 , 液晶接口对着自己竖着放来看
/*********************************************************************************
* 如果有此宏定义则为横屏模式，否则为竖屏模式(垂直扫描 所以为横屏)
* 1：开启   0：关闭
*********************************************************************************/
#define Vertical 1   
/* 选择BANK1-BORSRAM1 连接 TFT，地址范围为0X60000000~0X63FFFFFF
 * FSMC_A23 接LCD的DC(寄存器/数据选择)脚
 * 16 bit => FSMC[24:0]对应HADDR[25:1]
 * 寄存器基地址 = 0X60000000
 * RAM基地址 = 0x60000000+2^23*2
 * 当选择不同的地址线时，地址要重新计算。
 */
#define Bank1_LCD_D    ((u32)0x61000000)    //Disp Data ADDR
#define Bank1_LCD_C    ((u32)0x60000000)    //Disp Reg ADDR

#define BLACK 0X0000

u16 POINT_COLOR = BLACK;

static void LCD_SSD1289_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Enable the FSMC Clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
    
    /* config lcd gpio clock base on FSMC */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE , ENABLE);
     
    
    /* config tft data lines base on FSMC
	   * data lines,FSMC-D0~D15: PD 14 15 0 1,PE 7 8 9 10 11 12 13 14 15,PD 8 9 10
	   */	
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | 
                                  GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                  GPIO_Pin_15;
    GPIO_Init(GPIOE, &GPIO_InitStructure); 
    
    /* config tft control lines base on FSMC
     * PD4-FSMC_NOE  :LCD-RD
     * PD5-FSMC_NWE  :LCD-WR
     * PD7-FSMC_NE1  :LCD-CS
     * PE2-FSMC_A23  :LCD-DC
	   */

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; 
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
    GPIO_Init(GPIOD, &GPIO_InitStructure);  
    
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 ; 
    GPIO_Init(GPIOE, &GPIO_InitStructure);  
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* config tft rst gpio PE1*/
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ; 	 
//    GPIO_Init(GPIOE, &GPIO_InitStructure); 
    
    /* tft control gpio init */	 
//    GPIO_ResetBits(GPIOE, GPIO_Pin_1);	          // RST = 0   
    GPIO_SetBits(GPIOD, GPIO_Pin_4);		  // RD = 1  
    GPIO_SetBits(GPIOD, GPIO_Pin_5);		  // WR = 1 
    GPIO_SetBits(GPIOD, GPIO_Pin_7);		  // CS = 1 
}

static void LCD_SSD1289_FSMC_Config(void)
{
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  p; 
  
  p.FSMC_AddressSetupTime = 0x01;	 //地址建立时间
  p.FSMC_AddressHoldTime = 0x00;	 //地址保持时间
  p.FSMC_DataSetupTime = 0x02;		 //数据建立时间
  p.FSMC_BusTurnAroundDuration = 0x00;
  p.FSMC_CLKDivision = 0x00;
  p.FSMC_DataLatency = 0x00;
  p.FSMC_AccessMode = FSMC_AccessMode_B;	 // 一般使用模式B来控制LCD
  
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p; 
  
  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure); 
  
  /* Enable FSMC Bank1_SRAM Bank */
  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);  
}

static void Delay(__IO u32 nCount)
{
    for(; nCount != 0; nCount--);
}

static void LCD_SSD1289_Rst(void)
{			
    GPIO_ResetBits(GPIOE, GPIO_Pin_1);
    Delay(0xFFF);					   
    GPIO_SetBits(GPIOE, GPIO_Pin_1 );		 	 
    Delay(0xFFF);	
}

/********************************************************************************
 * 函数名：LCD_WR_REG
 * 描述  ：SSD1289 写寄存器函数
 * 输入  ：-index 寄存器
 * 调用  ：内部调用
********************************************************************************/
static  void LCD_SSD1289_WR_REG(u16 index)
{
    *(__IO u16 *)(Bank1_LCD_C) = index;
}

/********************************************************************************
 * 函数名：LCD_WR_REG
 * 描述  ：往SSD1289寄存器写数据
 * 输入  ：-index 寄存器
 *         -val   写入的数据
********************************************************************************/
static  void LCD_SSD1289_WR_CMD(u16 index, u16 val)
{	
    *(__IO u16 *)(Bank1_LCD_C) = index;	
    *(__IO u16 *)(Bank1_LCD_D) = val;
}

/********************************************************************************
 * 函数名：LCD_WR_Data
 * 描述  ：往SSD1289 GRAM 写入数据
 * 输入  ：-val 写入的数据,16bit        
********************************************************************************/
/*
static  void LCD_WR_Data(unsigned int val)
{   
    *(__IO u16 *) (Bank1_LCD_D) = val; 	
}
 */

/********************************************************************************
 * 函数名：LCD_SSD1289_RD_data
 * 描述  ：读 SSD1289 RAM 数据
 * 输出  ：读取的数据,16bit *         
********************************************************************************/
static  u16 LCD_SSD1289_RD_data(void)
{
    u16 a = 0;
	
    a = (*(__IO u16 *)(Bank1_LCD_D)); 	//Dummy	
    a = *(__IO u16 *)(Bank1_LCD_D);     //L
    
    return(a);	
}

/*******************************************************************************

 * 函数名：LCD_SSD1289_ReadRegister
 * 描述  ：往SSD1289 GRAM 读出数据
 * 输入  ：-val 写入的数据,16bit        
*******************************************************************************/
unsigned short LCD_SSD1289_ReadRegister(void)
{  
  //写索引寄存器
  LCD_SSD1289_WR_REG(0x22);
  
  return(LCD_SSD1289_RD_data());
}
/*******************************************************************************

 *  函数名：LCD_SSD1289_BGR2RGB
 *  功能：BGR到RGB的转换
 *  输入：BGR_Code：读出的BGR数据
 *  输出：转换后的RGB颜色值
 
*******************************************************************************/
unsigned short LCD_SSD1289_BGR2RGB(unsigned short BGR_Code)
{
  unsigned short  r, g, b;

  r = BGR_Code & 0x1f;
  g = (BGR_Code>>5)  & 0x3f;
  b = (BGR_Code>>11) & 0x1f;
  
  return( (r<<11) + (g<<5) + (b<<0) );
}
/*******************************************************************************
 *  函数名：LCD_SSD1289_GetPointRGB
 *  功能：获取一个像素点的颜色值
 *  输入：x,y点的坐标
 *  输出：16bit的565颜色值
*******************************************************************************/
unsigned short LCD_SSD1289_GetPointRGB(unsigned short Xpos,unsigned short Ypos)
{
  LCD_SSD1289_SetCursor(Xpos,Ypos);
  return(LCD_SSD1289_BGR2RGB(LCD_SSD1289_ReadRegister()));
}

/*******************************************************************************
 *  函数名：RGB
 *  功能：RGB颜色混合函数
 *  输入：R 0-31,G 0-63,B 0-31
 *  输出：混合后的颜色值
*******************************************************************************/
/*
static u16 RGB(u8 R,u8 G,u8 B)
{	
    return ( ( (u8)R<<11 ) | ( (u8)G<<5 & 0X07E0 ) | ( (u8)(B&0X1F) ) );
}

*/
/*******************************************************************************

 *  函数名：LCD_SSD1289_SetCursor
 *  功能：设置LCD的坐标
 *  输入：x：LCD水平坐标
          y：LCD垂直坐标
 *  输出：无

*******************************************************************************/
void LCD_SSD1289_SetCursor(u16 Xpos, u16 Ypos)
{
#if Vertical             //当为横屏的时候坐标（0,0x013f）才是左上角
  u16 V_X,V_Y;   //当为横屏模式时为了写坐标的习惯，需要转换一下实际的x（水平坐标），  
  V_X = Ypos;               //y（垂直坐标）和习惯使用的x，y，为横屏的时候竖屏的y这时变为横屏的x
  V_Y = 0x013f - Xpos;      //竖屏的x变为横屏的y，所以有左边的转换关系。
  
  LCD_SSD1289_WR_CMD(0x4e, V_X);
  LCD_SSD1289_WR_CMD(0x4f, V_Y);  
#else             
  LCD_SSD1289_WR_CMD(0x4e, Xpos);
  LCD_SSD1289_WR_CMD(0x4f, Ypos);
#endif
  LCD_SSD1289_WR_REG(0x22);
}
/*******************************************************************************

 *  函数名：LCD_SSD1289_WriteGRAM
 *  功能：写入一个像素点数据
 *  输入：RGB_Code：像素点的颜色值
         
 *  输出：无

*******************************************************************************/
void LCD_SSD1289_WriteGRAM(u16 RGB_Code)
{
  LCD_SSD1289_WR_CMD(0x22,RGB_Code);
}
/********************************************************************************
 *  函数名：LCD_SSD1289_XorPixel
 *  功能：在指定的位置画一个点，背景色为Color与原来点的异或
 *  输入：x,y：坐标
          Color：像素点的颜色值
 *  输出：无
********************************************************************************/
void LCD_SSD1289_XorPixel(int x, int y,u16 Color)
{
  u16 Index = LCD_SSD1289_GetPointRGB(x,y);
  LCD_SSD1289_DrawPoint(x,y,Color-1-Index);
}
/********************************************************************************
 *  函数名：LCD_SSD1289_OrPixel
 *  功能：在指定的位置画一个点，背景色为Color与原来点的或
 *  输入：x,y：坐标
          Color：像素点的颜色值
 *  输出：无
********************************************************************************/
void LCD_SSD1289_OrPixel(int x, int y,u16 Color)
{
  u16 Index = LCD_SSD1289_GetPointRGB(x,y);
  LCD_SSD1289_DrawPoint(x,y,Color | Index);
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DrawPoint
 *  功能：在指定的位置画一个点，背景色为Color
 *  输入：x,y：坐标
          Color：像素点的颜色值
 *  输出：无
********************************************************************************/
void LCD_SSD1289_DrawPoint(u16 x, u16 y, u16 Color)
{
  LCD_SSD1289_SetCursor(x, y);
  LCD_SSD1289_WriteGRAM(Color);  
}

/********************************************************************************
 *  函数名：LCD_SSD1289_DrawHLine
 *  功能：  画一条水平直线
 *  输入：x,y：起始坐标
          x1:  水平终点坐标
          Color：像素点的颜色值
 *  输出：无
********************************************************************************/
void LCD_SSD1289_DrawHLine(u16 x,u16 y,u16 x1,u16 Color)
{
  u16 i;
  for(i = x;i <= x1;i++)
  {
    LCD_SSD1289_SetCursor(i,y);
    LCD_SSD1289_WriteGRAM(Color);  
  }
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DrawVLine
 *  功能：  画一条垂直直线
 *  输入： x,y：起始坐标
           x1:  垂直终点坐标
           Color：像素点的颜色值
 *  输出： 无
********************************************************************************/
void LCD_SSD1289_DrawVLine(u16 x,u16 y,u16 y1,u16 Color)
{
  u16 i;            
  for(i = y;i <= y1;i++)
  {
    LCD_SSD1289_SetCursor(x,i);
    LCD_SSD1289_WriteGRAM(Color);
  }
}
/*----------------------------------------以下为LCD应用函数----------------------------------------------*/
/********************************************************************************
 * 函数名：LCD_CLEAR
 * 描述  ：以x,y为坐标起点，在长len,高wid的范围内清屏
 * 输入  ：-x -y -len -wid
 * 输出  ：无 
********************************************************************************/
void LCD_SSD1289_CLEAR(u16 x,u16 y,u16 Wid,u16 High,u16 Color)
{                    
    u16 i,j;
    
    LCD_SSD1289_SetCursor(x,y);
    
    for(i = y;i < High;i++) //清除屏的高度 
    {  
      LCD_SSD1289_SetCursor(x,y + i);
      for(j = x;j < Wid;j++)//清除屏的宽度
        LCD_SSD1289_WriteGRAM(Color); 
    }
}
/********************************************************************************
 *  函数名：LCD_SSD1289_FullScreen
 *  功能：  把LCD置为指定的颜色（全屏）
 *  输入：  Color：LCD屏幕的颜色
 *  输出： 无
********************************************************************************/
void LCD_SSD1289_FullScreen(u16 Color)
{
#if Vertical 
  LCD_SSD1289_CLEAR(0,0,320,240,Color);
#else
  LCD_SSD1289_CLEAR(0,0,240,320,Color);
#endif
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayChar
 *  功能：  显示一个ASCII字符
 *  输入： x,y：      起始坐标
           pAscii:    字体点阵数据流
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数需要取模，可以支持2MFlash存储的ASCII点阵数据
********************************************************************************/
void LCD_SSD1289_DisplayChar(u16 x,u16 y,const u8 *pAscii,u16 WordColor,u16 BackColor)
{
  u8 i,j;
  u8 str;  
  for (i=0;i<16;i++)//因为一个字符的高度为16
  {
    LCD_SSD1289_SetCursor(x,y + i); 
    
    str = pAscii[i];//*(ASCII_Table + OffSet + i);
    for (j=0;j<8;j++)//一个字符宽度为8，一个像素点为16位
    {
       if ( str & (0x80>>j) )//0x80>>j，选择字符颜色（根据调色板）
       {
          LCD_SSD1289_WriteGRAM(WordColor);
       }
       else
       {
          LCD_SSD1289_WriteGRAM(BackColor);
       }               				
     }
  } 
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayChar_XY
 *  功能：  显示一个ASCII字符
 *  输入： x,y：      起始坐标
           pAscii:    ASCII码数据
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数不需要取模，使用本工程自带的ASCII点阵数据 ASCII_Table[]
********************************************************************************/
void LCD_SSD1289_DisplayChar_XY(u16 x,u16 y,const u8 *pAscii,u16 WordColor,u16 BackColor)
{
  u8 i,j;
  u8 str;
  u16 OffSet;  
  OffSet = (*pAscii - 32)*16;//寻找AsciiLib[]中相应字符的点阵数据
        /*pAscii传过来的是该字符的ASCII码数字表示（参看ASCII表，有严格
        顺序表示），减32是因为第0~31号是控制字符，第32号为“空格”字符
        除以16是因为在AsciiLib[]中的每个字符点阵为16个字节表示*/
  for (i=0;i<16;i++)//因为一个字符的高度为16
  {
    LCD_SSD1289_SetCursor(x,y + i); 
    
//    str = ASCII_Table[OffSet + i];//*(ASCII_Table + OffSet + i);
    for (j=0;j<8;j++)//一个字符宽度为8，一个像素点为16位
    {
       if ( str & (0x80>>j) )//0x80>>j，选择字符颜色（根据调色板）
       {
          LCD_SSD1289_WriteGRAM(WordColor);
       }
       else
       {
          LCD_SSD1289_WriteGRAM(BackColor);
       }               				
     }
  } 
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayChar_NoXY
 *  功能：  显示一个ASCII字符，无坐标指定
 *  输入： pAscii:    ASCII码数据地址
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数不需要取模，使用本工程自带的ASCII点阵数据 ASCII_Table[]
********************************************************************************/
void LCD_SSD1289_DisplayChar_NoXY(const u8 *pAscii,u16 WordColor,u16 BackColor)
{
  u8 i,j;
  u8 str;
  u16 OffSet;  
  OffSet = (*pAscii - 32)*16;//寻找AsciiLib[]中相应字符的点阵数据
        /*pAscii传过来的是该字符的ASCII码数字表示（参看ASCII表，有严格
        顺序表示），减32是因为第0~31号是控制字符，第32号为“空格”字符
        除以16是因为在AsciiLib[]中的每个字符点阵为16个字节表示*/
  for (i=0;i<16;i++)//因为一个字符的高度为16
  {
//    str = ASCII_Table[OffSet + i];//*(ASCII_Table + OffSet + i);
    for (j=0;j<8;j++)//一个字符宽度为8，一个像素点为16位
    {
       if ( str & (0x80>>j) )//0x80>>j，选择字符颜色（根据调色板）
       {
          LCD_SSD1289_WriteGRAM(WordColor);
       }
       else
       {
          LCD_SSD1289_WriteGRAM(BackColor);
       }               				
     }
  } 
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayString_XY
 *  功能：  在指定光标处显示字符串，可自动换行
 *  输入： x,y：      起始坐标
           pAscii:    ASCII码数据地址
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数不需要取模，使用本工程自带的ASCII点阵数据 ASCII_Table[]
********************************************************************************/
void LCD_SSD1289_DisplayString_XY(u16 x,u16 y,const u8 *pAscii,u16 WordColor,u16 BackColor)
{
#if Vertical 
  while(*pAscii != '\0')
  {
    if(x == 320)//当一行显示字符超过40个字符时，下一行接着显示
    {
      x = 0;
      y = y+16;
    }
    LCD_SSD1289_DisplayChar_XY(x,y,pAscii,WordColor,BackColor);
    x+=8;//传送下一个字符
    pAscii+=1;
  }
#else
  while(*pAscii != '\0')
  {
    if(x==240)//当一行显示字符超过30个字符时，下一行接着显示
    {
      x=0;
      y=y+16;
    }
    LCD_SSD1289_DisplayChar_XY(x,y,pAscii,WordColor,BackColor);
    
    x+=8;//传送下一个字符
    pAscii+=1;
  }  
#endif
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayString_NoXY
 *  功能：  显示字符串，无指定位置，可自动换行
 *  输入： pAscii:    ASCII码数据
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数不需要取模，使用本工程自带的ASCII点阵数据 ASCII_Table[]
********************************************************************************/
void LCD_SSD1289_DisplayString_NoXY(const u8 *pAscii,u16 WordColor,u16 BackColor)
{
#if Vertical 
  while(*pAscii != '\0')
  {
    LCD_SSD1289_DisplayChar_NoXY(pAscii,WordColor,BackColor);
    pAscii+=1;
  }
#else
  while(*pAscii != '\0')
  {
    LCD_SSD1289_DisplayChar_NoXY(pAscii,WordColor,BackColor);

    pAscii+=1;
  }  
#endif
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayHanZi_XY
 *  功能：  显示汉字
 *  输入： x,y：      起始坐标 
           pHz:       字体点阵数据流地址（应该为32字节）
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数需要自行取模
********************************************************************************/
void LCD_SSD1289_DisplayHanZi_XY(u16 x,u16 y,const u8 *pHz,u16 WordColor,u16 BackColor)
{
  u8 i,j;
  u16 str;
  for (i=0;i<16;i++)//因为一个字符的高度为16
  {
    LCD_SSD1289_SetCursor(x,y+i);
    str = ((u16)(*(pHz + i*2)<<8))|(*(pHz+i*2+1));
    for (j=0;j<16;j++)//一个汉字宽度为16，一个像素点为16位
    {
      if ( str & (0x8000>>j) )//0x80>>j，选择字符颜色（根据调色板）
      {
        LCD_SSD1289_WriteGRAM(WordColor);
      }
      else
      {
        LCD_SSD1289_WriteGRAM(BackColor);
      }               				
    }
  }
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayHanZiString_XY
 *  功能：  显示一串汉字
 *  输入： x,y：      起始坐标 
           pHz:       字体点阵数据流地址（应该为32 * N字节）N为汉字的个数
           WordColor：字体颜色
           BackColor：背景颜色
 *  输出： 无
 *  注意：本函数需要自行取模
********************************************************************************/
void LCD_SSD1289_DisplayHanZiString_XY(u16 x,u16 y,const u8 *pHz,u16 WordColor,u16 BackColor)
{

#if Vertical 
  u8 i = 0;
  while(i < 224)
  {
    if(x == 320)//当一行显示字符超过40个字符时，下一行接着显示
    {
      x = 0;
      y = y+16;
    }
    LCD_SSD1289_DisplayHanZi_XY(x,y,&pHz[i],WordColor,BackColor);
    x+=16;//传送下一个字符
    i+=32;
  }
#else
  u8 i = 0;
  while(i < 224)
  {
    if(x==240)//当一行显示字符超过30个字符时，下一行接着显示
    {
      x=0;
      y=y+16;
    }
    LCD_SSD1289_DisplayHanZi_XY(x,y,&pHz[i],WordColor,BackColor);
    
    x+=16;//传送下一个字符
    i+=32;
  }  
#endif
}
/********************************************************************************
 *  函数名：LCD_SSD1289_Col_ColorBar
 *  功能：  彩色竖条纹显示测试(6条竖条纹） Vertical  1 横屏  0 竖屏
 *  输入： 无
 *  输出： 无
********************************************************************************/
void LCD_SSD1289_Col_ColorBar(void)
{
  u16 j,k;
#if Vertical  
  LCD_SSD1289_SetCursor(0,0x013f);
  for(k=0;k<240;k++)
  {
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Blue);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Red);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Magenta);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Green);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Cyan);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Yellow);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Blue);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Red);
    }    
  }
#else
  LCD_SSD1289_SetCursor(0,0);
  for(k=0;k<320;k++)
  {
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Blue);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Red);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Magenta);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Green);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Cyan);
    }
    for(j=0;j<40;j++)
    {
      LCD_SSD1289_WriteGRAM(Yellow);
    }
  }
#endif  
}
/********************************************************************************
 *  函数名：LCD_SSD1289_DisplayHanZiString_XY
 *  功能：  显示指定大小的图片（图片宽和高最好为偶数）
 *  输入： x,y：        起始坐标 
           Width,Height: 图片的宽度和高度
           pBMP：       图片数据流地址
 *  输出： 无
 *  注意：本函数需要自行取模
********************************************************************************/
void LCD_SSD1289_DrawBMP(u16 x, u16 y, u16 Width, u16 Height, const u8 *pBMP)
{
  u32 m = 0;
  u16 i, j;

  LCD_SSD1289_SetCursor(x, y);

  for (i=0; i<Width; i++)  //行坐标
  {
     for (j=0; j<Height; j++) //列坐标
     {
       LCD_SSD1289_WriteGRAM((((uint16_t)(pBMP[m]))<<8) | pBMP[m + 1]);
       m = m + 2;
     }
  }
}

/********************************************************************************
 *  函数名：H_V_Type
 *  功能：  设定横屏或者竖屏
 *  输入：  Type：  显示模式  0 竖屏  1 横屏 
 *  输出：  无
********************************************************************************/
void H_V_Type(u8 Type)
{
  if(!Type)
    LCD_SSD1289_WR_CMD(0x0011,0x6070); //设定为竖屏模式
  else
    LCD_SSD1289_WR_CMD(0x0011,0x6058); //设定为横屏模式
}

void rt_hw_lcd_init(void)
{
  unsigned long i;
	
  LCD_SSD1289_GPIO_Config();
  LCD_SSD1289_FSMC_Config();		
//  LCD_SSD1289_Rst();
  
  LCD_SSD1289_WR_CMD(0x0000,0x0001);    Delay(50);  //打开晶振
  LCD_SSD1289_WR_CMD(0x0003,0xA8A4);    Delay(50);   //0xA8A4
  LCD_SSD1289_WR_CMD(0x000C,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x000D,0x080C);    Delay(50);
  LCD_SSD1289_WR_CMD(0x000E,0x2B00);    Delay(50);
  LCD_SSD1289_WR_CMD(0x001E,0x00B0);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0001,0x2B3F);    Delay(50);   //驱动输出控制320*240  0x6B3F  293f  2b3f 6b3f
  LCD_SSD1289_WR_CMD(0x0002,0x0600);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0010,0x0000);    Delay(50);
#if Vertical  
  H_V_Type(1);   //  LCD_WR_CMD(0x0011,0x6070);//0x4030 //定义数据格式  16位色  横屏 0x6058	 6078
#else
  H_V_Type(0);  
#endif
	Delay(50);
  LCD_SSD1289_WR_CMD(0x0005,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0006,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0016,0xEF1C);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0017,0x0003);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0007,0x0233);    Delay(50);        //0x0233
  LCD_SSD1289_WR_CMD(0x000B,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x000F,0x0000);    Delay(50);        //扫描开始地址
  LCD_SSD1289_WR_CMD(0x0041,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0042,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0048,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0049,0x013F);    Delay(50);
  LCD_SSD1289_WR_CMD(0x004A,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x004B,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0044,0xEF00);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0045,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0046,0x013F);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0030,0x0707);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0031,0x0204);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0032,0x0204);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0033,0x0502);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0034,0x0507);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0035,0x0204);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0036,0x0204);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0037,0x0502);    Delay(50);
  LCD_SSD1289_WR_CMD(0x003A,0x0302);    Delay(50);
  LCD_SSD1289_WR_CMD(0x003B,0x0302);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0023,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0024,0x0000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x0025,0x8000);    Delay(50);
  LCD_SSD1289_WR_CMD(0x004f,0);         Delay(30);//行首址0
  LCD_SSD1289_WR_CMD(0x004e,0);         Delay(30);//列首址0

  for (i=0; i<76800; i++)		   //320*240=76800		
  {
      LCD_SSD1289_WriteGRAM(Blue);	
  }    
}



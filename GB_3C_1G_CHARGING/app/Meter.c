

/**********************************************************
德和新能源科技有限公司
File name:  hwtst.c	              // 文件名
Author:     韩忠华                   // 作者
Version:    v1.00                   // 版本信息
Date:       2015-1-23              // 完成日期
Description:  	              // 使用232串口与GPRS模块通信，间接实现与综合平台的通信
Others:		              // 
Function List:	              //
History:                        //
**********************************************************/

#include "board.h"
#include "Dio.h"
#include "uart.h"
#include "eeprom.h"
#include "crc.h"

//校验码问题
extern UART_BUFF RS485Rx1;
uint8_t MeterData[4], MeterAddr[6];
uint8_t MeterErr, MeterCommErr, MeterAddrFlag;
uint8_t Position68;
uint32_t RdMeterTicks, RdMeterTicks1, RdMeterTicks2;       //读电量，读电压，读电流时间周期
uint8_t Meter_counter;
uint16_t MeterOnlineCnt;
uint32_t CurKwh;
uint32_t Cur_0_BeginTime,Cur_0_Time;
uint8_t RelaySta2;      //4852接受状态指示灯，交替开关
extern uint16_t output_vol, output_cur;         //输出电压电流，
                                                //电流电表反馈三位小数，下文做特别处理，舍掉最后一位，系统中按照两位小数传输
                                                //电压电表反馈一位小数，下文做特别处理，小数第二位补0，系统中按照两位小数传输


/*************************************************************
Function: uint32_t GetCurKwh ( void )
Description: 获取当前电量
Calls:       无
Called By:   
Input:       
Output:      无
Return:      CurKwh 当前正向有功总电量
Others:      无
*************************************************************/
uint32_t GetCurKwh ( void )
{
    return CurKwh;
}





/*************************************************************
Function: uint8_t Request_CSCalc(UART_BUFF f)
Description: 计算发送请求的校验码
Calls:       无
Called By:   
Input:       UART_BUFF f 应答帧数据
Output:      无
Return:      uint8_t cs 校验码
Others:      无
*************************************************************/
uint8_t Request_CSCalc ( UART_BUFF f )
{
    uint8_t cs = 0x00, i, len, j;
    len = f.Len;
    Position68 = 0 ;

    for ( i = 0; i < len; i++ )
    {
        if ( f.Buff[i] == 0x68 )
        {
            Position68 = i ;
            
            for ( j = i; j < len - 2; j++ )
            {
                cs += f.Buff[j];
            }

            break;
        }
    }
    
    return cs;
}

/*************************************************************
Function: uint8_t Answer_CSCalc(UART_BUFF f)
Description: 计算应答的校验码
Calls:       无
Called By:   
Input:       UART_BUFF f 应答帧数据
Output:      无
Return:      uint8_t cs 校验码
Others:      无
*************************************************************/
uint8_t Answer_CSCalc ( UART_BUFF f )
{
    uint8_t cs = 0x00, i, len, j;
    len = f.Len;
    Position68 = 0 ;

    for ( i = 0; i < len; i++ )
    {
        if ( f.Buff[i] == 0x68 )
        {
            Position68 = i ;
            f.Len = f.Buff[i+9] + 16;
            for ( j = i; j < len - 2; j++ )
            {
                cs += f.Buff[j];
            }

            break;
        }
    }
    
    return cs;
}

/*************************************************************
Function: void MeterDeal(void)
Description: 读取电表表号
Calls:       无
Called By:
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void ReadMeterAddr ( void ) //读取电表表号
{

}
/*************************************************************
Function: void BcdToHex(uint32_t *dat ,uint8_t *ptr)
Description: BCD数据转换为16进制
Calls:       无
Called By:   
Input:       *dat--HEX数据指针 *ptr--BCD数据指针
Output:      无
Return:      无
Others:      无
*************************************************************/
void BcdToHex(uint32_t *dat ,uint8_t *ptr)
{
    uint32_t tmp ;
    uint8_t tmp1;
    tmp = 0 ;
    tmp1 = *ptr ;
    tmp = (tmp1>>4)*10 ;
    tmp1 &= 0x0f;
    tmp = tmp + tmp1 ;
    
    tmp1 = *(ptr+1);
    tmp += (tmp1>>4)*1000 ;
    tmp1 &= 0x0f ;
    tmp += tmp1*100;
    
    tmp1 = *(ptr+2);
    tmp += (tmp1>>4)*100000 ;
    tmp1 &=0x0f ;
    tmp += tmp1*10000 ;
    
    tmp1 = *(ptr+3);
    tmp += (tmp1>>4)*10000000 ;
    tmp1 &= 0x0f;
    tmp += tmp1*1000000 ;
    *dat = tmp ;    
}
/*************************************************************
Function: void MeterGetRs232()
Description: 从站应答帧
Calls:       uint8_t CSCalc(UART_BUFF f)
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void GetMeterMsg ( void ) //从站应答帧
{

    uint8_t  len, i, j, CS;
    uint8_t tmp_[4];
    uint8_t temp;
    uint32_t tmp,tmp1,tmp2;
    
    uint16_t temp_;
    uint16_t *data;
    uint8_t crcl, crch;
    uint16_t addr = 0;
      
    if ( GetRs485Ch1Sta() == 1 ) //帧接收结束标记
    {
        if (len<8) 
        {
          RS485Rx1.Flag = 0 ;
          RS485Rx1.Len = 0 ;
          RS485Rx1.Idx = 0 ; 
          return;
        }
        MeterOnlineCnt = 1;
        CS = Answer_CSCalc ( RS485Rx1 );
        len = RS485Rx1.Len;
        RS485Rx1.Flag = 0 ;
        RS485Rx1.Len = 0 ;
        RS485Rx1.Idx = 0 ;        
  
///////////////////////////////////////////////////////////////////////////////////////////////////////    
///////////////////////////////////////////////////////////////////////////////////////////////////////  
    
    
          
       if ( RS485Rx1.Buff[0] == 1)//先判断是否本机地址
        {
            Chip_CRC_Init();
          crch = CRC16(RS485Rx1.Buff,(len-2),Hi);
          crcl = CRC16(RS485Rx1.Buff,(len-2),Lo);

            if ( ( crcl == RS485Rx1.Buff[len-2] ) && ( crch == RS485Rx1.Buff[len-1] ) ) //比较CRC
            {
                switch ( RS485Rx1.Buff[1] ) //功能码
                {
                    case 0x03:// 从主站接收到的：地址 03 地址高 地址低 寄存器数量高 寄存器数量低 CRCL CRCH
                        // 发送给主站的：地址 03 字节长度 数据1 数据2 数据N CRCL CRCH
                    {
                        RS485Tx1.Buff[0] = RS485Rx1.Buff[0] ;//地址
                        RS485Tx1.Buff[1] = RS485Rx1.Buff[1] ;//03功能码

                        temp = RS485Rx1.Buff[4] ;
                        temp <<= 8 ;
                        temp |= RS485Rx1.Buff[5] ;//temp中为主站要求的长度
                        temp *= 2 ;              //1个寄存器为2字节，所以长度X2
                        RS485Tx1.Buff[2] = temp ;//长度
                        len = temp ;
                        addr = * ( ( uint16_t * ) ( &RS485Rx1.Buff[2] ) );
                        addr =  SWAP ( addr ) ;
                        
                        switch(addr)
                        {
                            case 0x64:
                            {
                                LoadParameter();
                                RS485Tx1.Buff[3] = g_sParaConfig.id[0];
                                RS485Tx1.Buff[4] = g_sParaConfig.id[1];
                                RS485Tx1.Buff[5] = g_sParaConfig.id[2];
                                RS485Tx1.Buff[6] = g_sParaConfig.id[3];
                                break;
                            }
                        }

                        temp += 3 ;//数据区长度 + 帧头
                        
   //                     Chip_CRC_Init ();
   //                     data = ( uint16_t * )RS485Rx1.Buff;
   //                     temp_ = Chip_CRC_CRC16 ( data, temp );
   //                     crcl = temp_ ;
   //                     crch = temp_ >> 8 ;
                        
                         crch = CRC16(RS485Tx1.Buff,(temp),Hi);
                         crcl = CRC16(RS485Tx1.Buff,(temp),Lo);                       
                                                
                        RS485Tx1.Buff[temp] = crcl;
                        RS485Tx1.Buff[temp+1] = crch;
                        RS485Tx1.Len = temp + 2 ; //给发送长度
                        RS485Ch1SendMsg();
                        break;
                    }
                    case 0x10:// 从主站接收到的：地址 0x10 地址高 地址低 寄存器数量高 寄存器数量低 字节数 数据1  数据2 数据N CRCL CRCH
                    {
                    
                        // 发送给主站的：地址 0x10 地址高 地址低 寄存器数量高 寄存器数量低 CRCL CRCH
                        RS485Tx1.Buff[0] = RS485Rx1.Buff[0] ;//地址
                        RS485Tx1.Buff[1] = RS485Rx1.Buff[1] ;//01功能码
                        RS485Tx1.Buff[2] = RS485Rx1.Buff[2] ;//
                        RS485Tx1.Buff[3] = RS485Rx1.Buff[3] ;//
                        RS485Tx1.Buff[4] = RS485Rx1.Buff[4] ;//
                        RS485Tx1.Buff[5] = RS485Rx1.Buff[5] ;//

     //                   Chip_CRC_Init ();
     //                   data = ( uint16_t * )RS485Rx1.Buff;
     //                   temp_ = Chip_CRC_CRC16 ( data, 6 );
     //                   crcl = temp_ ;
      //                  crch = temp_ >> 8 ;
                        
                        crch = CRC16(RS485Rx1.Buff,6,Hi);
                        crcl = CRC16(RS485Rx1.Buff,6,Lo);                      
                        
                        RS485Tx1.Buff[6] = crcl;
                        RS485Tx1.Buff[7] = crch;

                        RS485Tx1.Len = 8 ;//给发送长度
                        RS485Ch1SendMsg(); //先发送回应给主站，避免数据处理时间过长导致通信错误

                        temp = RS485Tx1.Buff[4] ;
                        temp <<= 8 ;
                        temp |= RS485Tx1.Buff[5] ;//temp中为主站要求的长度
                        temp *= 2 ;             //1个寄存器为2字节，所以长度X2
                        RS485Rx1.Buff[2] = temp;//长度
                        len = temp ;//取长度
                        addr = * ( ( uint16_t * ) ( &RS485Tx1.Buff[2] ) );
                        addr =  SWAP ( addr ) ; //取地址
                        i = 7;
                        
                        switch(addr)
                        {
                            case 0x64:
                            {
                                g_sParaConfig.id[0] = RS485Rx1.Buff[7];
                                g_sParaConfig.id[1] = RS485Rx1.Buff[8];
                                g_sParaConfig.id[2] = RS485Rx1.Buff[9];
                                g_sParaConfig.id[3] = RS485Rx1.Buff[10];
                                SaveParameter();
                                break;
                            }
                        }

                    }

                }
            }
            else
            {
                return;
            }
        }
        
        
///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////      
        
        if ( CS == RS485Rx1.Buff[len-2] )
        {
            if ( RS485Rx1.Buff[8+Position68] == 0x91 ) //读取正向有功总电量,正常应答
            {

                      //接收状态指示灯
        RelaySta2 = ~ RelaySta2 ;
        temp = RelaySta2;
        if ( temp & 0x1 )
        {
            SetOutput ( LED2 );
        }
        else
        {
            ClrOutput ( LED2 );
        }
              
              
              switch (RS485Rx1.Buff[9 + Position68])
                {
                case 0x06:
                    {
                        output_vol = 0;
                        for ( i = 14 + Position68, j = 0; j < 2; i++, j++ )
                        {
                            tmp_[j] = RS485Rx1.Buff[i] - 0x33;
                        }
                        tmp = ((tmp_[0] >> 4) & 0x0f) * 100 + (tmp_[0] & 0x0f) *10;
                        tmp1 = ((tmp_[1] >> 4) & 0x0f) * 10000 + (tmp_[1] & 0x0f) *1000;
                        output_vol = tmp + tmp1 ;
                        break;
                    }
                case 0x07:
                    {
                        //output_cur = 0;
                        for ( i = 14 + Position68, j = 0; j < 3; i++, j++ )
                        {
                            tmp_[j] = RS485Rx1.Buff[i] - 0x33;
                        }
                        tmp = ((tmp_[0] >> 4) & 0x0f) ;
                        tmp1 = ((tmp_[1] >> 4) & 0x0f) * 100 + (tmp_[1] & 0x0f) *10;
                        tmp2 = ((tmp_[2] >> 4) & 0x0f) * 10000 + (tmp_[2] & 0x0f) *1000;
                        output_cur = tmp + tmp1 + tmp2; 
                        if (output_cur > 100)           //电流大于1A
                        {
                           Cur_0_BeginTime = SysTickCnt; 
                        }
                        break;
                    }
                case 0x08://读出电表数
                    {
                        for ( i = 14 + Position68, j = 0; j < 4; i++, j++ )
                        {
                            MeterData[j] = RS485Rx1.Buff[i] - 0x33;
                        }
                        BcdToHex ( &CurKwh, MeterData ); //电表返回BCD码,转为HEX    
                        break;
                    }
                }
            }
            else if ( RS485Rx1.Buff[8+Position68] == 0xD1 ) //读电表数，从站异常应答
            {
                MeterErr = RS485Rx1.Buff[len-3]; //读取错误信息字
            }
        }
    }

}

/*************************************************************
Function: void MeterCalc(void)
Description: 读取正向有功总电量
Calls:       uint8_t CSCalc(UART_BUFF f)
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void MeterCalc ( void ) //读取正向有功总电量 68 AA AA AA AA AA AA 68 11 04 33 33 34 33 AE 16
{

    uint8_t CS;
    RS485Tx1.Buff[0] = 0xFE; //前导字节
    RS485Tx1.Buff[1] = 0xFE;
    RS485Tx1.Buff[2] = 0xFE;
    RS485Tx1.Buff[3] = 0xFE;
    RS485Tx1.Buff[4] = 0x68; //帧起始符

    RS485Tx1.Buff[5] = 0xaa;//MeterAddr[0];//地址域
    RS485Tx1.Buff[6] = 0xaa;//MeterAddr[1];
    RS485Tx1.Buff[7] = 0xaa;//MeterAddr[2];
    RS485Tx1.Buff[8] = 0xaa;//MeterAddr[3];
    RS485Tx1.Buff[9] = 0xaa;//MeterAddr[4];
    RS485Tx1.Buff[10] = 0xaa;//MeterAddr[5];

    RS485Tx1.Buff[11] = 0x68; //帧起始符
    RS485Tx1.Buff[12] = 0x11; //控制码
    RS485Tx1.Buff[13] = 0x04; //数据域长度
    RS485Tx1.Buff[14] = 0x33; //数据标识码
    RS485Tx1.Buff[15] = 0x33;
    RS485Tx1.Buff[16] = 0x34;
    RS485Tx1.Buff[17] = 0x33;

    RS485Tx1.Len = 20;//发送长度
    CS = Request_CSCalc ( RS485Tx1 );    
    RS485Tx1.Buff[18] = CS; //校验码
    RS485Tx1.Buff[19] = 0x16; //结束符
//for (CS=0;CS<20;CS++) RS485Tx1.Buff[CS] = 0X55;
    RS485Ch1SendMsg();//读取电表总电量请求帧

}

/*************************************************************
Function: void MeterVol(void)
Description: 读电压
Calls:       uint8_t CSCalc(UART_BUFF f)
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void MeterVol ( void ) //读取正向有功总电量 68 AA AA AA AA AA AA 68 11 04 33 34 34 35 B1 16
{

    uint8_t CS;
    RS485Tx1.Buff[0] = 0xFE; //前导字节
    RS485Tx1.Buff[1] = 0xFE;
    RS485Tx1.Buff[2] = 0xFE;
    RS485Tx1.Buff[3] = 0xFE;
    RS485Tx1.Buff[4] = 0x68; //帧起始符

    RS485Tx1.Buff[5] = 0xaa;//MeterAddr[0];//地址域
    RS485Tx1.Buff[6] = 0xaa;//MeterAddr[1];
    RS485Tx1.Buff[7] = 0xaa;//MeterAddr[2];
    RS485Tx1.Buff[8] = 0xaa;//MeterAddr[3];
    RS485Tx1.Buff[9] = 0xaa;//MeterAddr[4];
    RS485Tx1.Buff[10] = 0xaa;//MeterAddr[5];

    RS485Tx1.Buff[11] = 0x68; //帧起始符
    RS485Tx1.Buff[12] = 0x11; //控制码
    RS485Tx1.Buff[13] = 0x04; //数据域长度
    RS485Tx1.Buff[14] = 0x33; //数据标识码
    RS485Tx1.Buff[15] = 0x34;
    RS485Tx1.Buff[16] = 0x34;
    RS485Tx1.Buff[17] = 0x35;

    RS485Tx1.Len = 20;//发送长度
    CS = Request_CSCalc ( RS485Tx1 );    
    RS485Tx1.Buff[18] = CS; //校验码
    RS485Tx1.Buff[19] = 0x16; //结束符
//for (CS=0;CS<20;CS++) RS485Tx1.Buff[CS] = 0X55;
    RS485Ch1SendMsg();//读取电表总电量请求帧

}

/*************************************************************
Function: void MeterCur(void)
Description: 读电流
Calls:       uint8_t CSCalc(UART_BUFF f)
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void MeterCur ( void ) //读取正向有功总电量 68 AA AA AA AA AA AA 68 11 04 33 34 35 35 B2 16
{

    uint8_t CS;
    RS485Tx1.Buff[0] = 0xFE; //前导字节
    RS485Tx1.Buff[1] = 0xFE;
    RS485Tx1.Buff[2] = 0xFE;
    RS485Tx1.Buff[3] = 0xFE;
    RS485Tx1.Buff[4] = 0x68; //帧起始符

    RS485Tx1.Buff[5] = 0xaa;//MeterAddr[0];//地址域
    RS485Tx1.Buff[6] = 0xaa;//MeterAddr[1];
    RS485Tx1.Buff[7] = 0xaa;//MeterAddr[2];
    RS485Tx1.Buff[8] = 0xaa;//MeterAddr[3];
    RS485Tx1.Buff[9] = 0xaa;//MeterAddr[4];
    RS485Tx1.Buff[10] = 0xaa;//MeterAddr[5];

    RS485Tx1.Buff[11] = 0x68; //帧起始符
    RS485Tx1.Buff[12] = 0x11; //控制码
    RS485Tx1.Buff[13] = 0x04; //数据域长度
    RS485Tx1.Buff[14] = 0x33; //数据标识码
    RS485Tx1.Buff[15] = 0x34;
    RS485Tx1.Buff[16] = 0x35;
    RS485Tx1.Buff[17] = 0x35;

    RS485Tx1.Len = 20;//发送长度
    CS = Request_CSCalc ( RS485Tx1 );    
    RS485Tx1.Buff[18] = CS; //校验码
    RS485Tx1.Buff[19] = 0x16; //结束符
//for (CS=0;CS<20;CS++) RS485Tx1.Buff[CS] = 0X55;
    RS485Ch1SendMsg();//读取电表总电量请求帧

}

/*************************************************************
Function:     void MeterJs(void)
Description:  广播校时
Calls:       无
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void MeterJs ( void ) //广播校时
{


}

/*************************************************************
Function:     uint8_t GetMeterOnlineSta(void)
Description:  获取电表通信状态
Calls:       无
Called By:   无
Input:       无
Output:      无
Return:      0-在线 1-掉线
Others:      无
*************************************************************/
uint8_t GetMeterOnlineSta(void)
{
    return MeterCommErr;
}

/*************************************************************
Function:     void MeterDeal ( void )
Description:  读电表，判断是否掉线
Calls:       无
Called By:   无
Input:       无
Output:      无
Return:      无
Others:      无
*************************************************************/
void MeterDeal ( void )
{
    GetMeterMsg();

    if ( MeterOnlineCnt >= 10 )         //10次通信超时则认为电表通信故障
    {
        MeterCommErr = 1 ;
    }
    else
    {
        MeterCommErr = 0 ;
    }
    
    if (SysTickCnt > RdMeterTicks + 1000 )//每1s读一次
    {
        MeterOnlineCnt += 1;
        if (Meter_counter == 1)
        {
            RdMeterTicks = SysTickCnt ;
            MeterCalc();//读电表电量
            Meter_counter += 1;
        }
        else if (Meter_counter == 2)
        {       
            RdMeterTicks = SysTickCnt ;
            MeterVol();//读电表电压
            Meter_counter += 1;
        }
        else if (Meter_counter == 3)
        {
            RdMeterTicks = SysTickCnt ;
            MeterCur();//读电表电流
            Meter_counter += 1;
        }
        else
        {
            Meter_counter = 1;
        }
    }
}












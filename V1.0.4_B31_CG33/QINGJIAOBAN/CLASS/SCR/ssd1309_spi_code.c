#include "ssd1309_spi_head.h"
#include "oledfont.h"
uint8_t SSD1309_SPI_BUFFER[128][8]={0};


 void SSD1309_SPI_WR(SSD1309_SPI_STR *ssd1309_str,uint8_t data,uint8_t cmd)
 {
     if(cmd)
        ssd1309_str->dc->write(ssd1309_str->dc,Bit_SET);
     else
        ssd1309_str->dc->write(ssd1309_str->dc,Bit_RESET);
     ssd1309_str->spi->readWrite(ssd1309_str->spi,data);
     ssd1309_str->dc->write(ssd1309_str->dc,Bit_SET);
 }
 
 void SSD1309_SPI_COLOR_TURN(SSD1309_SPI_STR *ssd1309_str,uint8_t i)
 {
     if(i == 0)
         ssd1309_str->wr_byte(ssd1309_str,0xA6,OLED_CMD);//正常显示
     else
         ssd1309_str->wr_byte(ssd1309_str,0xA7,OLED_CMD);//反向显示
 }
 
 void SSD1309_SPI_DISPLAY_TURN(SSD1309_SPI_STR *ssd1309_str,uint8_t i)
 {
     if(i == 0)
     {
         ssd1309_str->wr_byte(ssd1309_str,0xC8,OLED_CMD);//正常显示
         ssd1309_str->wr_byte(ssd1309_str,0xA1,OLED_CMD);
     }
     else
     {
         ssd1309_str->wr_byte(ssd1309_str,0xC0,OLED_CMD);//反转显示
         ssd1309_str->wr_byte(ssd1309_str,0xA0,OLED_CMD);
     }
 }
 
 void SSD1309_SPI_DISPLAY_ON(SSD1309_SPI_STR *ssd1309_str)
 {
     ssd1309_str->wr_byte(ssd1309_str,0x8D,OLED_CMD);    //电荷泵使能
     ssd1309_str->wr_byte(ssd1309_str,0x14,OLED_CMD);    //开启电荷泵
     ssd1309_str->wr_byte(ssd1309_str,0xAF,OLED_CMD);    //开启屏幕
 }
 
 void SSD1309_SPI_DISPLAY_OFF(SSD1309_SPI_STR *ssd1309_str)
 {
     ssd1309_str->wr_byte(ssd1309_str,0x8D,OLED_CMD);    //电荷泵使能
     ssd1309_str->wr_byte(ssd1309_str,0x10,OLED_CMD);    //关闭电荷泵
     ssd1309_str->wr_byte(ssd1309_str,0xAE,OLED_CMD);    //关闭屏幕
 }
 
 void SSD1309_SPI_REFRESH(SSD1309_SPI_STR *ssd1309_str)
 {
     for(uint8_t i=0;i<8;i++)
     {
         ssd1309_str->wr_byte(ssd1309_str,0xb0+i,OLED_CMD);    //设置起始行地址
         ssd1309_str->wr_byte(ssd1309_str,0x00,OLED_CMD);      //设置低列起始地址
         ssd1309_str->wr_byte(ssd1309_str,0x10,OLED_CMD);      //设置高列起始地址
         for(uint8_t n=0;n<128;n++)
            ssd1309_str->wr_byte(ssd1309_str,SSD1309_SPI_BUFFER[n][i],OLED_DATA);
     }
 }
 
 void SSD1309_SPI_CLEAR(SSD1309_SPI_STR *ssd1309_str)
 {
    for(uint8_t i=0;i<8;i++)
    {
        for(uint8_t n=0;n<128;n++)
        {
            SSD1309_SPI_BUFFER[n][i]=0;
        }
    }
    ssd1309_str->refresh(ssd1309_str);
 }
 
 // x = 0~127   y = 0~63
 void SSD1309_SPI_DRAW_POINT(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t mode)
 {
     uint8_t i,m,n;
     i = y/8;
     m = y%8;
     n = 1<<m;
     if(mode)
         SSD1309_SPI_BUFFER[x][i]|=n;
     else
     {
         SSD1309_SPI_BUFFER[x][i] = ~SSD1309_SPI_BUFFER[x][i];
         SSD1309_SPI_BUFFER[x][i] |= n;
         SSD1309_SPI_BUFFER[x][i] = ~SSD1309_SPI_BUFFER[x][i];
     }
 }
 //画线
 //x1,y1:起点坐标
 //x2,y2:结束坐标
 void SSD1309_SPI_DRAW_LINE(SSD1309_SPI_STR *ssd1309_str,uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t mode)
 {
    uint16_t t;
    int32_t xerr=0,yerr=0,delta_x,delta_y,distance;
    int32_t incx,incy,uRow,uCol;
    delta_x=x2-x1; //计算坐标增量 
    delta_y=y2-y1;
    uRow=x1;//画线起点坐标
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向 
    else if (delta_x==0)incx=0;//垂直线 
    else 
    {
        incx=-1;
        delta_x=-delta_x;
    }
    if(delta_y>0)incy=1;
    else if (delta_y==0)incy=0;//水平线 
    else 
    {
        incy=-1;
        delta_y=-delta_x;
    }
    if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
    else distance=delta_y;
    for(t=0;t<distance+1;t++)
    {
        ssd1309_str->draw_point(ssd1309_str,uRow,uCol,mode);//画点
        xerr+=delta_x;
        yerr+=delta_y;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
 }
 
 void SD1309_DRAW_CIRCLE(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t r)
 {
    uint16_t a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)      
    {
        ssd1309_str->draw_point(ssd1309_str,x + a, y - b,1);
        ssd1309_str->draw_point(ssd1309_str,x - a, y - b,1);
        ssd1309_str->draw_point(ssd1309_str,x - a, y + b,1);
        ssd1309_str->draw_point(ssd1309_str,x + a, y + b,1);
 
        ssd1309_str->draw_point(ssd1309_str,x + b, y + a,1);
        ssd1309_str->draw_point(ssd1309_str,x + b, y - a,1);
        ssd1309_str->draw_point(ssd1309_str,x - b, y - a,1);
        ssd1309_str->draw_point(ssd1309_str,x - b, y + a,1);
        
        a++;
        num = (a * a + b * b) - r*r;//计算画的点离圆心的距离
        if(num > 0)
        {
            b--;
            a--;
        }
    }
 } 
 
 //在指定位置显示一个字符,包括部分字符
 //x:0~127
 //y:0~63
 //size1:选择字体 6x8/6x12/8x16/12x24
 //mode:0,反色显示;1,正常显示
 void SD1309_SHOW_CHAR(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t chr,uint8_t size1,uint8_t mode)
 {
     uint8_t i,m,temp,size2,chr1;
     uint8_t x0=x,y0=y;
     if(size1==8)size2=6;
     else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
     chr1=chr-' ';  //计算偏移后的值
     for(i=0;i<size2;i++)
     {
        if(size1==8)
            temp=asc2_0806[chr1][i]; //调用0806字体
        else if(size1==12)
            temp=asc2_1206[chr1][i]; //调用1206字体
        else if(size1==16)
            temp=asc2_1608[chr1][i]; //调用1608字体
        else if(size1==24)
            temp=asc2_2412[chr1][i]; //调用2412字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01)
                ssd1309_str->draw_point(ssd1309_str,x,y,mode);
            else 
                ssd1309_str->draw_point(ssd1309_str,x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((size1!=8)&&((x-x0)==size1/2))
        {
            x=x0;
            y0=y0+8;
        }
        y=y0;
     }
 }

 //显示字符串
 //x,y:起点坐标  
 //size1:字体大小 
 //*chr:字符串起始地址 
 //mode:0,反色显示;1,正常显示
 void SD1309_SHOW_STR(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t* chr,uint8_t size1,uint8_t mode)
 {
     while((*chr>=' ')&&(*chr<='~'))
     {
         ssd1309_str->show_char(ssd1309_str,x,y,*chr,size1,mode);
         if(size1==8)x+=6;
         else x+=size1/2;
         chr++;
     }
 }

 //m^n
 u32 OLED_Pow(u8 m,u8 n)
 {
     u32 result=1;
     while(n--)
     {
         result*=m;
     }
     return result;
 }
 
 //显示数字
 //x,y :起点坐标
 //num :要显示的数字
 //len :数字的位数
 //size:字体大小
 //mode:0,反色显示;1,正常显示
 void SD1309_SHOW_NUM(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1,uint8_t mode,char zero)
 {
     uint8_t t,temp,m=0;
     if(size1==8)m=2;
     for(t=0;t<len;t++)
     {
        temp=(num/OLED_Pow(10,len-t-1))%10;
        if(temp==0)
        {
            
            ssd1309_str->show_char(ssd1309_str,x+(size1/2+m)*t,y,zero,size1,mode);
        }
        else 
        {
            ssd1309_str->show_char(ssd1309_str,x+(size1/2+m)*t,y,temp+'0',size1,mode);
        }
    }
 }
 
 //显示汉字
 //x,y:起点坐标
 //num:汉字对应的序号
 //mode:0,反色显示;1,正常显示
 void SD1309_SHOW_CHINESE(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t num,uint8_t size1,uint8_t mode)
 {
    uint8_t m,temp;
    uint8_t x0=x,y0=y;
    uint16_t i,size3=(size1*size1)>>3;  //得到字体一个字符对应点阵集所占的字节数
    for(i=0;i<size3;i++)
    {
        if(size1==16)
                {temp=Hzk1[num][i];}//调用16*16字体
        else if(size1==24)
                {temp=Hzk2[num][i];}//调用24*24字体
        else if(size1==32)       
                {temp=Hzk3[num][i];}//调用32*32字体
        else if(size1==64)
                {temp=Hzk4[num][i];}//调用64*64字体
        else return;
        for(m=0;m<8;m++)
        {
            if(temp&0x01)ssd1309_str->draw_point(ssd1309_str,x,y,mode);
            else ssd1309_str->draw_point(ssd1309_str,x,y,!mode);
            temp>>=1;
            y++;
        }
        x++;
        if((x-x0)==size1)
        {x=x0;y0=y0+8;}
        y=y0;
    }
 }

 //显示汉字
 //x,y:起点坐标
 //num:汉字对应的序号
 //mode:0,反色显示;1,正常显示
 void SD1309_SHOW_CHINESEs(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,char*chinese,uint8_t size1,uint8_t mode)
 {
    uint16_t size = 32;
    uint8_t temp,x0 = x,y0 = y;
    for(uint8_t i=0;i<64;i++)
    {
         if(strcmp(chinese, (char*)ch_16[i].chinese) == 0)
         {
             for(uint8_t j=0;j<size;j++)
             {
                 temp = ch_16[i].code[j];
                 for(uint8_t m=0;m<8;m++)
                 {
                    if(temp&0x01)ssd1309_str->draw_point(ssd1309_str,x,y,mode);
                    else ssd1309_str->draw_point(ssd1309_str,x,y,!mode);
                    temp>>=1;
                    y++;
                 }
                 x++;
                 if((x-x0)==size1)
                 {
                    x=x0;
                    y0=y0+8;
                 }
                 y=y0;
             }
             break;
         }
    }
 }
 
 void SD1309_SHOW_PICTURE(SSD1309_SPI_STR *ssd1309_str,uint8_t x,uint8_t y,uint8_t sizex,uint8_t sizey,uint8_t* BMP,uint8_t mode)
 {
    u16 j=0;
    u8 i,n,temp,m;
    u8 x0=x,y0=y;
    sizey=sizey/8+((sizey%8)?1:0);
    for(n=0;n<sizey;n++)
    {
         for(i=0;i<sizex;i++)
         {
            temp=BMP[j];
            j++;
            for(m=0;m<8;m++)
            {
                if(temp&0x01)ssd1309_str->draw_point(ssd1309_str,x,y,mode);
                else ssd1309_str->draw_point(ssd1309_str,x,y,!mode);
                temp>>=1;
                y++;
            }
            x++;
            if((x-x0)==sizex)
            {
                x=x0;
                y0=y0+8;
            }
            y=y0;
         }
     }
 }

 
 void SSD1309_SPI_INIT(SSD1309_SPI_STR *ssd1309_str)
 {
     ssd1309_str->dc->str_init(ssd1309_str->dc);
     ssd1309_str->res->str_init(ssd1309_str->res);
     ssd1309_str->spi->str_init(ssd1309_str->spi);
     
     ssd1309_str->res->write(ssd1309_str->res,Bit_RESET);
     vTaskDelay(20);
     ssd1309_str->res->write(ssd1309_str->res,Bit_SET);
     
     ssd1309_str->wr_byte(ssd1309_str,0xAE,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xFD,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x12,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xd5,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xA0,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xA8,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x3f,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xD3,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x00,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x40,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xA1,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xC8,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xDA,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x12,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x81,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xBF,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xD9,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x25,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xDB,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0x34,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xA4,OLED_CMD);
     ssd1309_str->wr_byte(ssd1309_str,0xA6,OLED_CMD);
     
     ssd1309_str->clear(ssd1309_str);
      
     ssd1309_str->wr_byte(ssd1309_str,0xAF,OLED_CMD);
     
     
 }
 
 void SSD1309_SPI_STR_INIT(SSD1309_SPI_STR *ssd1309_str)
 {
     ssd1309_str->wr_byte        =SSD1309_SPI_WR;
     ssd1309_str->color_turn     =SSD1309_SPI_COLOR_TURN;
     ssd1309_str->diaplay_turn   =SSD1309_SPI_DISPLAY_TURN;
     ssd1309_str->display_on     =SSD1309_SPI_DISPLAY_ON;
     ssd1309_str->display_off    =SSD1309_SPI_DISPLAY_OFF;
     ssd1309_str->refresh        =SSD1309_SPI_REFRESH;
     ssd1309_str->clear          =SSD1309_SPI_CLEAR;
     ssd1309_str->draw_point     =SSD1309_SPI_DRAW_POINT;
     ssd1309_str->draw_line      =SSD1309_SPI_DRAW_LINE;
     ssd1309_str->draw_circle    =SD1309_DRAW_CIRCLE;
     ssd1309_str->show_char      =SD1309_SHOW_CHAR;
     ssd1309_str->show_str       =SD1309_SHOW_STR;
     ssd1309_str->show_num       =SD1309_SHOW_NUM;
     ssd1309_str->show_chinese   =SD1309_SHOW_CHINESE;
     ssd1309_str->show_pic       =SD1309_SHOW_PICTURE;
     ssd1309_str->init           =SSD1309_SPI_INIT;
     ssd1309_str->init(ssd1309_str);
 }
 
/**************************************************OS***********************************************/


GPIO_STR cs_str   = GPIO_CLASS_CREATE(OLED_CS_PORT,OLED_CS_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz); 
GPIO_STR sclk_str = GPIO_CLASS_CREATE(OLED_SCLK_PORT,OLED_SCLK_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR sdin_str = GPIO_CLASS_CREATE(OLED_SDIN_PORT,OLED_SDIN_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR dc_str   = GPIO_CLASS_CREATE(OLED_DC_PORT,OLED_DC_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR res_str  = GPIO_CLASS_CREATE(OLED_RES_PORT,OLED_RES_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);

SPI_STR  spi_str  = SPI_CLASS_CREATE(&sclk_str,NULL,&sdin_str,&cs_str,mode3);

SSD1309_SPI_STR oled_str = SSD1309_SPI_CLASS_CREATE(&spi_str,&res_str,&dc_str,64,128); 
 
 
 
uint8_t title_init=0;
uint16_t title_refresh_time = 500;
void title_page(SSD1309_SPI_STR *oled_str)
{
    uint8_t temp;
    if(title_init == 0)
    {
        title_refresh_time = 500;
        title_init = 1;
    }
    title_refresh_time++;
    if(title_refresh_time >= 500)
    {      
        /*修改电量*/
        data_pools_read(POWER_CHARGE_ADDR,POWER_CHARGE_LEN,&temp);
        if(temp == 4)
            oled_str->show_pic(oled_str,109,2,17,8,(uint8_t *)POWER_LIN4,1);
        else if(temp == 3)
            oled_str->show_pic(oled_str,109,2,17,8,(uint8_t *)POWER_LIN3,1);
        else if(temp == 2)
            oled_str->show_pic(oled_str,109,2,17,8,(uint8_t *)POWER_LIN2,1);
        else if(temp == 1)
            oled_str->show_pic(oled_str,109,2,17,8,(uint8_t *)POWER_LIN1,1);
        else if(temp == 0)
            oled_str->show_pic(oled_str,109,2,17,8,(uint8_t *)POWER_LIN0,1);
        title_refresh_time = 0;
    }
}
 
 uint8_t logo_init = 0;
 uint16_t show_time = 0;
 void logo_page(SSD1309_SPI_STR *oled_str)
 {
     if(logo_init == 0)
     {   
         show_time = 0;
         logo_init = 1;
         SD1309_SHOW_CHINESEs(oled_str,32,2,"华",16,1);
         SD1309_SHOW_CHINESEs(oled_str,48,2,"夏",16,1);
         SD1309_SHOW_CHINESEs(oled_str,64,2,"高",16,1);
         SD1309_SHOW_CHINESEs(oled_str,80,2,"铁",16,1);
         oled_str->show_str(oled_str,40,29,(uint8_t *)VERSION,16,1);
     }
     show_time++;
     if(show_time == 40)
     {
         uint16_t temp = MAIN_PAGE_ID;
         data_pools_write(POOLS_PAGE_ID_ADDR,2,&temp);
     }
 }
 
uint8_t main_init = 0;
void main_page(SSD1309_SPI_STR *oled_str)
{
    uint16_t temp;
    if(main_init == 0)
    {
        SD1309_SHOW_CHINESEs(oled_str,17,15,"距",16,1);
        oled_str->show_char(oled_str,39,15,'+',16,1);
        oled_str->show_char(oled_str,76,15,'.',16,1);
        SD1309_SHOW_CHINESEs(oled_str,17,40,"平",16,1);
        oled_str->show_char(oled_str,39,40,'+',16,1);
        oled_str->show_char(oled_str,76,40,'.',16,1);
        temp = 99;
        data_pools_write(PING_DATA_ZHENG_ADDR,PING_DATA_ZHENG_LEN,&temp);
        data_pools_write(PING_DATA_FU_ADDR,PING_DATA_FU_LEN,&temp);
        data_pools_write(JU_DATA_FU_ADDR,JU_DATA_FU_LEN,&temp);
        data_pools_write(JU_DATA_ZHENG_ADDR,JU_DATA_ZHENG_LEN,&temp);
        temp = 4;
        data_pools_write(POWER_CHARGE_ADDR,POWER_CHARGE_LEN,&temp);
        
        main_init = 1;
    }
    
    /*修改数值*/
    data_pools_read(JU_DATA_ZHENG_ADDR,JU_DATA_ZHENG_LEN,&temp);
    oled_str->show_num(oled_str,52,15,temp,3,16,1,' ');
    data_pools_read(JU_DATA_FU_ADDR,JU_DATA_FU_LEN,&temp);
    oled_str->show_num(oled_str,84,15,temp,2,16,1,' ');
    data_pools_read(PING_DATA_ZHENG_ADDR,PING_DATA_ZHENG_LEN,&temp);
    oled_str->show_num(oled_str,52,40,temp,3,16,1,' ');
    data_pools_read(PING_DATA_FU_ADDR,PING_DATA_FU_LEN,&temp);
    oled_str->show_num(oled_str,84,40,temp,2,16,1,' ');
    
}

typedef struct
{
    char  *code;
    uint8_t x;
    uint8_t y;
    uint8_t high;
    uint8_t width;
}shortcut_menu_str;

shortcut_menu_str shortcut_menu_ico[]=
{
    {menu_change_mima_ico,30,13,16,67},
    {menu_precision_ico,47,34,16,33},
};

uint8_t shortcut_menu_init = 0,last_shortcut_menu_sw,shortcut_menu_sw;
void shortcut_menu_page(SSD1309_SPI_STR *oled_str)
{
    if(shortcut_menu_init == 0)
    {
        shortcut_menu_init = 1;
        shortcut_menu_sw = 0;
        last_shortcut_menu_sw = 0xFF;
        for(uint8_t i=0;i<2;i++)
        {
            oled_str->show_pic(oled_str,shortcut_menu_ico[i].x,shortcut_menu_ico[i].y,\
            shortcut_menu_ico[i].width,shortcut_menu_ico[i].high,(uint8_t*)shortcut_menu_ico[i].code,1);
            
        }
    }
    data_pools_read(SHORTCUT_MENU_ID_ADDR,SHORTCUT_MENU_ID_LEN,&shortcut_menu_sw);
    if(last_shortcut_menu_sw != shortcut_menu_sw)
    {
        for(uint8_t i=0;i<SHORTCUT_MENU_NUM;i++)
        {
            oled_str->show_pic(oled_str,shortcut_menu_ico[i].x,shortcut_menu_ico[i].y,\
            shortcut_menu_ico[i].width,shortcut_menu_ico[i].high,(uint8_t*)shortcut_menu_ico[i].code,1);
        }
        oled_str->show_pic(oled_str,shortcut_menu_ico[shortcut_menu_sw].x,shortcut_menu_ico[shortcut_menu_sw].y,\
        shortcut_menu_ico[shortcut_menu_sw].width,shortcut_menu_ico[shortcut_menu_sw].high,(uint8_t*)shortcut_menu_ico[shortcut_menu_sw].code,0);
        last_shortcut_menu_sw = shortcut_menu_sw;
    }
}
 
uint8_t checkup_init = 0 ;
void checkup_page(SSD1309_SPI_STR *oled_str)
{
    if(checkup_init == 0)
    {
        SD1309_SHOW_CHINESEs(oled_str,17,15,"查",16,1);
        oled_str->show_char(oled_str,39,15,'+',16,1);
        oled_str->show_char(oled_str,76,15,'.',16,1);
        SD1309_SHOW_CHINESEs(oled_str,17,40,"护",16,1);
        oled_str->show_char(oled_str,39,40,'+',16,1);
        oled_str->show_char(oled_str,76,40,'.',16,1);
        checkup_init = 1;
    }
    
}
 
uint8_t mubiao_init = 0,last_mubiao_select = 0xFF;
void mubiao_page(SSD1309_SPI_STR *oled_str)
{
    uint16_t temp = 0;
    if(mubiao_init == 0)
    {
        mubiao_init = 1;
        last_mubiao_select = 0xFF;
        SD1309_SHOW_CHINESEs(oled_str,17,15,"距",16,1);
        oled_str->show_char(oled_str,39,15,'+',16,1);
        SD1309_SHOW_CHINESEs(oled_str,17,40,"平",16,1);
        oled_str->show_char(oled_str,39,40,'+',16,1);
        
    }
    data_pools_read(MUBIAO_SELECT_ADDR,MUBIAO_SELECT_LEN,&temp);
    if(temp != last_mubiao_select)
    {
        last_mubiao_select = temp;
        data_pools_read(MUBIAO_ONE_ADDR,MUBIAO_ONE_LEN,&temp);
        oled_str->show_num(oled_str,47,15,temp,1,16,1,'0');
        data_pools_read(MUBIAO_ONE_ADDR,MUBIAO_ONE_LEN,&temp);
        oled_str->show_num(oled_str,55,15,temp,1,16,1,'0');
        SD1309_SHOW_CHINESEs(oled_str,63,15,"±",16,1);
        data_pools_read(MUBIAO_TWO_ADDR,MUBIAO_TWO_LEN,&temp);
        oled_str->show_num(oled_str,78,15,temp,1,16,1,'0');
        data_pools_read(MUBIAO_TWO_ADDR,MUBIAO_TWO_LEN,&temp);
        oled_str->show_num(oled_str,86,15,temp,1,16,1,'0');
        
        data_pools_read(MUBIAO_THR_ADDR,MUBIAO_THR_LEN,&temp);
        oled_str->show_num(oled_str,47,40,temp,1,16,1,'0');
        data_pools_read(MUBIAO_THR_ADDR,MUBIAO_THR_LEN,&temp);
        oled_str->show_num(oled_str,55,40,temp,1,16,1,'0');
        data_pools_read(MUBIAO_THR_ADDR,MUBIAO_THR_LEN,&temp);
        oled_str->show_num(oled_str,63,40,temp,1,16,1,'0');
        SD1309_SHOW_CHINESEs(oled_str,71,40,"±",16,1);
        data_pools_read(MUBIAO_FOU_ADDR,MUBIAO_FOU_LEN,&temp);
        oled_str->show_num(oled_str,87,40,temp,1,16,1,'0');
        data_pools_read(MUBIAO_FOU_ADDR,MUBIAO_FOU_LEN,&temp);
        oled_str->show_num(oled_str,95,40,temp,1,16,1,'0');
        switch(last_mubiao_select)
        {
            case 0:
                oled_str->show_num(oled_str,47,15,temp,1,16,0,'0');
                break;
            case 1:
                oled_str->show_num(oled_str,55,15,temp,1,16,0,'0');
                break;
            case 2:
                oled_str->show_num(oled_str,78,15,temp,1,16,0,'0');
                break;
            case 3:
                oled_str->show_num(oled_str,86,15,temp,1,16,0,'0');
                break;
            case 4:
                oled_str->show_num(oled_str,47,40,temp,1,16,0,'0');
                break;
            case 5:
                oled_str->show_num(oled_str,55,40,temp,1,16,0,'0');
                break;
            case 6:
                oled_str->show_num(oled_str,63,40,temp,1,16,0,'0');
                break;
            case 7:
                oled_str->show_num(oled_str,87,40,temp,1,16,0,'0');
                break;
            case 8:
                oled_str->show_num(oled_str,95,40,temp,1,16,0,'0');
                break;
        }
    }
}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
void Oled_task(void *pvParameters)
{   
    uint16_t last_page = 0xFF;
    oled_str.str_init(&oled_str);
    uint16_t page_id;
    while(1)
    {
         data_pools_read(POOLS_PAGE_ID_ADDR,2,&page_id);
         if(last_page != page_id)
         {
             logo_init = 0;
             main_init = 0;
             title_init = 0;
             mubiao_init = 0;
             shortcut_menu_init = 0;
             checkup_init = 0;
             oled_str.clear(&oled_str);
             last_page = page_id;
         }
         /*刷新抬头*/
         if(page_id == MAIN_PAGE_ID || page_id == CHECKUP_PAGE_ID)
             title_page(&oled_str);
         /*执行具体页面任务*/
         switch(page_id)
         {
             case LOGO_PAGE_ID:
                 logo_page(&oled_str);
             break;
             case MAIN_PAGE_ID:
                 main_page(&oled_str);
                 break;
             case SHORTCUT_MENU_PAGE_ID:
                 shortcut_menu_page(&oled_str);
                 break;
             case CHECKUP_PAGE_ID:
                 checkup_page(&oled_str);
             break;
             case MUBIAO_PAGE_ID:
                 mubiao_page(&oled_str);
             break;
         }
         /*更新屏幕*/
         oled_str.refresh(&oled_str);
         vTaskDelay(5);
    }
}

 
void Oled_Task_Create(void)
{
    //任务句柄
    TaskHandle_t OledTask_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )Oled_task,
                (const char*    )"Oled_task",
                (uint16_t       )OLED_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )OLED_TASK_PRIO,
                (TaskHandle_t*  )&OledTask_Handler);
}


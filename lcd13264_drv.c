#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> 
#include <linux/miscdevice.h> 
#include <linux/mm.h> //mmap
#include <linux/gpio.h>
#include <linux/delay.h>

//声明描述按键信息的数据结构
struct lcd_resource {
    char *name;
    int gpio;
};

#define uchar           unsigned char
#define DEBUG           //
#define LCD_GPIO_CSB    45      //GPIO1_13
#define LCD_GPIO_RESET  50      //GPIO1_18
#define LCD_GPIO_A0     56      //4G_RESET_2
#define LCD_GPIO_LEDA   47      //4G_RESET_1
#define LCD_GPIO_SCL    57      //4G_WAKEUP_OUT_2
#define LCD_GPIO_SDA    60      //4G_WAKEUP_OUT_1

#define DELAY_NUM       1
#define SDA_OUT(x)      gpio_direction_output(LCD_GPIO_SDA,x)
#define SDA_IN()        gpio_direction_input(LCD_GPIO_SDA)
#define IIC_SDA(x)      gpio_set_value(LCD_GPIO_SDA,x)
#define IIC_SCL(x)      gpio_set_value(LCD_GPIO_SCL,x)
#define READ_SDA        gpio_get_value(LCD_GPIO_SDA)

#define CSB(x)          gpio_set_value(LCD_GPIO_CSB,x)
#define RESET(x)        gpio_set_value(LCD_GPIO_RESET,x)
#define A0(x)           gpio_set_value(LCD_GPIO_A0,x)
#define LEDA(x)         gpio_set_value(LCD_GPIO_LEDA,x)

//定义初始化按键的硬件信息对象
static struct lcd_resource lcd_info[] = {
    {   .name = "LCD_GPIO_CSB",     .gpio = LCD_GPIO_CSB    },
    {   .name = "LCD_GPIO_RESET",   .gpio = LCD_GPIO_RESET  },
    {   .name = "LCD_GPIO_A0",      .gpio = LCD_GPIO_A0     },
    {   .name = "LCD_GPIO_LEDA",    .gpio = LCD_GPIO_LEDA   },
    {   .name = "LCD_GPIO_SCL",     .gpio = LCD_GPIO_SCL    },
    {   .name = "LCD_GPIO_SDA",     .gpio = LCD_GPIO_SDA    },
};

static u8 buf[132*8];

void IIC_Start(void)
{
    SDA_OUT(1);
    IIC_SCL(1);
    udelay(DELAY_NUM);
    IIC_SDA(0);
    udelay(DELAY_NUM);
    IIC_SCL(0);
}

void IIC_Stop(void)
{
    SDA_OUT(0);
    IIC_SCL(0);
    udelay(DELAY_NUM);
    IIC_SCL(1);
    IIC_SDA(1);
    udelay(DELAY_NUM);
}

uchar IIC_Wait_Ack(void)
{
    u8 ucErrTime=0;
	SDA_IN();      
	IIC_SDA(1);udelay(1);	   
	IIC_SCL(1);udelay(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL(0);	   
	return 0; 
}

void IIC_Ack(void)
{
	IIC_SCL(0);
	SDA_OUT(0);
	udelay(DELAY_NUM);
	IIC_SCL(1);
	udelay(DELAY_NUM);
	IIC_SCL(0);
}

void IIC_NAck(void)
{
	IIC_SCL(0);
	SDA_OUT(1);
	udelay(DELAY_NUM);
	IIC_SCL(1);
	udelay(DELAY_NUM);
	IIC_SCL(0);
}

void IIC_Send_Byte(u8 txd)
{                        
    u8 t;   		    
    IIC_SCL(1);
    for(t=0;t<8;t++)
    {              
		if((txd&0x80)>>7)
			IIC_SDA(1);
		else
			IIC_SDA(0);
		txd<<=1; 	
		IIC_SCL(0);
		udelay(DELAY_NUM);   
		IIC_SCL(1);
		udelay(DELAY_NUM); 
    }	 
} 

// u8 IIC_Read_Byte(unsigned char ack)
// {
// 	unsigned char i,receive=0;
// 	SDA_IN();
//     for(i=0;i<8;i++ )
// 	{
//         IIC_SCL(0); 
//         udelay(DELAY_NUM);
// 		IIC_SCL(1);
//         receive<<=1;
//         if(READ_SDA)receive++;   
// 		udelay(1); 
//     }					 
//     if (!ack)
//         IIC_NAck();
//     else
//         IIC_Ack(); 
//     return receive;
// }

void lcdA(char turn)
{
    gpio_set_value(LCD_GPIO_LEDA,turn&0x01);
}

void lcd_send_cmd(uchar lcd_cmd)
{
    A0(0);
    CSB(0);
    IIC_Send_Byte(lcd_cmd);
    CSB(1);
}

void lcd_send_data(uchar lcd_data)
{
    A0(1);
    CSB(0);
    IIC_Send_Byte(lcd_data);
    CSB(1);
}

void set_lcdpage_address(uchar page_addr)
{
    lcd_send_cmd(page_addr|0xb0);
}

void set_lcdcolumn_address(uchar clm_addr)
{
	lcd_send_cmd((clm_addr>>4)|0x10);
	lcd_send_cmd(clm_addr&0x0f);
}


void disp_graphics(u8 *gph)
{
	u8 i,j;
	for(i=4;i<8;i++)
	{
		set_lcdpage_address(i-4);
		set_lcdcolumn_address(0x00);
		for(j=0;j<132;j++)
		{
			lcd_send_data(*(gph+i*132+j));
		}
	}
	for(i=0;i<4;i++)
	{
		set_lcdpage_address(i+4);
		set_lcdcolumn_address(0x00);
		for(j=0;j<132;j++)
		{
			lcd_send_data(*(gph+i*132+j));
		}
	}
}

// void disp_lattice(uchar lcm_data1,uchar lcm_data2)
// {
// 	uchar i,j;
// 	for(i=0;i<8;i++)
// 	{
// 		set_lcdpage_address(i);
// 		set_lcdcolumn_address(0x00);
// 		for(j=0;j<66;j++)
// 		{
// 			lcd_send_data(lcm_data1);
// 			lcd_send_data(lcm_data2);
// 		}
// 	}
// }

void lcd13264_init(void)
{
    mdelay(5);
    RESET(0);
    mdelay(20);
    RESET(1);
    mdelay(20);
    DEBUG("%s %d\n",__func__,__LINE__);
    lcd_send_cmd(0xE2);		//系统重置
    mdelay(100);
    lcd_send_cmd(0x40);		//设置显示从第0行开始
    lcd_send_cmd(0xA0);		//设置SEG方向
    lcd_send_cmd(0xC8);		//设置COM方向
    lcd_send_cmd(0xA2);		//A3 设置偏差= 1/7 A2 设置偏差= 1/9
    lcd_send_cmd(0x2F);		//Boost，稳压器，跟随器
    lcd_send_cmd(0xF8);		//设置助推器比率
    lcd_send_cmd(0x27);		//设定调节比（3）
    lcd_send_cmd(0x81);		//设置电压等级
    lcd_send_cmd(28);		//电压值
    lcd_send_cmd(0xA6);		//0xA6 禁用反向显示 0xA7 反向显示
    lcd_send_cmd(0xAF);		//设置显示启用
    //lcd_send_cmd(0xA5);		//A5 display all		A4 normal display
    DEBUG("%s %d\n",__func__,__LINE__);
}



static int lcd_open(struct inode *inode, struct file * fl)
{
    int i;
    //申请GPIO口
    for(i = 0; i < ARRAY_SIZE(lcd_info); i++) {
        gpio_request(lcd_info[i].gpio,lcd_info[i].name);
        gpio_direction_output(lcd_info[i].gpio,1);
    }
    lcd13264_init();
    //disp_lattice(0xff,0x00);
    return 0;
}
static int lcd_release(struct inode *inode, struct file * fl)
{
    int i;
    //注销GPIO口
    for(i = 0; i < ARRAY_SIZE(lcd_info); i++) {
        gpio_free(lcd_info[i].gpio); 
    }
    return 0;
}
ssize_t lcd_read(struct file *file, const char __user *buffer,
				  size_t count, loff_t *pos)
{
    return 0;
}
static ssize_t lcd_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *pos)
{
    if (copy_from_user(buf, buffer, count))
		goto out;
    disp_graphics(buf);
    return 0;

    out:
        return -1;
}
static long lcd_ioctl(struct file *fl, unsigned int cmd, unsigned long data)
{
    return 0;
}

//定义初始化硬件操作接口对象
static struct file_operations lcd_fops = {
    .owner		    =   THIS_MODULE,
    .open		    =	lcd_open,
	.release	    =	lcd_release,
	.read		    =	lcd_read,
	.write		    =	lcd_write,
	.unlocked_ioctl	=	lcd_ioctl,
};

//定义初始化混杂设备对象
static struct miscdevice lcd_misc = {
    .name = "lcd13264",             //内核自动创建/dev/lcd13264
    .minor = MISC_DYNAMIC_MINOR,    //内核帮你分配次设备号
    .fops = &lcd_fops               //添加硬件操作接口
};

static int lcd_init(void)
{
    misc_register(&lcd_misc);
    return 0;
}

static void lcd_exit(void)
{
    misc_deregister(&lcd_misc);
}
module_init(lcd_init);
module_exit(lcd_exit);
MODULE_LICENSE("GPL");
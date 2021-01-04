#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

/*disclaimer: not all of Raspberry pins
 * are available to be used as GPIOs */
#define CHARDEV_MINOR        19   /* start of minor numbers requested */
#define CHARDEV_MINOR_NUM    1    /* how many minors requested */
#define DEVICE_NAME "display_lkm" /* name that will be assigned to this device in /dev fs */
#define BUF_SIZE 512
#define NUM_COM 4 /* number of commands that this driver support */

#define LCDWIDTH 84
#define LCDHEIGHT 48

#define BL_GPIO 26
#define CLCK_GPIO 11
#define RST_GPIO 24
#define CE_GPIO 8
#define DC_GPIO 23
#define DIN_GPIO 10

#define PCD8544_POWERDOWN 0x04
#define PCD8544_ENTRYMODE 0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01

#define PCD8544_DISPLAYBLANK 0x0
#define PCD8544_DISPLAYNORMAL 0x4
#define PCD8544_DISPLAYALLON 0x1
#define PCD8544_DISPLAYINVERTED 0x5

// H = 0
#define PCD8544_FUNCTIONSET 0x20
#define PCD8544_DISPLAYCONTROL 0x08
#define PCD8544_SETYADDR 0x40
#define PCD8544_SETXADDR 0x80

// H = 1
#define PCD8544_SETTEMP 0x04
#define PCD8544_SETBIAS 0x10
#define PCD8544_SETVOP 0x80

char matrix [] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x40, 0xf0, 0xe0, 0xf4, 0xd4, 0x98, 0x4c, 0x08, 0x18, 0x84, 0x28, 0x00, 0x10, 0x10, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x68, 0x40, 0x60, 0x68, 0xe0, 0x68, 0x60, 0xe8, 0xd2, 0x76, 0xd0, 0x4d, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x02, 0x2d, 0x64, 0x19, 0x1e, 0xff, 0xff, 0x4f, 0x7e, 0x61, 0x42, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x94, 0x64, 0x41, 0x54, 0x20, 0xa0, 0x40, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x01, 0x00, 0x04, 0x00, 0x00, 0x10, 0x20, 0x10, 0x09, 0x06, 0x03, 0x05, 0x7d, 0x07, 0x0a, 0x0e, 0x0a, 0x18, 0x16, 0x00, 0x00, 0x00, 0x10, 0x84, 0x00, 0x1a, 0x1c, 0x43, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x40, 0x22, 0x40, 0x18, 0x61, 0x42, 0x38, 0xa6, 0x52, 0xab, 0x94, 0x7d, 0xa6, 0xd4, 0x6b, 0xa8, 0xaa, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x80, 0x90, 0x61, 0x28, 0x88, 0x90, 0x00, 0x1c, 0x00, 0x30, 0x05, 0x50, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x10, 0x7e, 0x7e, 0xaa, 0xb2, 0x10, 0x18, 0x02, 0x08, 0x60, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04, 0x01, 0x08, 0x11, 0x66, 0x36, 0xdd, 0xd7, 0xfe, 0xf5, 0xdd, 0x7f, 0xfa, 0xf8, 0xe0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x01, 0x04, 0x00, 0x18, 0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x29, 0x28, 0x80, 0x10, 0x00, 0xa6, 0xed, 0x7f, 0xfe, 0xee, 0xf8, 0x70, 0xe8, 0x20, 0x92, 0xa9, 0xd8, 0x80, 0x94, 0x04, 0x08, 0x00, 0x40, 0x22, 0x08, 0x02, 0x01, 0x01, 0x07, 0x03, 0x01, 0x03, 0x61, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf8, 0xf8, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xe0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x40, 0x60, 0xa0, 0xe0, 0x90, 0x10, 0x50, 0xa8, 0x50, 0x98, 0x60, 0x00, 0x80, 0x40, 0x22, 0x5a, 0xa5, 0x5a, 0xad, 0xda, 0xfd, 0xfb, 0xdf, 0xbf, 0xca, 0xfd, 0x52, 0xa9, 0x96, 0xb5, 0xdf, 0xff, 0xff, 0xfa, 0xf8, 0xe0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x20, 0x80, 0x10, 0x90, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
     };

/* buffer with set of supported commands */
const char * commands[NUM_COM] = {"on", "off", "bl_on", "bl_off"};
/* enumerators to match commands with values for following processing */
enum commands { on  = 0,
                off   = 1,
                bl_on = 2,
                bl_off = 3,
                na       = NUM_COM+1};

//enum direction {in, out};
enum state {poweron, poweroff};
enum blight {lighton, lightoff};

struct display_lkm_dev
{
    struct cdev cdev;

//    struct gpio pin;
    enum state state;
    enum blight blight;
};

static int display_lkm_open(struct inode *inode, struct file *filp);
static int display_lkm_release(struct inode *inode, struct file *filp);
static ssize_t display_lkm_read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t display_lkm_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos);


static struct file_operations display_lkm_fops =
{
    .owner = THIS_MODULE,
    .open = display_lkm_open,
    .release = display_lkm_release,
    .read = display_lkm_read,
    .write = display_lkm_write,
};

static int display_lkm_init(void);
static void display_lkm_exit(void);

static dev_t first;
struct display_lkm_dev display_device;
static struct class *cd_class;

static unsigned int which_command(const char * com)
{
    unsigned int i;

    for(i = 0 ; i < NUM_COM; i++)
    {
        if(!strcmp(com, commands[i]))
            return i;
    }
    return na;
}

static int display_lkm_open (struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "[display_lkm_open] - open() method called\n");
    return 0;
}

static int display_lkm_release (struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "[display_lkm_release] - close() method called\n");
    return 0;
}

static ssize_t display_lkm_read ( struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_DEBUG "[display_lkm_release] - read() method called\n");
    return 0;
}

void SPIwrite(int c)
{
    gpio_set_value(CE_GPIO, 0);
    int i;
    int j;
    for (i = 0; i < 8; i++)  {
        gpio_set_value(DIN_GPIO, !!(c & (1 << (7 - i))));

        gpio_set_value(CLCK_GPIO, 1);
        for (j = 400; j > 0; j--); // clock speed, anyone? (LCD Max CLK input: 4MHz)
        gpio_set_value(CLCK_GPIO, 0);
    }
    gpio_set_value(CE_GPIO, 1);
}

void SPIcommand(int c)
{
    gpio_set_value(DC_GPIO, 0);
    SPIwrite(c);
}

void SPIdata(int c)
{
    gpio_set_value(DC_GPIO, 1);
    SPIwrite(c);
}

void display(void)
{
    int col, maxcol, p;

    for(p = 0; p < 6; p++)
    {
        SPIcommand(PCD8544_SETYADDR | p);
        // start at the beginning of the row
        col = 0;
        maxcol = LCDWIDTH-1;
        SPIcommand(PCD8544_SETXADDR | col);
        for(; col <= maxcol; col++) {
            //uart_putw_dec(col);
            //uart_putchar( );

            SPIdata(matrix[(LCDWIDTH * p) + col]);
        }
    }
    SPIcommand(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?

}

static ssize_t display_lkm_write ( struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned int gpio, len = 0;
    int i;
//    char kbuf[BUF_SIZE];

    len = count < BUF_SIZE ? count-1 : BUF_SIZE-1;

    /* one more special kernel macro to copy data between
     * user space memory and kernel space memory
     */
    memset(matrix, 0, BUF_SIZE);
    if(raw_copy_from_user(matrix, buf, len) != 0)
        return -EFAULT;

    matrix[len] = '\0';

    display();

    /* perform a switch on recieved command value
     * to determine, what request is received
     */
//    switch(which_command(kbuf))
//    {
//    case on:
//    {
//        if (display_device.state == poweron)
//        {
//            printk("[display_lkm_release] - Cannot power display on, it is enabled already\n", gpio);
//            return -EPERM;
//        }
//        else
//        {
//            display_device.state = poweron;
//            printk("[display_lkm_release] - Power display on\n");
//            display();
//        }
//        break;
//    }
//    case off:
//    {
//        if (display_device.state == poweroff)
//        {
//            printk("[display_lkm_release] - Cannot power display off, it is disabled already\n", gpio);
//            return -EPERM;
//        }
//        else
//        {
//            display_device.state = poweroff;
//            printk("[display_lkm_release] - Power display off\n");
//            display();
//        }
//        break;
//    }
//    case bl_on:
//    {
//        if (display_device.blight == lighton)
//        {
//            printk("[display_lkm_release] - Cannot enable BL, it is enabled already\n", gpio);
//            return -EPERM;
//        }
//        else
//        {
//            printk("[display_lkm_release] - Enable light\n");
////            gpio_direction_output(BL_GPIO, 1);
//            gpio_set_value(BL_GPIO, 1);
//            display_device.blight = lighton;
//        }
//        break;
//    }
//    case bl_off:
//        if (display_device.blight == lightoff)
//        {
//            printk("[display_lkm_release] - Cannot disable BL, it is disable already\n", gpio);
//            return -EPERM;
//        }
//        else
//        {
//            printk("[display_lkm_release] - Disable light\n");
//            //    gpio_direction_output(BL_GPIO, 1);
//            gpio_set_value(BL_GPIO, 0);
//            display_device.blight = lightoff;
//        }
//        break;
//    }

    *f_pos += count;
    return count;
}

void setupDisplay(void){
    SPIcommand(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );

    SPIcommand(PCD8544_SETBIAS | 0x4);

    int contrast = 55;
    if (contrast > 0x7f)
        contrast = 0x7f;

    SPIcommand( PCD8544_SETVOP | contrast);

    SPIcommand(PCD8544_FUNCTIONSET);

    SPIcommand(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

    display();
}

static int __init display_lkm_init(void)
{
    int ret;
    struct device *dev_ret;
//    struct device *dev_ret;

    printk(KERN_DEBUG "[display_lkm_init] - init functions called");
    /* allocate minor numbers */
    if ((ret = alloc_chrdev_region(&first, CHARDEV_MINOR, CHARDEV_MINOR_NUM, DEVICE_NAME)) < 0)
    {
        return ret;
    }

    if (IS_ERR(cd_class = class_create(THIS_MODULE, DEVICE_NAME)))
    {
        printk(KERN_DEBUG "Cannot create class %s\n", DEVICE_NAME);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cd_class);
    }

    display_device.blight = lighton;
    display_device.state = poweron;
    display_device.cdev.owner = THIS_MODULE;

    gpio_direction_output(BL_GPIO, 0);
    gpio_direction_output(CLCK_GPIO, 0);
    gpio_direction_output(DIN_GPIO, 0);
    gpio_direction_output(CE_GPIO, 0);
    gpio_direction_output(DC_GPIO, 0);
    gpio_direction_output(RST_GPIO, 0);

    int j;
    for (j = 400; j > 0; j--);
    gpio_set_value(RST_GPIO, 1);
    gpio_set_value(CE_GPIO, 1);
    gpio_set_value(BL_GPIO, 1);

    setupDisplay();

    cdev_init(&display_device.cdev, &display_lkm_fops);

    if ((ret = cdev_add(&display_device.cdev, first, 1)) < 0)
    {
        device_destroy(cd_class, first);
        class_destroy(cd_class);
        unregister_chrdev_region(first, 1);
        return ret;
    }

    if (IS_ERR(dev_ret = device_create(cd_class, NULL, first, NULL, "Nokia5110LCD")))
    {
        class_destroy(cd_class);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    printk("[display_lkm_init] - Driver initialized\n");
    
    return 0;
}

static void __exit display_lkm_exit(void)
{
    cdev_del(&display_device.cdev);
    device_destroy(cd_class, first);
    class_destroy(cd_class);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "[display_lkm_init] - unregistered from kernel");
}

module_init(display_lkm_init);
module_exit(display_lkm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wino Team");
MODULE_DESCRIPTION("Nokia 5110 LCD Display Kernel Module - Linux device driver for Raspberry Pi");

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define DEVICE_NAME "PLAYER"
#define CLASS_NAME "RPLAYER"

#define CHARDEV_MINOR        19
#define CHARDEV_MINOR_NUM    1


#define PREVIOUS_GPIO 5
#define PLAY_PAUSE_GPIO 6
#define NEXT_GPIO 13
#define VOL_DOWN_GPIO 19
#define VOL_UP_GPIO 26
#define BUFF_SIZE 512

#define BUTTONS_NUM 5
int BUTTONS[5] = {PREVIOUS_GPIO, PLAY_PAUSE_GPIO, NEXT_GPIO, VOL_DOWN_GPIO, VOL_UP_GPIO};

int count = 0;

const long MIN_INTERRUPT_DELAY_NS = 1000000000;

char playerBuffer[BUFF_SIZE] = "empty";

ktime_t lastInterrupt, now;

static dev_t first;
static struct cdev c_dev;
static struct class *cd_class;

static irqreturn_t button_handler(int irq, void * ident)
{
    now = ktime_get();
    s64 difference = ktime_to_ns(ktime_sub(now, lastInterrupt));

    if ((long long)difference >= MIN_INTERRUPT_DELAY_NS) {
        memset(playerBuffer, 0, BUFF_SIZE);
        if(count == 0){
            count = 1;
        } else {
            count = 0;
        }
        if(irq == gpio_to_irq(PREVIOUS_GPIO)){
            if(raw_copy_from_user(playerBuffer, "previous", 8) != 0)
                return -EFAULT;
            playerBuffer[8] = count + '0';
            playerBuffer[9] = '\0';
        } else if(irq == gpio_to_irq(PLAY_PAUSE_GPIO)){
            if(raw_copy_from_user(playerBuffer, "pp", 2) != 0)
                return -EFAULT;
            playerBuffer[2] = count + '0';
            playerBuffer[3] = '\0';
        } else if(irq == gpio_to_irq(NEXT_GPIO)) {
            if(raw_copy_from_user(playerBuffer, "next", 4) != 0)
                return -EFAULT;
            playerBuffer[8] = count + '0';
            playerBuffer[4] = '\0';
        }
    }

    return IRQ_HANDLED;
}

static int player_open(struct inode *i, struct file *f)
{
    printk(KERN_DEBUG "[player] - open() method called\n");
    return 0;
}

static int player_release(struct inode *i, struct file *f)
{
    printk(KERN_DEBUG "[player] - close() method called\n");
    return 0;
}

static ssize_t player_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    int index = 0;
    while (playerBuffer[index] != '\0'){
        if(put_user(playerBuffer[index], buf+index))
            break;
        index++;
    }
    put_user('\0', buf+index);
    return index;
}

static ssize_t player_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_DEBUG "[player] - write() method called\n");
    return len;
}

static struct file_operations player_fops =
        {
                .owner = THIS_MODULE,
                .open = player_open,
                .release = player_release,
                .read = player_read,
                .write = player_write
        };

static int __init button_init (void)
{
    int i = 0, j;
    int err;

    int ret;
    struct device *dev_ret;

    /* allocate minor numbers */
    if ((ret = alloc_chrdev_region(&first, CHARDEV_MINOR, CHARDEV_MINOR_NUM, DEVICE_NAME)) < 0)
    {
        return ret;
    }
    /* create class for device */
    if (IS_ERR(cd_class = class_create(THIS_MODULE, CLASS_NAME)))
    {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cd_class);
    }

    /* create device */
    if (IS_ERR(dev_ret = device_create(cd_class, NULL, first, NULL, DEVICE_NAME)))
    {
        class_destroy(cd_class);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    /* init cdev sctructure */
    cdev_init(&c_dev, &player_fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0)
    {
        device_destroy(cd_class, first);
        class_destroy(cd_class);
        unregister_chrdev_region(first, 1);
        return ret;
    }

    lastInterrupt = ktime_get();

    printk("button_lkm initialized \n");
    for(i = 0; i < BUTTONS_NUM; ++i){
        if ((err = gpio_request(BUTTONS[i], THIS_MODULE->name)) !=0) {
            for(j = 0; j < i; ++j){
                gpio_free(BUTTONS[j]);
                free_irq(gpio_to_irq(BUTTONS[j]), THIS_MODULE->name);
            }
            cdev_del(&c_dev);
            device_destroy(cd_class, first);
            class_destroy(cd_class);
            unregister_chrdev_region(first, 1);
            return err;
        }

        if ((err = gpio_direction_input(BUTTONS[i])) !=0)
        {
            for(j = 0; j < i; ++j){
                gpio_free(BUTTONS[j]);
                free_irq(gpio_to_irq(BUTTONS[j]), THIS_MODULE->name);
            }
            gpio_free(BUTTONS[i]);
            cdev_del(&c_dev);
            device_destroy(cd_class, first);
            class_destroy(cd_class);
            unregister_chrdev_region(first, 1);
            return err;
        }

        if((err = request_irq(gpio_to_irq(BUTTONS[i]), button_handler, IRQF_SHARED | IRQF_TRIGGER_RISING, THIS_MODULE->name, THIS_MODULE->name)) != 0)
        {
            for(j = 0; j < i; ++j){
                gpio_free(BUTTONS[j]);
                free_irq(gpio_to_irq(BUTTONS[j]), THIS_MODULE->name);
            }
            gpio_free(BUTTONS[i]);
            cdev_del(&c_dev);
            device_destroy(cd_class, first);
            class_destroy(cd_class);
            unregister_chrdev_region(first, 1);
            return err;
        }
    }
    printk(KERN_INFO "Waiting for interrupts ... \n");

    return 0;

}


static void __exit button_exit (void)
{
    int j = 0;
    for(j = 0; j < BUTTONS_NUM; ++j){
        gpio_free(BUTTONS[j]);
        free_irq(gpio_to_irq(BUTTONS[j]), THIS_MODULE->name);
    }

    cdev_del(&c_dev);
    device_destroy(cd_class, first);
    class_destroy(cd_class);
    unregister_chrdev_region(first, 1);
}


module_init(button_init);
module_exit(button_exit);

MODULE_AUTHOR("Slavek");
MODULE_DESCRIPTION("GPIO interrupt");
MODULE_LICENSE("GPL");
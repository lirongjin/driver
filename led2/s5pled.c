#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>


/*配置使用静态地址映射或者是动态地址映射*/
#define CONFIG_STATIC_MAP
//#define CONFIG_DYNAMIC_MAP

#define LED_NAME        "s5pled"
#define LED_DEV_MAJOR   250
#define LED_DEV_BASE    0
#define LED_DEV_NUM     3   

#define LED_GPIO_CON        (S5PV210_GPJ0CON)
#define LED_GPIO_DAT        (S5PV210_GPJ0DAT)

#define LED_GPIO_CON_PA     (0xe0200240)
#define LED_GPIO_DAT_PA     (0xe0200244)

#if defined(CONFIG_STATIC_MAP)
# define rLED_GPIO_CON      (*(volatile unsigned int *)LED_GPIO_CON)
# define rLED_GPIO_DAT      (*(volatile unsigned int *)LED_GPIO_DAT)
#elif defined(CONFIG_DYNAMIC_MAP)
# define rLED_GPIO_CON      (*(volatile unsigned int *)led_con)
# define rLED_GPIO_DAT      (*(volatile unsigned int *)led_dat)
#endif

/*3个led灯在GPIO_DAT寄存器中的bit位*/
#define LED0            3
#define LED1            4
#define LED2            5

/*led打开关闭操作的宏定义和查看led状态的宏定义*/
#define led_on(bit)         ({rLED_GPIO_CON |= 1 << (bit); \
                                rLED_GPIO_DAT &= ~(1 << (bit)); \
                            })
#define led_off(bit)        ({rLED_GPIO_CON |= 1 << (bit); \
                                rLED_GPIO_DAT |= (1 << (bit)); \
                            })      
#define led_status(bit)     ({rLED_GPIO_CON &= ~(1 << (bit)); \
                                rLED_GPIO_DAT & (1 << (bit)) ? 0 : 1; \
                            })

    
/*led设备文件的打开、关闭、读和写函数的声明*/
static int led_open(struct inode *inode, struct file *file);
static int led_release(struct inode *inode, struct file *file);
static ssize_t led_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
static ssize_t led_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos);

#ifdef CONFIG_DYNAMIC_MAP
static volatile unsigned int *led_con = NULL, *led_dat = NULL;
#endif

static dev_t led_dev = 0;
static struct cdev *led_cdev;
static struct class *led_class;

/*led文件操作结构体定义，非常关键!!!*/
static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write
};


/*
 * function: led设备文件打开函数定义。主要作用是在使用动态地址映射时进行动态地址映射操作。
 * inode: 节点指针
 * file: 文件指针
 * ret: 成功返回0，否则返回-1。
 **/
static int led_open(struct inode *inode, struct file *file)
{
#ifdef CONFIG_DYNAMIC_MAP
    /*申请和映射操作led的IO资源*/
    if(!request_mem_region(LED_GPIO_CON_PA, 4, "led_con") || !request_mem_region(LED_GPIO_CON_PA, 4, "led_dat"))
    {
        return -EBUSY;
    }
    
    led_con = ioremap(LED_GPIO_CON_PA, 4);
    if(!led_con)
    {
        return -EBUSY;
    }
    led_dat = ioremap(LED_GPIO_DAT_PA, 4);
    if(!led_dat)
    {
        return -EBUSY;
    }
#endif
  
    return 0;
}

/*
 * function: led设备文件关闭函数定义。主要作用是在使用动态地址映射时进行动态地址解除映射操作。
 * inode: 节点指针
 * file: 文件指针
 * ret: 成功返回0，否则返回-1。
 **/
static int led_release(struct inode *inode, struct file *file)
{
#ifdef CONFIG_DYNAMIC_MAP
    /*解除映射和释放led的IO资源*/
    iounmap(led_con);
    iounmap(led_dat);
    release_mem_region(LED_GPIO_CON_PA, 4);
    release_mem_region(LED_GPIO_DAT_PA, 4);
#endif
    
    return 0;
}

/*
 * function: led设备文件读函数定义。可以用来获取led的当前状态
 * file: 文件指针
 * ubuf: 用户应用层buf地址指针
 * count: 用户应用层buf大小
 * ppos: 位置指针
 * ret: 成功返回0, 否则返回-1。
 **/
static ssize_t led_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    int ret;
    char kbuf[20] = {0};

  
    printk(KERN_DEBUG "led_read start\n");
    
    if(count > sizeof(kbuf))
    {
        count = sizeof(kbuf);
    }
    
    /*读取led当前状态并写入kbuf中，接下来把kbuf中的内容写入到用户应用层buf中。*/
    memset(kbuf, 0, sizeof(kbuf));
    kbuf[0] = led_status(LED0);
    kbuf[1] = led_status(LED1);
    kbuf[2] = led_status(LED2);

    ret = copy_to_user(ubuf, kbuf, count);
    if(ret)
    {
        printk(KERN_ERR "led_read rest is %d\n", ret);

        return -1;
    }
    printk(KERN_DEBUG "led_read success copy\n");

    return 0;
}

/*
 * function: led设备文件写函数定义。用来控制led的亮灭。
 * file: 文件指针
 * ubuf: 用户应用层buf地址指针
 * count: 用户应用层buf大小
 * ppos: 位置指针
 * ret: 成功返回0，否则返回-1。
 **/
static ssize_t led_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    int ret;
    char kbuf[20] = {0};


    printk(KERN_DEBUG "led_write start\n");

    /*读取用户应用层buf中的内容，根据默认的定义操作相应的led亮灭。*/
    memset(kbuf, '\0', sizeof(kbuf));
    if(count > sizeof(kbuf))
    {
        count = sizeof(kbuf);
    }
    ret = copy_from_user(kbuf, ubuf, count);
    if(ret)
    {
        printk(KERN_ERR "led_write rest is %d\n", ret);

        return count - ret;
    }
    printk(KERN_DEBUG "led_write success content is %s\n", kbuf);
    
    kbuf[0] ? led_on(LED0) : led_off(LED0);
    kbuf[1] ? led_on(LED1) : led_off(LED1);
    kbuf[2] ? led_on(LED2) : led_off(LED2);

    return count;
}

/*
 * function: led驱动初始化函数，主要是注册led file_operations结构体，返回主设备号。
 * ret: 成功时返回0，否则返回出错码。
 **/
static int __init led_init(void)
{
    int ret;
    struct device *led_device;

    printk(KERN_DEBUG "led_init\n");

    ret = alloc_chrdev_region(&led_dev, LED_DEV_BASE, LED_DEV_NUM, LED_NAME);
    if(ret < 0)
    {
        printk(KERN_ERR "alloc_chrdev_region led fail\n");

        goto err;
    }
    led_cdev = cdev_alloc();
    if(!led_cdev)
    {
        printk(KERN_ERR "cdev_alloc fail\n");

        goto err_cdev_alloc;
    }
    led_cdev->owner = THIS_MODULE;
    led_cdev->ops = &led_fops;

    ret = cdev_add(led_cdev, led_dev, LED_DEV_NUM);
    if(ret)
    {
        printk(KERN_ERR "cdev_add led_cdev fail\n");
        
        goto err_cdev_add;
    }
    
    led_class = class_create(THIS_MODULE, "led_class");
    if(!led_class)
    {
        printk(KERN_ERR "class_create\n");
        
        ret = -ENOMEM;
        goto err_class_create;
    }
    led_device = device_create(led_class, NULL, led_dev, NULL, "s5pled");
    if(!led_device)
    {
        printk(KERN_ERR "device_create fail\n");
        
        goto err_device_create;
    }

    return 0;

err_device_create:
    class_destroy(led_class);
    
err_class_create:
    cdev_del(led_cdev);
    
err_cdev_add:
    kobject_put(&led_cdev->kobj);

err_cdev_alloc:
    unregister_chrdev_region(led_dev, LED_DEV_NUM);
    
err:
    return ret;
}

/*
 * function: led驱动卸载函数,用来解除注册的led字符设备文件。
 * ret: 无。
 * 
 **/
static void __exit led_exit(void)
{
    printk(KERN_DEBUG "led_exit\n");

    device_destroy(led_class, led_dev);
    class_destroy(led_class);
    cdev_del(led_cdev);
    unregister_chrdev_region(led_dev, LED_DEV_NUM);
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("lirongjin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("led driver");
MODULE_ALIAS("led");


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <linux/seq_file.h>

#define LLL_MAX_USER_SIZE 0x100

#define BCM2837_GPIO_ADDRESS 0x3F200000

static struct proc_dir_entry *dht22_proc = NULL;

static char data_buffer[LLL_MAX_USER_SIZE+1] = {0};

static unsigned int *gpio_registers = NULL;

#define GPIO_PIN_4  4
/* 2835 register offsets */
#define GPFSEL0      0
#define GPFSEL1      1
#define GPFSEL2      2
#define GPFSEL3      3
#define GPFSEL4      4
#define GPFSEL5      5
#define GPSET0       7
#define GPSET1       8
#define GPCLR0       10
#define GPCLR1       11
#define GPLEV0       13
#define GPLEV1       14
#define GPPUD        37
#define GPPUDCLK0    38
#define GPPUDCLK1    39

static int get_level(uint32_t *addr, uint32_t gpio)
{
    return *(addr + GPLEV0) >> gpio & 1;
}

static void gpio_pin_on(unsigned int pin)
{
	unsigned int fsel_index = pin/10;
	unsigned int fsel_bitpos = pin%10;
	unsigned int* gpio_fsel = gpio_registers + fsel_index;
	unsigned int* gpio_on_register = (unsigned int*)((char*)gpio_registers + 0x1c);

	*gpio_fsel &= ~(7 << (fsel_bitpos*3));
	*gpio_fsel |= (1 << (fsel_bitpos*3));
	*gpio_on_register |= (1 << pin);

	return;
}

static void gpio_pin_off(unsigned int pin)
{
	unsigned int *gpio_off_register = (unsigned int*)((char*)gpio_registers + 0x28);
	*gpio_off_register |= (1<<pin);
	return;
}

static int lll_read(struct seq_file *m, void *v)
{
    int level = get_level(gpio_registers, GPIO_PIN_4);
	seq_printf(m, "Hello, World!\nPin4: %d", level);
	return 0;
}

ssize_t lll_write(struct file *file, const char __user *user, size_t size, loff_t *off)
{
	unsigned int pin = UINT_MAX;
	unsigned int value = UINT_MAX;

	memset(data_buffer, 0x0, sizeof(data_buffer));

	if (size > LLL_MAX_USER_SIZE)
	{
		size = LLL_MAX_USER_SIZE;
	}

	if (copy_from_user(data_buffer, user, size))
		return 0;

	printk("Data buffer: %s\n", data_buffer);

	if (sscanf(data_buffer, "%d,%d", &pin, &value) != 2)
	{
		printk("Inproper data format submitted\n");
		return size;
	}

	if (pin > 21 || pin < 0)
	{
		printk("Invalid pin number submitted\n");
		return size;
	}

	if (value != 0 && value != 1)
	{
		printk("Invalid on/off value\n");
		return size;
	}

	printk("You said pin %d, value %d\n", pin, value);
	if (value == 1)
	{
		gpio_pin_on(pin);
	} else if (value == 0)
	{
		gpio_pin_off(pin);
	}

	return size;
}

static int lll_open(struct inode *inode, struct file *file)
{
	return single_open(file, lll_read, NULL);
}

static const struct proc_ops dht22_proc_fops = 
{
	.proc_open = lll_open,
	.proc_read = seq_read,
	.proc_write = lll_write,
	.proc_release = seq_release
};


static int __init dht22_driver_init(void)
{
	printk("DHT22 driver init\n");
	
	gpio_registers = (int*)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
	if (gpio_registers == NULL)
	{
		printk("Failed to map GPIO memory to driver\n");
		return -1;
	}
	
	printk("Successfully mapped in GPIO memory\n");
	
	// create an entry in the proc-fs
	dht22_proc = proc_create("dht22", 0, NULL, &dht22_proc_fops);
	if (dht22_proc == NULL)
	{
		return -1;
	}

	return 0;
}

static void __exit dht22_driver_exit(void)
{
	printk("DHT22 driver exit\n");
	iounmap(gpio_registers);
	proc_remove(dht22_proc);
	return;
}

module_init(dht22_driver_init);
module_exit(dht22_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ulvesson");
MODULE_DESCRIPTION("Simple DHT22 driver");
MODULE_VERSION("0.1");

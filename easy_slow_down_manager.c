#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/dmi.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung notebook fan controling module.");
MODULE_AUTHOR("George Kibardin");

/*
 * Special thank to Greg Kroah-Hartman from whom I've borrowed key pieces of code
 */

/*
 * There is 3 different modes here:
 *   0 - off
 *   1 - on
 *   2 - max performance mode
 * off is "normal" mode.
 * on means that whatever the bios setting for etiquette mode, is enabled.  It
 * seems that the BIOS can set either "auto" mode, or "slow" mode.  If "slow"
 * mode is set, the fan turns off, and the cpu is throttled down to not cause
 * the fan to turn on if at all possible.
 * max performance means that the processor can be overclocked and run faster
 * then is physically possible.  Ok, maybe not physically possible, but it is
 * overclocked.  Funny that the system has a setting for this...
 */
#define SABI_GET_ETIQUETTE_MODE                0x31
#define SABI_SET_ETIQUETTE_MODE                0x32

/* 0 is off, 1 is on, and 2 is a second user-defined key? */
#define SABI_GET_WIRELESS_BUTTON       0x12
#define SABI_SET_WIRELESS_BUTTON       0x13

/*
 * SABI HEADER in low memory (f0000)
 * We need to poke through memory to find a signature in order to find the
 * exact location of this structure.
 */
struct sabi_header {
    u16 port;
    u8 iface_func;
    u8 en_mem;
    u8 re_mem;
    u16 data_offset;
    u16 data_segment;
    u8 bios_ifver;
    u8 launcher_string;
} __attribute__((packed));

/*
 * The SABI interface that we use to write and read values from the system.
 * It is found by looking at the data_offset and data_segment values in the sabi
 * header structure
 */
struct sabi_interface {
    u16 mainfunc;
    u16 subfunc;
    u8 complete;
    u8 retval[20];
} __attribute__((packed));

/* Structure to get data back to the calling function */
struct sabi_retval {
    u8 retval[4];
};

static struct sabi_header __iomem *sabi;
static struct sabi_interface __iomem *sabi_iface;
static void __iomem *f0000_segment;
static struct mutex sabi_mutex;
static struct proc_dir_entry *proc_entry_slow_down;
static struct proc_dir_entry *proc_entry_wifi_kill;

static struct dmi_system_id __initdata samsung_dmi_table[] = {
    {
        .ident = "Samsung",
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "SAMSUNG ELECTRONICS CO., LTD."),
        },
        .callback = NULL,
    },
    { },
    };


static int debug;
module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Verbose output");


int sabi_exec_command(u8 command, u8 data, struct sabi_retval *sretval)
{
    int retval = 0;
    
    mutex_lock(&sabi_mutex);

    /* enable memory to be able to write to it */
    outb(readb(&sabi->en_mem), readw(&sabi->port));

    /* write out the command */
    writew(0x5843, &sabi_iface->mainfunc);
    writew(command, &sabi_iface->subfunc);
    writeb(0, &sabi_iface->complete);
    writeb(data, &sabi_iface->retval[0]);
    outb(readb(&sabi->iface_func), readw(&sabi->port));

    /* sleep for a bit to let the command complete */
    msleep(100);

    /* write protect memory to make it safe */
    outb(readb(&sabi->re_mem), readw(&sabi->port));

    /* see if the command actually succeeded */
    if (readb(&sabi_iface->complete) == 0xaa &&
        readb(&sabi_iface->retval[0]) != 0xff) {
        if (sretval) {
            sretval->retval[0] = readb(&sabi_iface->retval[0]);
            sretval->retval[1] = readb(&sabi_iface->retval[1]);
            sretval->retval[2] = readb(&sabi_iface->retval[2]);
            sretval->retval[3] = readb(&sabi_iface->retval[3]);
        }
    }
    else {
        /* Something bad happened, so report it and error out */
        printk(KERN_WARNING "SABI command 0x%02x failed with completion flag 0x%02x and output 0x%02x\n",
            command, readb(&sabi_iface->complete),
        readb(&sabi_iface->retval[0]));
        retval = -EINVAL;
    }
    mutex_unlock(&sabi_mutex);
    return retval;
}

int easy_slow_down_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data) {
    struct sabi_retval sretval;
    
    if (off > 0) {
        *eof = 1;
    }
    else if (!sabi_exec_command(SABI_GET_ETIQUETTE_MODE, 0, &sretval)) {
        page[0] = sretval.retval[0] + '0';
        return 1;
    }
    return 0;
}

int easy_slow_down_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data) {
    char mode = '0';
    if (copy_from_user(&mode, buffer, 1)) {
        return -EFAULT;
    }
    if (mode >= '0' && mode <= '2') {
        sabi_exec_command(SABI_SET_ETIQUETTE_MODE, mode - '0', NULL);
    }
    return count;
}

int easy_wifi_kill_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data) {
    struct sabi_retval sretval;
    
    if (off > 0) {
        *eof = 1;
    }
    else if (!sabi_exec_command(SABI_GET_WIRELESS_BUTTON, 0, &sretval)) {
        page[0] = sretval.retval[0] + '0';
        return 1;
    }
    return 0;
}

int easy_wifi_kill_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data) {
    char mode = '0';
    if (copy_from_user(&mode, buffer, 1)) {
        return -EFAULT;
    }
    if (mode >= '0' && mode <= '2') {
        sabi_exec_command(SABI_SET_WIRELESS_BUTTON, mode - '0', NULL);
    }
    return count;
}

int easy_slow_down_init(void) {
    
    const char *test_str = "SwSmi@";
    int pos;
    int index = 0;
    void __iomem *base;
    unsigned int ifaceP;

    mutex_init(&sabi_mutex);

    if (!dmi_check_system(samsung_dmi_table)) {
        printk(KERN_ERR "Easy slow down manager is intended to work only with Samsung laptops.\n");
        return -ENODEV;
    }

    f0000_segment = ioremap(0xf0000, 0xffff);
    if (!f0000_segment) {
        printk(KERN_ERR "Easy slow down manager: Can't map the segment at 0xf0000\n");
        return -EINVAL;
    }

    printk(KERN_INFO "Easy slow down manager: checking for SABI support.\n");

    /* Try to find the signature "SwSmi@" in memory to find the header */
    base = f0000_segment;
    for (pos = 0; pos < 0xffff; ++pos) {
        char temp = readb(base + pos);
        if (temp == test_str[index]) {
            if (5 == index++)
                break;
        }
        else {
            index = 0;
        }
    }
    if (pos == 0xffff) {
        printk(KERN_INFO "Easy slow down manager: SABI is not supported\n");
        iounmap(f0000_segment);
        return -EINVAL;
    }

    sabi = (struct sabi_header __iomem *)(base + pos + 1);

    printk(KERN_INFO "Easy slow down manager: SABI is supported (%x)\n", pos + 0xf0000 - 6);
    if (debug) {
        printk(KERN_DEBUG "SABI header:\n");
        printk(KERN_DEBUG " SMI Port Number = 0x%04x\n", readw(&sabi->port));
        printk(KERN_DEBUG " SMI Interface Function = 0x%02x\n", readb(&sabi->iface_func));
        printk(KERN_DEBUG " SMI enable memory buffer = 0x%02x\n", readb(&sabi->en_mem));
        printk(KERN_DEBUG " SMI restore memory buffer = 0x%02x\n", readb(&sabi->re_mem));
        printk(KERN_DEBUG " SABI data offset = 0x%04x\n", readw(&sabi->data_offset));
        printk(KERN_DEBUG " SABI data segment = 0x%04x\n", readw(&sabi->data_segment));
        printk(KERN_DEBUG " BIOS interface version = 0x%02x\n", readb(&sabi->bios_ifver));
        printk(KERN_DEBUG " KBD Launcher string = 0x%02x\n", readb(&sabi->launcher_string));
    }

    /* Get a pointer to the SABI Interface */
    ifaceP = (readw(&sabi->data_segment) & 0x0ffff) << 4;
    ifaceP += readw(&sabi->data_offset) & 0x0ffff;
    sabi_iface = (struct sabi_interface __iomem *)ioremap(ifaceP, 16);
    if (!sabi_iface) {
        printk(KERN_ERR "Easy slow down manager: Can't remap %x\n", ifaceP);
        iounmap(f0000_segment);
        return -EINVAL;
    }

    if (debug) {
        printk(KERN_DEBUG "Easy slow down manager: SABI Interface = %p\n", sabi_iface);
    }


    proc_entry_slow_down = create_proc_entry("easy_slow_down_manager", 0666, NULL);
    if (proc_entry_slow_down == NULL) {
        printk(KERN_INFO "Easy slow down manager: Couldn't create proc entry\n");
        iounmap(sabi_iface);
        iounmap(f0000_segment);
        return -ENOMEM;
    }
    else {
        proc_entry_slow_down->read_proc = easy_slow_down_read;
        proc_entry_slow_down->write_proc = easy_slow_down_write;
    }

    proc_entry_wifi_kill = create_proc_entry("easy_wifi_kill", 0666, NULL);
    if (proc_entry_wifi_kill == NULL) {
        printk(KERN_INFO "Easy slow down manager: Couldn't create proc entry\n");
        remove_proc_entry("easy_slow_down_manager", NULL);
        iounmap(sabi_iface);
        iounmap(f0000_segment);
        return -ENOMEM;
    }
    else {
        proc_entry_wifi_kill->read_proc = easy_wifi_kill_read;
        proc_entry_wifi_kill->write_proc = easy_wifi_kill_write;
    }
    return 0;
}

void easy_slow_down_exit(void) {
    remove_proc_entry("easy_slow_down_manager", NULL);
    remove_proc_entry("easy_wifi_kill", NULL);
    iounmap(sabi_iface);
    iounmap(f0000_segment);
    printk(KERN_INFO "Easy slow down manager exit.\n");
}

module_init(easy_slow_down_init);
module_exit(easy_slow_down_exit);


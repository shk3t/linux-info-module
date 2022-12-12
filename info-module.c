#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#define BUF_SIZE 4096

static int pid = 1;
static int struct_id = 1;
static struct proc_dir_entry* parent;
struct mutex lab_mutex;

static int __init init_lab_module(void);
static void __exit exit_lab_module(void);
static int open_proc(struct inode* inode, struct file* file);
static int close_proc(struct inode* inode, struct file* file);
static ssize_t read_proc(struct file* filp, char __user* buffer, size_t length, loff_t* offset);
static ssize_t write_proc(struct file* filp, const char* buff, size_t len, loff_t* off);

static struct proc_ops proc_fops = {.proc_open = open_proc,
                                    .proc_read = read_proc,
                                    .proc_write = write_proc,
                                    .proc_release = close_proc};

// https://elixir.bootlin.com/linux/v5.19/source/arch/x86/include/asm/processor.h
static size_t write_thread_struct(char __user* ubuf, struct task_struct* task) {
    char buf[BUF_SIZE];
    size_t len = 0;

    struct thread_struct thread = task->thread;
    len += sprintf(buf + len, "%16s %lx\n", "stack pointer:", thread.sp);
    len += sprintf(buf + len, "%16s %hx\n", "es:", thread.es);
    len += sprintf(buf + len, "%16s %hx\n", "ds:", thread.ds);
    len += sprintf(buf + len, "%16s %hx\n", "fsindex:", thread.fsindex);
    len += sprintf(buf + len, "%16s %hx\n", "gsindex:", thread.gsindex);
    len += sprintf(buf + len, "%16s %lx\n", "fsbase:", thread.fsbase);
    len += sprintf(buf + len, "%16s %lx\n", "gsbase:", thread.gsbase);

    if (copy_to_user(ubuf, buf, len)) {
        return -EFAULT;
    }
    return len;
}

static size_t write_vm_area_struct(char __user* ubuf, struct task_struct* task) {
    char buf[BUF_SIZE];
    size_t len = 0;

    struct vm_area_struct* vm_area = find_vma(task->mm, (unsigned long)(NULL));
    len += sprintf(buf + len, "%16s %16s %16s %16s %16s\n", "start", "end", "permissions", "flags",
                   "offset");

    for (size_t i = 0; vm_area && i < 30; i++) {
        unsigned long pgprot = vm_area->vm_page_prot.pgprot;
        len += sprintf(buf + len, "%16lx %16lx %16lo %16lu %16lu\n", vm_area->vm_start,
                       vm_area->vm_end, pgprot >= (unsigned long)(32768) ? 0 : pgprot,
                       vm_area->vm_flags, vm_area->vm_pgoff);
        vm_area = vm_area->vm_next;
    }
    len += sprintf(buf + len, "...\n");

    if (copy_to_user(ubuf, buf, len)) {
        return -EFAULT;
    }

    return len;
}

static int open_proc(struct inode* inode, struct file* file) {
    mutex_lock(&lab_mutex);
    pr_info("proc file opened\t");
    return 0;
}

static int close_proc(struct inode* inode, struct file* file) {
    mutex_unlock(&lab_mutex);
    pr_info("proc file closed\n");
    return 0;
}

static ssize_t read_proc(struct file* file, char __user* ubuf, size_t count, loff_t* poffset) {
    char buf[BUF_SIZE];
    int len = 0;

    if (*poffset > 0 || count < BUF_SIZE) {
        return 0;
    }

    struct task_struct* task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);

    if (task == NULL) {
        len += sprintf(buf, "task_struct for pid %d is NULL\n", pid);
        if (copy_to_user(ubuf, buf, len)) {
            return -EFAULT;
        }
        *poffset = len;
        return len;
    }

    switch (struct_id) {
        default:
        case 0:
            len = write_thread_struct(ubuf, task);
            break;
        case 1:
            len = write_vm_area_struct(ubuf, task);
            break;
    }

    *poffset = len;
    printk(KERN_INFO "proc file read\n");
    return len;
}

static ssize_t write_proc(struct file* filp,
                          const char __user* ubuf,
                          size_t count,
                          loff_t* poffset) {
    int read_digits_count, c, a, b;
    char buf[BUF_SIZE];

    if (*poffset > 0 || count > BUF_SIZE) {
        return -EFAULT;
    }

    if (copy_from_user(buf, ubuf, count)) {
        return -EFAULT;
    }

    read_digits_count = sscanf(buf, "%d %d", &a, &b);

    if (read_digits_count != 2) {
        return -EFAULT;
    }

    struct_id = a;
    pid = b;
    c = strlen(buf);
    *poffset = c;
    printk(KERN_INFO "proc file wrote\n");
    return c;
}

static int __init lab_driver_init(void) {
    parent = proc_mkdir("lab", NULL);
    if (parent == NULL) {
        pr_info("error creating proc entry");
        return -1;
    }

    mutex_init(&lab_mutex);

    proc_create("struct_info", 0666, parent, &proc_fops);
    pr_info("lab-module inserted\n");
    return 0;
}

static void __exit lab_driver_exit(void) {
    mutex_destroy(&lab_mutex);
    proc_remove(parent);

    pr_info("lab-module removed\n");
}

module_init(lab_driver_init);
module_exit(lab_driver_exit);

MODULE_LICENSE("GPL");

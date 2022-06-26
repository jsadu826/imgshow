#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/uaccess.h>

#define MODULE_NAME "mod_imgshow"
#define REGION_NAME "region_imgshow"
#define CLASS_NAME "imgshow_cls_imgshow"
#define DEVICE_NAME "imgshow"

MODULE_LICENSE("GPL");

// use STATIC

static int __init imgshow_init(void);
static void __exit imgshow_exit(void);
module_init(imgshow_init);
module_exit(imgshow_exit);

static int imgshow_open(struct inode *inode, struct file *filp);
static int imgshow_release(struct inode *inode, struct file *filp);
static ssize_t imgshow_read(struct file *filp, char __user *buffer, size_t count, loff_t *offp);
static ssize_t imgshow_write(struct file *filp, const char __user *buffer, size_t count, loff_t *offp);

static struct file_operations imgshow_fops = {
    .read = imgshow_read,
    .write = imgshow_write,
    .open = imgshow_open,
    .release = imgshow_release,
};

static void copy_partial(unsigned short out_w, char __user *buffer);

static int major;
static struct cdev cdev;
static struct class *cls;

static char *img_dat = NULL;
static int img_w = 0, img_h = 0;
static int is_read_end = 0;
static int ret;

static int __init imgshow_init(void) {
  dev_t dev;
  // alloc_chrdev_region(..., first minor number, minor range length, device name)
  alloc_chrdev_region(&dev, 0, 1, REGION_NAME); // check "cat /proc/devices"
  major = MAJOR(dev);

  cdev_init(&cdev, &imgshow_fops);
  // cdev_add(..., ..., region length (normally 1))
  cdev_add(&cdev, MKDEV(major, 0), 1); // now cdev is visible to the kernel
  cdev.owner = THIS_MODULE;

  cls = class_create(THIS_MODULE, CLASS_NAME); // check "/sys/class/[class name]"

  /* create node under /dev throught a virtual file system sysfs
  check "sys/devices/virtual/[class name]/[user space device name]"
  or "/dev/[user space device name]" */
  device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); // now cdev is visible to the usr

  printk(KERN_INFO "%s loaded\n", MODULE_NAME);
  return 0;
}

static void __exit imgshow_exit(void) {
  if (img_dat != NULL) {
    kfree(img_dat);
  }
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);
  cdev_del(&cdev);
  unregister_chrdev_region(MKDEV(major, 0), 1);
  printk(KERN_INFO "%s unloaded\n", MODULE_NAME);
}

static int imgshow_open(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "%s opened\n", DEVICE_NAME);
  return 0;
}

static int imgshow_release(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "%s closed\n", DEVICE_NAME);
  return 0;
}

static ssize_t imgshow_write(struct file *filp, const char __user *buffer, size_t count, loff_t *offp) {
  size_t offs1, offs2;
  char *in_w, *in_h;
  char *raw_in = (char *)kmalloc(count, GFP_KERNEL);
  ret = copy_from_user(raw_in, buffer, count);

  // get image width
  for (offs1 = 0;; ++offs1) {
    if (raw_in[offs1] == ' ') {
      break;
    }
  }
  in_w = (char *)kmalloc(offs1 + 1, GFP_KERNEL);
  strncpy(in_w, raw_in, offs1);
  ret = kstrtoint(in_w, 10, &img_w);

  // get image height
  for (offs2 = offs1 + 1;; ++offs2) {
    if (raw_in[offs2] == ' ') {
      break;
    }
  }
  in_h = (char *)kmalloc(offs2 - offs1, GFP_KERNEL);
  strncpy(in_h, raw_in + offs1 + 1, offs2 - offs1 - 1);
  ret = kstrtoint(in_h, 10, &img_h);

  // get image data
  if (img_dat != NULL) {
    kfree(img_dat);
  }
  img_dat = (char *)kmalloc(count - offs2, GFP_KERNEL);
  strcpy(img_dat, raw_in + offs2 + 1);

  kfree(raw_in);
  kfree(in_w);
  kfree(in_h);
  printk(KERN_INFO "%s: width = %d, height = %d\n", DEVICE_NAME, img_w, img_h);
  return count;
}

static ssize_t imgshow_read(struct file *filp, char __user *buffer, size_t count, loff_t *offp) {
  struct fd tmnl_fd;
  struct tty_struct *tmnl_tty;
  unsigned short tmnl_w;

  if (img_dat == NULL) {
    return -ENODATA;
  } else if (is_read_end) {
    is_read_end = 0;
    return 0;
  }

  tmnl_fd = fdget(0);
  tmnl_tty = ((struct tty_file_private *)tmnl_fd.file->private_data)->tty;
  tmnl_w = tmnl_tty->winsize.ws_col;

  if (tmnl_w >= img_w) {
    ret = copy_to_user(buffer, img_dat, strlen(img_dat));
  } else {
    copy_partial(tmnl_w, buffer);
  }
  is_read_end = 1;
  return count;
}

static void copy_partial(unsigned short out_w, char __user *buffer) {
  int h = 0;
  size_t dat_offs = 0;
  size_t buf_offs = 0;
  for (; h < img_h - 1; ++h) {
    ret = copy_to_user(buffer + buf_offs, img_dat + dat_offs, out_w);
    dat_offs += img_w + 1;
    buf_offs += out_w;
  }
}

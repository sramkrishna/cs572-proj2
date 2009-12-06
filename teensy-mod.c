#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/uaccess.h>



MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Sri Kahiro Nelson");

/*
 * Teensy vendor_id and product_id for USB.  Used lsusb to get the numbers.
 */


#define VENDOR_ID		0x16c0
#define PRODUCT_ID		0x0480
/* we hard code our teensi_endpoint address that we programmed from the firmware */
#define TEENSI_ENDPOINT_ADDR    0x83

/* set up our usb device information structure 
 * Some of this code is taken from the source of usb-skeleton.c in the 
 * kernel source.  Some stuff might be superfluous
 */

struct usb_teensi_dev {
	struct usb_device 		*udev;
	struct usb_interface 		*interface;
	unsigned char			blue;
	unsigned char			red;
	unsigned char			green;
};

#define to_teensi_dev(d) container_of(d, struct usb_teensi_dev, kref)


/*
 * Code Acknowledgement
 *
 * we got this bit of code from a linux journal magazine
 * http://www.linuxjournal.com/article/7353
 *
 */

static struct usb_device_id id_table [] = {
      { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
      {  }
};


MODULE_DEVICE_TABLE (usb, id_table);

static void change_color (struct usb_teensi_dev *dev)
{
	int retval = 0;
	unsigned int color = 0x07;
	unsigned char *buffer = NULL;
	int *sent = 0;

	buffer = kmalloc(8,GFP_KERNEL);
	if (!buffer) {
		dev_err (&dev->udev->dev, "out of memory\n");
		return;
	}

	if (dev->blue)
		color = 0x1;
	if (dev->red)
		color = 0x0;
	if (dev->green)
		color = 0x2;

	retval = usb_bulk_msg (dev->udev, 
				usb_sndbulkpipe(dev->udev,TEENSI_ENDPOINT_ADDR),
				(void *)color, 8, sent, 0);
	if (retval)
		err ("failed! retval = %d\n", retval);
	kfree(buffer);
}



#define show_set(value) \
static ssize_t show_##value(struct device *dev, struct device_attribute *attr, char *buf)              \
{                                                                       \
        struct usb_interface *intf = to_usb_interface(dev);             \
        struct usb_teensi_dev *led = usb_get_intfdata(intf);                   \
                                                                        \
        return sprintf(buf, "%d\n", led->value);                        \
}                                                                       \
static ssize_t set_##value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)    \
{                                                                       \
        struct usb_interface *intf = to_usb_interface(dev);             \
        struct usb_teensi_dev *led = usb_get_intfdata(intf);                   \
        int temp = simple_strtoul(buf, NULL, 10);                       \
                                                                        \
        led->value = temp;                                              \
        change_color(led);                                              \
        return count;                                                   \
}                                                                       \
static DEVICE_ATTR(value, S_IWUGO | S_IRUGO, show_##value, set_##value);
show_set(blue);
show_set(red);
show_set(green);



static int teensi_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_teensi_dev *dev;
	int retval = -ENOMEM;

	printk (KERN_INFO "We are probing now...");

	/* allocate memory for our device state and initialize it */
	dev = kzalloc (sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err("Out of memory");
		goto error_mem;
	}
	dev->udev = usb_get_dev (interface_to_usbdev(interface));
	dev->interface = interface;
	usb_set_intfdata (interface,dev);

	/* create some /sys files for us to write to */

	retval = device_create_file (&interface->dev, &dev_attr_blue);
	if (retval)
		goto error;
	retval = device_create_file (&interface->dev, &dev_attr_red);
	if (retval)
		goto error;
	retval = device_create_file (&interface->dev, &dev_attr_green);
	if (retval)
		goto error;
	

        /* let the user know what node this device is now attached to */
        dev_info(&interface->dev, "USB Skeleton device now attached to USBSkel-%d", interface->minor);
        return 0;

error:
	device_remove_file(&interface->dev, &dev_attr_blue);
        device_remove_file(&interface->dev, &dev_attr_red);
        device_remove_file(&interface->dev, &dev_attr_green);
        usb_set_intfdata (interface, NULL);
        usb_put_dev(dev->udev);
        kfree(dev);
error_mem:
        return retval;
}

static void teensi_disconnect(struct usb_interface *interface)
{
        struct usb_teensi_dev *dev;

        dev = usb_get_intfdata(interface);
        device_remove_file(&interface->dev, &dev_attr_blue);
        device_remove_file(&interface->dev, &dev_attr_red);
        device_remove_file(&interface->dev, &dev_attr_green);

        usb_set_intfdata(interface, NULL);
	usb_put_dev(dev->udev);

	kfree (dev);
	info ("teensi device removed.");
}

static struct usb_driver led_driver = {
    .name = "teensi",
    .probe =  teensi_probe,
    .disconnect = teensi_disconnect,
    .id_table = id_table,
};

static int teensi_init (void)
{
    int retval = 0;

    retval = usb_register (&led_driver);
    if (retval)
        err ("usb_register failed. Error number %d", retval);
    return retval;
}

static void teensi_exit (void)
{
    usb_deregister (&led_driver);
}

module_init (teensi_init);
module_exit (teensi_exit);

#include <linux/config.h>
#ifdef CONFIG_USB_DEBUG
        #define DEBUG 1
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>



MODULE_LICENSE ("DUAL BSD/GPL");
MODULE_AUTHOR ("Sri Kahiro Nelson")

/*
 * Teensy vendor_id and product_id for USB
 */


#define VENDOR_ID		0x16C0
#define PRODUCT_ID		0x047C


/*
 * Code Acknowledgement
 *
 * we got this bit of code from a linux journal magazine
 * http://www.linuxjournal.com/article/7353
 *
 */

static struct usb_device_id id_table [] = {
      { USB_DEVICE (VENDOR_ID, PRODUCT_ID) },
      {  },
};

MODULE_DEVICE_TABLE = (usb, id_table);

static struct usb_driver led_driver = {
    .owner = THIS_MODULE,
    .name = "teensi",
    .probe = led_probe,
    .disconnect = led_disconnect,
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

static int teensi_exit (void)
{
    usb_deregister (&led_driver);
}

module_init (teensi_init);
module_exit (teensi_exit);

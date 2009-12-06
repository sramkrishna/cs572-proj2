#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kref.h>
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
#define PRODUCT_ID		0x0478

/* set up our usb device information structure 
 * Some of this code is taken from the source of usb-skeleton.c in the 
 * kernel source.  Some stuff might be superfluous
 */

struct usb_teensi_dev {
	struct usb_device 		*udev;
	struct usb_interface 		*interface;
	struct semaphore 		limit_sem;
	struct usb_anchor 		submitted;
	unsigned char  			*cmd_out_buffer; /* buffer to send cmd to change color */
	unsigned char  			*cmd_in_buffer; /* buffer to send cmd to change color */
	size_t 				cmd_out_size;
	size_t 				cmd_in_size;
	__u8				cmd_out_endpointAddr; /* address of command out endpoint */
	__u8				cmd_in_endpointAddr; /* address of command out endpoint */
	int				errors; 	/* last request tanked */
	int				open_count;	/* count number of openers */
	spinlock_t			err_lock;
	struct kref			kref;
	struct mutex			io_mutex;	/* synchronize I/O for disconnect */
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

#define USB_TEENSI_MINOR_BASE     666
#define WRITES_IN_FLIGHT 5


MODULE_DEVICE_TABLE (usb, id_table);

static const struct file_operations teensi_fops = {
	.owner =	THIS_MODULE,
/*	.write =	teensi_write,
	.open =		teensi_open,
	.release =	teensi_release,
	.flush =	teensi_FLUSH,*/
};

static struct usb_class_driver teensi_class = {
	.name =		"teensi%d",
	.fops =		&teensi_fops,
	.minor_base =	USB_TEENSI_MINOR_BASE,
};

static void teensi_delete(struct kref *kref)
{
        struct usb_teensi_dev *dev = to_teensi_dev(kref);

        usb_put_dev(dev->udev);
        kfree(dev->cmd_in_buffer);
        kfree(dev);
}

static int teensi_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_teensi_dev *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;

	printk (KERN_INFO "We are probing now...");

//	/* allocate memory for our device state and initialize it */
//	dev = kzalloc (sizeof(*dev), GFP_KERNEL);
//	if (!dev) {
//		err("Out of memory");
//		goto error;
//	}
//	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
//	mutex_init (&dev->io_mutex);
//	spin_lock_init (&dev->err_lock);
//	init_usb_anchor (&dev->submitted);
//	dev->udev = usb_get_dev (interface_to_usbdev(interface));
//	dev->interface = interface;
//
//	/* set up the endpoint information */
//	/* use only the first bulk-in and bulk-out endpoints */
//	iface_desc = interface->cur_altsetting;
//
//	printk(KERN_INFO "we found our device, we are going to use it!\n");
//
//	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
 //               endpoint = &iface_desc->endpoint[i].desc;
//
 //               if (!dev->cmd_in_endpointAddr &&
  //                  usb_endpoint_is_bulk_in(endpoint)) {
   //                     /* we found a bulk in endpoint */
    //                    buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
     //                   dev->cmd_in_size = buffer_size;
      //                  dev->cmd_in_endpointAddr = endpoint->bEndpointAddress;
       //                 dev->cmd_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
        //                if (!dev->cmd_in_buffer) {
         //                       err("Could not allocate bulk_in_buffer");
          //                      goto error;
           //             }
            //    }
//
 //               if (!dev->cmd_out_endpointAddr &&
  //                  usb_endpoint_is_bulk_out(endpoint)) {
   //                     /* we found a bulk out endpoint */
//			printk (KERN_INFO "We found a bulk out endpoint!\n");
 //                       dev->cmd_out_endpointAddr = endpoint->bEndpointAddress;
  //              }
   //     }
//
//	if (!(dev->cmd_in_endpointAddr && dev->cmd_out_endpointAddr)) {
 //               err("Could not find both bulk-in and bulk-out endpoints");
  //              goto error;
   //     }
//
 //       /* save our data pointer in this interface device */
  //      usb_set_intfdata(interface, dev);
//
 //       /* we can register the device now, as it is ready */
  //      retval = usb_register_dev(interface, &teensi_class);
   //     if (retval) {
    //            /* something prevented us from registering this driver */
     //           err("Not able to get a minor for this device.");
      //          usb_set_intfdata(interface, NULL);
       //         goto error;
        //}
//
 //       /* let the user know what node this device is now attached to */
  //      info("USB Skeleton device now attached to USBSkel-%d", interface->minor);
        return 0;

error:
        if (dev)
                /* this frees allocated memory */
                kref_put(&dev->kref, teensi_delete);

        return retval;
}

static void teensi_disconnect(struct usb_interface *interface)
{
        struct usb_teensi_dev *dev;
        int minor = interface->minor;

//        dev = usb_get_intfdata(interface);
 //       usb_set_intfdata(interface, NULL);

        /* give back our minor */
  //      usb_deregister_dev(interface, &teensi_class);

        /* prevent more I/O from starting */
//        mutex_lock(&dev->io_mutex);
//        dev->interface = NULL;
//        mutex_unlock(&dev->io_mutex);

//        usb_kill_anchored_urbs(&dev->submitted);

        /* decrement our usage count */
//        kref_put(&dev->kref, teensi_delete);

        info("USB Skeleton #%d now disconnected", minor);
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

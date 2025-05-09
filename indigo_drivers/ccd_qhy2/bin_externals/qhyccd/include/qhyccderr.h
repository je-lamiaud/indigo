
#ifndef __QHYCCDERR_H__
#define __QHYCCDERR_H__

// This is used by old CCD or A series cameras, if you exp 5 sec, you need to read just after you begin exp, not 5 sec after start exp
#define QHYCCD_READ_DIRECTLY            0x2001

#define QHYCCD_DELAY_200MS              0x2000

/**
 * It means the camera using PCIE transfer data */
#define QHYCCD_PCIE						 9


/**
 * It means the camera using WINPCAP transfer data */
#define QHYCCD_WINPCAP                   8


/**
 * It means the camera using GiGaE transfer data */
#define QHYCCD_QGIGAE                   7

/**
 * It means the camera using usb sync transfer data */
#define QHYCCD_USBSYNC                  6

/**
 * It means the camera using usb async transfer data */
#define QHYCCD_USBASYNC                 5

/**
 * It means the camera is color one */
#define QHYCCD_COLOR                    4

/**
 * It means the camera is mono one*/
#define QHYCCD_MONO                     3

/**
 * It means the camera has cool function */
#define QHYCCD_COOL                     2

/**
 * It means the camera do not have cool function */
#define QHYCCD_NOTCOOL                  1

/**
 * camera works well */
#define QHYCCD_SUCCESS                  0

/**
 * Other error */
#define QHYCCD_ERROR                    0xFFFFFFFF

#define QHYCCD_ERROR_IMAGESHIFT          -2

#define QHYCCD_ERROR_DEVICELOSE          -3

#if 0
/**
 * There is no camera connected */
#define QHYCCD_ERROR_NO_DEVICE         -2

/**
 * Do not support the function */
#define QHYCCD_ERROR        -3

/**
 * Set camera params error */
#define QHYCCD_ERROR_SETPARAMS         -4

/**
 * Get camera params error */
#define QHYCCD_ERROR_GETPARAMS         -5

/**
 * The camera is exposing now */
#define QHYCCD_ERROR_EXPOSING          -6

/**
 * The camera expose failed */
#define QHYCCD_ERROR_EXPFAILED         -7

/**
 * There is another instance is getting data from camera */
#define QHYCCD_ERROR_GETTINGDATA       -8

/**
 * Get data from camera failed */
#define QHYCCD_ERROR_GETTINGFAILED     -9

/**
 * Init camera failed */
#define QHYCCD_ERROR_INITCAMERA        -10

/**
 * Release SDK resouce failed */
#define QHYCCD_ERROR_RELEASERESOURCE   -11

/**
 * Init SDK resouce failed */
#define QHYCCD_ERROR_INITRESOURCE      -12

/**
 * There is no match camera */
#define QHYCCD_ERROR             -13

/**
 * Open cam failed */
#define QHYCCD_ERROR_OPENCAM           -14

/**
 * Init cam class failed */
#define QHYCCD_ERROR_INITCLASS         -15

/**
 * Set Resolution failed */
#define QHYCCD_ERROR        -16

/**
 * Set usbtraffic failed */
#define QHYCCD_ERROR        -17

/**
 * Set usb speed failed */
#define QHYCCD_ERROR          -18

/**
 * Set expose time failed */
#define QHYCCD_ERROR_SETEXPOSE         -19

/**
 * Set cam gain failed */
#define QHYCCD_ERROR_SETGAIN           -20

/**
 * Set cam white balance red failed */
#define QHYCCD_ERROR_SETRED            -21

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_SETBLUE           -22

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_EVTCMOS           -23

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR_EVTUSB            -24

/**
 * Set cam white balance blue failed */
#define QHYCCD_ERROR           -25
#endif

#endif

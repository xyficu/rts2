/*! @file 
 * Status constants.
 * 
 * @author petr
 */

#ifndef __RTS__STATUS__
#define __RTS__STATUS__

// device priority
#define DEVICE_NOPRIORITY	0
#define DEVICE_PRIORITY		1

#define DEVICE_MASK_PRIORITY	1

// Camera status
#define CAM_MASK_EXPOSE		0x01

#define CAM_NOEXPOSURE		0x00
#define CAM_EXPOSING		0x01

#define CAM_MASK_READING	0x02

#define CAM_NOTREADING		0x00
#define CAM_READING		0x02

#define CAM_MASK_DATA		0x04

#define CAM_NODATA		0x00
#define CAM_DATA		0x04

#define CAM_MASK_FOCUSINGS	0x08

#define CAM_NOFOCUSING		0x00
#define CAM_FOCUSING		0x08

#define CAM_MASK_SHUTTER	0x30

#define CAM_SHUT_CLEARED	0x00
#define CAM_SHUT_SET		0x10
#define CAM_SHUT_TRANS		0x20

#define CAM_MASK_COOLING	0xC0

#define CAM_COOL_OFF		0x00
#define CAM_COOL_FAN		0x40
#define CAM_COOL_PWR		0x80
#define CAM_COOL_TEMP		0xC0

// Photomer status
#define PHOT_MASK_INTEGRATE	0x01
#define PHOT_NOINTEGRATE	0x00
#define PHOT_INTEGRATE		0x01

// telescope status
#define TEL_MASK_MOVING		0x07

#define TEL_OBSERVING		0x00
#define TEL_MOVING		0x01
#define TEL_PARKED		0x02
#define TEL_PARKING		0x04

#define TEL_MASK_TRACK		0x08

#define TEL_NOTRACK		0x00
#define TEL_TRACKING		0x08

// dome status

#define DOME_DOME_MASK		0x0f

#define DOME_UNKNOW		0x00
#define DOME_CLOSED		0x01
#define DOME_OPENING		0x02
#define DOME_OPENED		0x04
#define DOME_CLOSING		0x08

#define DOME_WEATHER_MASK	0x30
#define DOME_WEATHER_OK		0x10
#define DOME_WEATHER_BAD	0x20
#define DOME_WEATHER_UNKNOW	0x30

#define MIRROR_MASK		0x0f
#define MIRROR_UNKNOW		0x00
#define MIRROR_A		0x01
#define MIRROR_A_B		0x02
#define MIRROR_B		0x03
#define MIRROR_B_A		0x04

#define SERVERD_DAY		0
#define SERVERD_EVENING		1
#define SERVERD_DUSK		2
#define SERVERD_NIGHT		3
#define SERVERD_DAWN		4
#define SERVERD_MORNING		5

#define SERVERD_OFF		11

#define SERVERD_STATUS_MASK	0x0f
#define SERVERD_STANDBY_MASK	0x30

#define SERVERD_STANDBY		0x10

// "executor" and related states..
#define EXEC_STATE_MASK		0x0f
#define EXEC_IDLE		0x00
#define EXEC_MOVE		0x01
#define EXEC_ACQUIRE		0x02
#define EXEC_OBSERVE		0x03
#define EXEC_LASTREAD		0x04

#define IMGPROC_IDLE		0x00
#define IMGPROC_RUN		0x00

// to send data

#define DEVDEM_DATA		0x80

// Errors

#define DEVDEM_E_COMMAND	-1	// invalid command
#define DEVDEM_E_PARAMSNUM	-2	// invalid parameter number
#define DEVDEM_E_PARAMSVAL	-3	// invalid parameter(s) value
#define DEVDEM_E_HW		-4	// some HW failure
#define DEVDEM_E_SYSTEM		-5	// some system error
#define DEVDEM_E_PRIORITY	-6	// error by changing priority
#define DEVDEM_E_TIMEOUT	-7	// connection has time-outed

// Client errors goes together, intersection between devdem and plancomm clients
// must be empty.

#define PLANCOMM_E_NAMESPACE	-101	//! invalid namespace
#define PLANCOMM_E_HOSTACCES	-102	//! no route to host and various other problems. See errno for futhre details.
#define PLANCOMM_E_COMMAND	-103	//! invalid command
#define PLANCOMM_E_PARAMSNUM	-104	//! invalid number of parameters to system command

// maximal number of devices
#define MAX_DEVICE		10

// maximal sizes of some important strings
#define DEVICE_NAME_SIZE	50
#define CLIENT_LOGIN_SIZE	50
#define CLIENT_PASSWD_SIZE	50
#define DEVICE_URI_SIZE		80


// device types
#define DEVICE_TYPE_UNKNOW	0
#define DEVICE_TYPE_SERVERD	1
#define DEVICE_TYPE_MOUNT	2
#define DEVICE_TYPE_CCD		3
#define DEVICE_TYPE_DOME	4
#define DEVICE_TYPE_WEATHER	5
#define DEVICE_TYPE_ARCH	6
#define DEVICE_TYPE_PHOT	7
#define DEVIDE_TYPE_ARCH	6
#define DEVICE_TYPE_PHOT	7
#define DEVICE_TYPE_PLAN	8
#define DEVICE_TYPE_GRB		9
#define DEVICE_TYPE_FOCUS	10
#define DEVICE_TYPE_MIRROR	11

#define DEVICE_TYPE_EXECUTOR    20
#define DEVICE_TYPE_IMGPROC	21

// and more to come..
// #define DEVICE_TYPE_

// default serverd port
#define SERVERD_PORT		5557

// name of the default server status
#define SERVER_STATUS		"server_st"

#define MAX_STATUS_TXT		60

#endif /* __RTS__STATUS__ */

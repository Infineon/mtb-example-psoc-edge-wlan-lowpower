/******************************************************************************
* File Name: lowpower_task.h
*
* Description: This file includes the macros and enumerations used by the
* example to connect to an AP, configure power-save mode of the WLAN device, and
* configure the parameters for suspending the network stack.
*
* Related Document: See README.md
*
*******************************************************************************
 * (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Include guard
*******************************************************************************/
#ifndef LOWPOWER_TASK_H_
#define LOWPOWER_TASK_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
* Includes
*******************************************************************************/
#include "cybsp.h"

/* FreeRTOS header file */
#include <FreeRTOS.h>
#include <task.h>

/*******************************************************************************
* Defines
*******************************************************************************/
/* Wi-Fi Credentials: Modify WIFI_SSID and WIFI_PASSWORD to match your Wi-Fi
 * network Credentials.
 */
#define WIFI_SSID                         "MY_WIFI_SSID"
#define WIFI_PASSWORD                     "MY_WIFI_PASSWORD"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY                     (CY_WCM_SECURITY_WPA2_AES_PSK)

#define LED_BLINK_DELAY_MS                (100U)

/* This macro specifies the interval in milliseconds that the device monitors
 * the network for inactivity. If the network is inactive for duration lesser 
 * than INACTIVE_WINDOW_MS in this interval, the MCU does not suspend the network 
 * stack and informs the calling function that the MCU wait period timed out 
 * while waiting for network to become inactive.
 */
#define INACTIVE_INTERVAL_MS              (300U)

/* This macro specifies the continuous duration in milliseconds for which the 
 * network has to be inactive. If the network is inactive for this duaration,
 * the MCU will suspend the network stack. Now, the MCU will not need to service
 * the network timers which allows it to stay longer in sleep/deepsleep.
 */
#define INACTIVE_WINDOW_MS                (200U)

/* Delay between successive Wi-Fi connection attempts, in milliseconds. */
#define WIFI_CONN_RETRY_INTERVAL_MSEC     (100U)

/* Debug prints */
#define APP_INFO( x )           do { printf("Info: "); printf x;} while(0);
#define ERR_INFO( x )           do { printf("Error: "); printf x;} while(0);

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
extern TaskHandle_t lowpower_task_handle;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
void lowpower_task(void *arg);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* LOWPOWER_TASK_H_ */


/* [] END OF FILE */
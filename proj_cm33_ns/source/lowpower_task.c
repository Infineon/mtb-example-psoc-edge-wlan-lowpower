/*******************************************************************************
* File Name:   lowpower_task.c
*
* Description: This file contains the task definition for initializing the
* Wi-Fi device, configuring the selected WLAN power save mode, connecting to
* the AP, and suspending the network stack to enable higher power savings on
* the PSOC Edge E84 MCU.
*
* Related Document: See Readme.md
*
********************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/* Header file includes */
#include "lowpower_task.h"

/* Wi-Fi Connection Manager (WCM) header file. */
#include "cy_wcm.h"

/* lwIP header files */
#include "cy_network_mw_core.h"
#include "lwip/netif.h"

/* Low Power Assistant header files. */
#include "network_activity_handler.h"

/* Wi-Fi Host Driver (WHD) header files. */
#include "whd_wifi_api.h"

/* Retarget_io header file */
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define MAX_WIFI_RETRY_COUNT                         (3U)
#define RESET_VAL                                    (0U)
#define APP_SDIO_INTERRUPT_PRIORITY                  (7U)
#define APP_HOST_WAKE_INTERRUPT_PRIORITY             (2U)
#define APP_SDIO_FREQUENCY_HZ                        (25000000U)
#define SDHC_SDIO_64BYTES_BLOCK                      (64U)
#define INTERFACE_ID                                 (0U)

/*******************************************************************************
* Global Variables
*******************************************************************************/

/* Low-power task handle */
TaskHandle_t lowpower_task_handle;
static mtb_hal_sdio_t sdio_instance;
static cy_stc_sd_host_context_t sdhc_host_context;
static cy_wcm_config_t wcm_config;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

/* SysPm callback parameter structure for SDHC */
static cy_stc_syspm_callback_params_t sdcardDSParams =
{
        .context   = &sdhc_host_context,
        .base      = CYBSP_WIFI_SDIO_HW
};

/* SysPm callback structure for SDHC*/
static cy_stc_syspm_callback_t sdhcDeepSleepCallbackHandler =
{
    .callback           = Cy_SD_Host_DeepSleepCallback,
    .skipMode           = SYSPM_SKIP_MODE,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &sdcardDSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};

#endif

/*******************************************************************************
* Function definitions
*******************************************************************************/
/*******************************************************************************
* Function Name: sdio_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for SDIO instance.
*******************************************************************************/
static void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

/*******************************************************************************
* Function Name: host_wake_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for the host wake up input pin.
*******************************************************************************/
static void host_wake_interrupt_handler(void)
{
    mtb_hal_gpio_process_interrupt(&wcm_config.wifi_host_wake_pin);
}

/*******************************************************************************
* Function Name: app_sdio_init
********************************************************************************
* Summary:
* This function configures and initializes the SDIO instance used in 
* communication between the host MCU and the wireless device.
*******************************************************************************/
static void app_sdio_init(void)
{
    cy_rslt_t result;
    mtb_hal_sdio_cfg_t sdio_hal_cfg;

    cy_stc_sysint_t sdio_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY
    };

    cy_stc_sysint_t host_wake_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_HOST_WAKE_IRQ,
        .intrPriority = APP_HOST_WAKE_INTERRUPT_PRIORITY
    };

    /* Initialize the SDIO interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = Cy_SysInt_Init(&sdio_intr_cfg,
            sdio_interrupt_handler);

    /* SDIO interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    /* Setup SDIO using the HAL object and desired configuration */
    result = mtb_hal_sdio_setup(&sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config,
            NULL, &sdhc_host_context);

    /* SDIO setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Initialize and Enable SD HOST */
    Cy_SD_Host_Enable(CYBSP_WIFI_SDIO_HW);
    Cy_SD_Host_Init(CYBSP_WIFI_SDIO_HW,
            CYBSP_WIFI_SDIO_sdio_hal_config.host_config, &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(CYBSP_WIFI_SDIO_HW, CY_SD_HOST_BUS_WIDTH_4_BIT);

    sdio_hal_cfg.frequencyhal_hz = APP_SDIO_FREQUENCY_HZ;
    sdio_hal_cfg.block_size = SDHC_SDIO_64BYTES_BLOCK;

    /* Configure SDIO */
    mtb_hal_sdio_configure(&sdio_instance, &sdio_hal_cfg);

    /* Setup GPIO using the HAL object for WIFI WL REG ON  */
    mtb_hal_gpio_setup(&wcm_config.wifi_wl_pin, CYBSP_WIFI_WL_REG_ON_PORT_NUM,
            CYBSP_WIFI_WL_REG_ON_PIN);

    /* Setup GPIO using the HAL object for WIFI HOST WAKE PIN  */
    mtb_hal_gpio_setup(&wcm_config.wifi_host_wake_pin,
            CYBSP_WIFI_HOST_WAKE_PORT_NUM, CYBSP_WIFI_HOST_WAKE_PIN);

    /* Initialize the Host wakeup interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status_host_wake =
            Cy_SysInt_Init(&host_wake_intr_cfg, host_wake_interrupt_handler);

    /* Host wake up interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status_host_wake)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_HOST_WAKE_IRQ);
}

/*******************************************************************************
* Function Name: wifi_connect
********************************************************************************
* Summary:
*  This function executes a connect to the AP. The maximum number of
*  times it attempts to connect to the AP is specified by MAX_RETRY_COUNT.
*
* Parameters:
*  None
*
* Return:
*  cy_rslt_t: It contains the status of operation of connecting to the
*  specified AP.
*
*******************************************************************************/
static cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;
    memset(&connect_param, RESET_VAL, sizeof(cy_wcm_connect_params_t));
    memcpy(&connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(&connect_param.ap_credentials.password, WIFI_PASSWORD,
            sizeof(WIFI_PASSWORD));
    connect_param.ap_credentials.security = WIFI_SECURITY;
    APP_INFO(("Connecting to AP\n"));

   /* Attempt to connect to Wi-Fi until a connection is made or until
    * MAX_WIFI_RETRY_COUNT attempts have been made.
    */
    for(uint32_t conn_retries = RESET_VAL; conn_retries < MAX_WIFI_RETRY_COUNT;
            conn_retries++ )
    {
        result = cy_wcm_connect_ap(&connect_param, &ip_address);

        if(CY_RSLT_SUCCESS == result)
        {
            APP_INFO(("Successfully connected to Wi-Fi network '%s'.\n",
                    connect_param.ap_credentials.SSID));

            if (CY_WCM_IP_VER_V4 == ip_address.version)
            {
                APP_INFO(("Assigned IP address = %s\n",
                        ip4addr_ntoa((const ip4_addr_t *)&ip_address.ip.v4)));
            }
            else if(CY_WCM_IP_VER_V6 == ip_address.version)
            {
                APP_INFO(("Assigned IP address = %s\n",
                        ip6addr_ntoa((const ip6_addr_t *)&ip_address.ip.v6)));
            }

            break;
        }

        ERR_INFO(("Connection to Wi-Fi network failed with error code %d."
               "Retrying in %d ms...\n",
               (int)result, WIFI_CONN_RETRY_INTERVAL_MSEC));
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MSEC));
    }

    return result;
}

/*******************************************************************************
* Function Name: lowpower_task
********************************************************************************
* Summary:
*  The task initializes the Wi-Fi, LPA (Low-Power Assist middleware) and the OLM
*  (Offload Manager). The Wi-Fi then joins with Access Point with the provided
*  SSID and PASSWORD. After successfully connecting to the network the task
*  suspends the lwIP network stack indefinitely which helps RTOS to enter the
*  Idle state, and then eventually into deep-sleep power mode. The MCU will stay
*  in deep-sleep power mode till the network stack resumes. The network stack
*  resumes whenever any Tx/Rx activity detected in the EMAC interface (path
*  between Wi-Fi driver and network stack).
*
* Parameters:
*  void *arg: Task specific arguments. Never used.
*
* Return:
*  void: Should never return.
*
******************************************************************************/
void lowpower_task(void *arg)
{
    cy_rslt_t result;
    struct netif *wifi;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)
    
    /* SDHC SysPm callback registration */
    Cy_SysPm_RegisterCallback(&sdhcDeepSleepCallbackHandler);
    
#endif /* (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) */

    /* Initialize SDIO instance*/
    app_sdio_init();

    /* Configure SDIO interface instance */
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_AP_STA ;
    wcm_config.wifi_interface_instance = &sdio_instance;

    /* Initializes the Wi-Fi device and lwIP stack.*/
    result = cy_wcm_init(&wcm_config);

    if(CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to initialize Wi-Fi Connection Manager.\n"));
        handle_app_error();
    }

    /* Connect to Wi-Fi AP. */
    result = wifi_connect();

    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to connect to AP.\n"));
        handle_app_error();
    }

   /* Obtain the pointer to the lwIP network interface. This pointer is used to
    * access the Wi-Fi driver interface to configure the WLAN power-save mode.
    */
    wifi = (struct netif*)cy_network_get_nw_interface
                         (CY_NETWORK_WIFI_STA_INTERFACE, INTERFACE_ID);

    while (true)
    {
       /* Configures an emac activity callback to the Wi-Fi interface and
        * suspends the network if the network is inactive for a duration of
        * INACTIVE_WINDOW_MS inside an interval of INACTIVE_INTERVAL_MS. The
        * callback is used to signal the presence/absence of network activity
        * to resume/suspend the network stack.
        */
        wait_net_suspend(wifi, portMAX_DELAY, INACTIVE_INTERVAL_MS,
                INACTIVE_WINDOW_MS);

        /* Invert the User LED 1 when the device wakes up */
        Cy_GPIO_Inv(CYBSP_USER_LED_PORT,CYBSP_USER_LED_NUM);
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
        Cy_GPIO_Inv(CYBSP_USER_LED_PORT,CYBSP_USER_LED_NUM);
    }
}


/* [] END OF FILE */
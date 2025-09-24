[Click here](../README.md) to view the README.

## Design and implementation

The design of this application is minimalistic to get started with code examples on PSOC&trade; Edge MCU devices. All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project | Description
--------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. Resource initialization for this example is performed by this CM33 non-secure project. It configures the system clocks, pins, clock to peripheral connections, and other platform resources. It then enables the CM55 core using the `Cy_SysEnableCM55()` function and the CM55 core is subsequently put to DeepSleep mode.

In the CM33 non-secure application, the clocks and system resources are initialized by the BSP initialization function. The retarget-io middleware is configured to use the debug UART. The LPTimer is initialized to allow the system to enter deep sleep mode when the idle task is executed. A low power task is created that initializes the Wi-Fi device, configures it in the specified WLAN power save mode, and suspends the network stack indefinitely until there is network activity detected by the WLAN device.

This code example uses the [lwIP](https://savannah.nongnu.org/projects/lwip) network stack, which runs multiple network timers for various network-related activities. These timers need to be serviced by the host MCU. As a result, the host MCU will not be able to stay in sleep or deep sleep state longer.

In this example, after successfully connecting to an AP, the host MCU suspends the network stack after a period of inactivity. The example uses two macros: `INACTIVE_INTERVAL_MS` and `INACTIVE_WINDOW_MS` to determine whether the network is inactive. The host MCU monitors the network for inactivity in an interval of length `INACTIVE_INTERVAL_MS`. If the network is inactive for a continuous duration specified by the `INACTIVE_WINDOW_MS` macro, the network stack will be suspended until there is a network activity.

The host MCU is alerted by the WLAN device on network activity, after which the network stack resumes. The host MCU is in deep sleep when the network stack is suspended. Because there are no network timers to be serviced, the host MCU stays in deep sleep for longer. This state where the host MCU is in deep sleep waiting for network activity is referred to as the wait state. 

Note that Bluetooth&reg; domain is turned off by disabling the BT_POWER_PIN (P11_0). Ensure to enable this pin if the Bluetooth&reg; domain is used.

<br>
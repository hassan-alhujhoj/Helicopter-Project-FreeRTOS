/*
 * main.c
 *
 * Main source file for ENCE464 Heli project
 *
 *  Created on: 27/07/2020
 *  Contributers: Hassan Alhujhoj, Abdullah Naeem and Tim Hadler
 *  Last modified: 8.8.2020
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/pin_map.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pwm.h"

#include "utils/ustdlib.h"

#include "stdlib.h"

#include "OrbitOLED/OrbitOLEDInterface.h"

#include "OLEDDisplay.h"
#include "constants.h"
#include "motors.h"
#include "yaw.h"
#include "altitude.h"
#include "controllers.h"
#include "userInput.h"
#include "buttons4.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void displayOLED(void* pvParameters)
{
    const uint16_t delay_ms = 1000/DISPLAY_RATE_HZ;

    char text_buffer[16];
    while(1) {
        // Display Height
        sprintf(text_buffer, "Altitude: %d%%", getAlt());
        writeDisplay(text_buffer, LINE_1);
        // Display yaw
        sprintf(text_buffer, "Yaw: %d", getYaw());
        writeDisplay(text_buffer, LINE_2);
        // Target height
        sprintf(text_buffer, "Target Alt: %d%%", getTargetAlt());
        writeDisplay(text_buffer, LINE_3);
        // Target yaw
        sprintf(text_buffer, "Target Yaw: %d", getTargetYaw());
        writeDisplay(text_buffer, LINE_4);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

// Transmit a string via UART0
void UARTSend (char *pucBuffer)
{
    // Loop while there are more characters to send.
    while(*pucBuffer)
    {
        // Write the next character to the UART Tx FIFO.
        UARTCharPut(UART_USB_BASE, *pucBuffer);
        pucBuffer++;
    }
}

// Function to update UART communications
void sendData(void* pvParameters)
{
    uint8_t heliState;
    char* stateStr;
    char statusStr[16 + 1];
    const uint16_t delay_ms = 1000/UART_SEND_RATE_HZ;

    while(1) {
        heliState = getState();
        if (heliState == LANDED) {
            stateStr = "Landed";

        } else if (heliState == LANDING) {
            stateStr = "Landing";

        } else if (heliState == TAKE_OFF) {
            stateStr = "Take off";

        } else if (heliState == IN_FLIGHT) {
            stateStr = "In flight";

        } else {
            stateStr = "Intd state";
        }

        // Form and send a status message to the console
        sprintf (statusStr, "Alt %d [%d] \r\n", getAlt(), getTargetAlt()); // * usprintf
        UARTSend (statusStr);
        sprintf (statusStr, "Yaw %d [%d] \r\n", getYaw(), getTargetYaw()); // * usprintf
        UARTSend (statusStr);
        sprintf (statusStr, "Main %d Tail %d \r\n", getPWM(), getPWM() ); // * usprintf
        UARTSend (statusStr);
        sprintf(statusStr, "Mode: %s \r\n", stateStr);
        UARTSend (statusStr);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void createTasks(void)
{
    xTaskCreate(pollButtons, "Button Poll", 64, (void *) NULL, 3, NULL);
    xTaskCreate(displayOLED, "display", 256, (void *) NULL, 3, NULL);
    xTaskCreate(controller, "controller", 64, (void *) NULL, 2, NULL);
    xTaskCreate(processAlt, "Altitude Calc", 64, (void *) NULL, 4, NULL);
    xTaskCreate(sendData, "UART", 256, (void *) NULL, 3, NULL);
    xTaskCreate(FSM, "Finite State Machine", 64, (void *) NULL, 4, NULL);
}

// initialiseUSB_UART - 8 bits, 1 stop bit, no parity
void initialiseUSB_UART (void)
{
    // Enable GPIO port A which is used for UART0 pins.
    SysCtlPeripheralEnable(UART_USB_PERIPH_UART);
    SysCtlPeripheralEnable(UART_USB_PERIPH_GPIO);

    // Select the alternate (UART) function for these pins.
    GPIOPinTypeUART(UART_USB_GPIO_BASE, UART_USB_GPIO_PINS);
    GPIOPinConfigure (GPIO_PA0_U0RX);
    GPIOPinConfigure (GPIO_PA1_U0TX);

    UARTConfigSetExpClk(UART_USB_BASE, SysCtlClockGet(), BAUD_RATE,
            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
            UART_CONFIG_PAR_NONE);
    UARTFIFOEnable(UART_USB_BASE);
    UARTEnable(UART_USB_BASE);
}

// Initialize the program
void initialize(void)
{
    // Set clock to 80MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    initButtons();
    initModeSwitch();
    initADC();
    initDisplay();
    initMotors();
    initYaw();
    createTasks();
    initialiseUSB_UART();

    //// BUTTONS...
    GPIOPinTypeGPIOInput (LEFT_BUT_PORT_BASE, LEFT_BUT_PIN);
    GPIOPadConfigSet (LEFT_BUT_PORT_BASE, LEFT_BUT_PIN, GPIO_STRENGTH_2MA,
       GPIO_PIN_TYPE_STD_WPU);

    //---Unlock PF0 for the right button:
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R |= GPIO_PIN_0; //PF0 unlocked
    GPIO_PORTF_LOCK_R = GPIO_LOCK_M;
    GPIOPinTypeGPIOInput (RIGHT_BUT_PORT_BASE, RIGHT_BUT_PIN);
    GPIOPadConfigSet (RIGHT_BUT_PORT_BASE, RIGHT_BUT_PIN, GPIO_STRENGTH_2MA,
       GPIO_PIN_TYPE_STD_WPU);

    // For Reference signal
    SysCtlPeripheralEnable(REF_PERIPH);
    while (!SysCtlPeripheralReady(REF_PERIPH));
    GPIOPinTypeGPIOInput(REF_GPIO_BASE, REF_PIN);
    GPIOPadConfigSet (REF_GPIO_BASE, REF_PIN, GPIO_STRENGTH_4MA,
       GPIO_PIN_TYPE_STD_WPD);

    IntMasterEnable();
}

void main(void)
{
    initialize();
    vTaskStartScheduler();
}

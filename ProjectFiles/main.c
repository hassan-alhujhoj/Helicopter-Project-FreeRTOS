/*
 * main.c
 *
 *  Main source file for ENCE464 Heli project. This source file initialises the program and starts the scheduler out.
 *
 *  Created on: 27/07/2020
 *  Contributers: Hassan Alhujhoj, Abdullah Naeem and Tim Hadler
 *  Last modified: 08/08/2020
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "OLEDDisplay.h"
<<<<<<< HEAD
#include "constants.h"


//******************************************************************
// Global Variables
//******************************************************************
char text_buffer[16];    // global for now



void blinkLED(void *pvParameters) {
    uint8_t pin = (*(uint8_t *) pvParameters);
    uint8_t current = 0;

    while(1) {
        current ^= pin;
        GPIOPinWrite(LED_GPIO_BASE, pin, current);
        vTaskDelay(LED_BLINK_RATE / portTICK_RATE_MS);
    }
}
=======
#include "motors.h"
#include "yaw.h"
#include "altitude.h"
#include "controllers.h"
#include "userInput.h"
#include "debugger.h"
#include "OLEDDisplay.h"
>>>>>>> 3c95983310e1dc447b422e753fcfaa6b2d2cc8fb


// Initialize the program
void initialize(void)
{
    // Set clock to 80MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // INitialise all the peripherals and modules
    initUserInput();
    initADC();
    initDisplay();
    initMotors();
    initYaw();
    initialiseUSB_UART();

    // Create freeRTOS tasks
    xTaskCreate(pollButtons, "Button Polling", 64, (void *) NULL, 3, NULL);
    xTaskCreate(displayOLED, "OLED Display", 256, (void *) NULL, 3, NULL);
    xTaskCreate(controller, "Controller", 64, (void *) NULL, 2, NULL);
    xTaskCreate(processAlt, "Altitude Calc", 64, (void *) NULL, 4, NULL);
    xTaskCreate(sendData, "UART", 256, (void *) NULL, 3, NULL);
    xTaskCreate(FSM, "Finite State Machine", 64, (void *) NULL, 4, NULL);

    IntMasterEnable();
}


void main(void)
{
    initialize();
    vTaskStartScheduler();
}

/* HAL-only entry function */

#include "hal_data.h"
#include <stdio.h>
#include <string.h>

// Buffer Size
#define UART_BUFFER_SIZE 1024

// Buffers
char outputBuffer[UART_BUFFER_SIZE];

// Flags
volatile bool transmitComplete;

int _write(int file, char *buffer, int count);
int _write(int file, char *buffer, int count)
{
    // As far as I know, there isn't a way to retrieve how many
    // bytes were send on using the uart->write function if it does not return
    // SSP_SUCCESS (unless we want to use the tx interrupt function and a global counter
    // so, we will send each character one by one instead.
    int bytesTransmitted = 0;

    for (int i = 0; i < count; i++)
    {
        // Start Transmission
        transmitComplete = false;
        g_uart.p_api->write (g_uart.p_ctrl, (uint8_t const *) (buffer + i), 1);
        while (!transmitComplete)
        {
        }

        bytesTransmitted++;
    }

    return bytesTransmitted;
}

// Callback Function for UART interrupts
void user_uart_callback(uart_callback_args_t * p_args)
{
    // Get Event Type
    switch (p_args->event)
    {
        // Transmission Complete
        case UART_EVENT_TX_COMPLETE:
            transmitComplete = true;
        break;
        default:
        break;
    }
}

void hal_entry(void)
{
    uint8_t *deviceAddress = (uint8_t *) 0x60000000;

    bool isWriting;
    uint8_t writeData[] = "The quick brown fox jumps over the lazy dog.";
    uint8_t readData[sizeof(writeData)];
    uint8_t initialData[1024];

    // Open UART
    g_uart.p_api->open (g_uart.p_ctrl, g_uart.p_cfg);

    // Disable Output Buffering
    setvbuf ( stdout, NULL, _IONBF, UART_BUFFER_SIZE);

    // Open QSPI
    g_qspi.p_api->open (g_qspi.p_ctrl, g_qspi.p_cfg);

    // Use TTY100 commands to clear screen and reset screen pointer
    printf ("\033[2J"); // Clear Screen
    printf ("\033[H"); // Return Home
    printf ("\033[3J"); // Clear Back Buffer

    // Print Header
    printf ("Lesson 010: QSPI Flash\r\n\r\n");

    // Read Initial Flash Data
    g_qspi.p_api->read (g_qspi.p_ctrl, deviceAddress, (uint8_t *) initialData, sizeof(initialData));
    printf ("Initial Flash Data: '%s'\r\n", initialData);

    // Erasing Flash
    printf ("Erasing Flash\r\n");
    g_qspi.p_api->sectorErase (g_qspi.p_ctrl, deviceAddress);

    // Wait for flash to be idle (not writing)
    printf ("Waiting for Flash to be idle...");
    while (true)
    {
        g_qspi.p_api->statusGet (g_qspi.p_ctrl, &isWriting);
        if (isWriting)
            continue;
        break;
    }
    printf ("OK!\r\n\r\n");

    // Write data to flash
    printf ("Writing '%s' to Flash.\r\n\r\n", writeData);
    g_qspi.p_api->pageProgram (g_qspi.p_ctrl, deviceAddress, writeData, sizeof(writeData));

    // Wait for flash to be idle (not writing)
    printf ("Waiting for Flash to be idle...");
    while (true)
    {
        g_qspi.p_api->statusGet (g_qspi.p_ctrl, &isWriting);
        if (isWriting)
            continue;
        break;
    }
    printf ("OK!\r\n\r\n");

    // Read Flash Data
    g_qspi.p_api->read (g_qspi.p_ctrl, deviceAddress, (uint8_t *) readData, sizeof(readData));
    printf ("Flash Data: '%s'\r\n", readData);

    // Close Flash
    g_qspi.p_api->close (g_qspi.p_ctrl);

    // Endless Loop
    while (true)
    {
    }

}

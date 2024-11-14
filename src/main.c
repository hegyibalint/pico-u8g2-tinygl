#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/assert.h>
#include <hardware/spi.h>

#include "u8g2.h"
#include "zbuffer.h"
#include "GL/gl.h"

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 64
#define DISPLAY_PAGE_SIZE 8

static ZBuffer *frame_buffer;
static u8g2_t display;

const char *message = "Hello, World!";

// Function to adopt SPI functions for the display
uint8_t u8x8_byte_hw_spi_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    int err;
    const uint8_t *data = (uint8_t *)arg_ptr;
    const uint8_t data_len = arg_int;

    switch (msg)
    {
    case U8X8_MSG_BYTE_SEND:
        int written = spi_write_blocking(spi0, (const uint8_t *)arg_ptr, arg_int);
        // printf("  Written %d bytes\n", written);
        break;
    case U8X8_MSG_BYTE_INIT:
        // Setup SPI
        spi_init(spi0, 4000 * 1000);
        spi_set_format(
            spi0,
            8, // Data bits
            0, // CPOL
            0, // CPHA
            SPI_MSB_FIRST);
        // Configure SPI pins
        gpio_set_function(2, GPIO_FUNC_SPI);
        gpio_set_function(3, GPIO_FUNC_SPI);
        // gpio_set_function(4, GPIO_FUNC_SPI); // MOSI is not connected to the display
        gpio_set_function(5, GPIO_FUNC_SPI);
        break;
    case U8X8_MSG_BYTE_SET_DC:
        // printf("Set Byte DC: %d\n", arg_int);
        gpio_put(6, arg_int);
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        // Start SPI transfer
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        // End SPI transfer
        break;
    default:
        printf("Unknown message: %d\n", msg);
        return 0;
    }
    return 1;
}

// Function to adopt GPIO and delay functions for the display
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // Configure DC pin
        gpio_init(6);
        gpio_set_dir(6, GPIO_OUT);
        // Configure Reset pin
        gpio_init(7);
        gpio_set_dir(7, GPIO_OUT);
        break;
    case U8X8_MSG_GPIO_CS:
        break;
    case U8X8_MSG_GPIO_DC:
        gpio_put(6, arg_int);
        break;
    case U8X8_MSG_GPIO_RESET:
        gpio_put(7, arg_int);
        break;
    case U8X8_MSG_DELAY_MILLI:
        // Delay for the number of milliseconds passed in arg_int
        sleep_ms(arg_int);
        break;
    case U8X8_MSG_DELAY_100NANO:
        // Delay for approximately 100 nanoseconds
        // Caution: sleep_us is the closest available but 1 microsecond is the minimum
        sleep_us(1);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        // Delay for approximately 10 microseconds
        sleep_us(10);
        break;
    default:
        printf("Unknown message: %d\n", msg);
        return 0; // A message was received that is not handled
    }
    return 1; // Command processed successfully
}

void flush_frame_buffer()
{
    u8g2_SendBuffer(&display);
}

void setup_u8g2()
{
    // u8g2_Setup_sh1122_256x64_XX done by hand, so we can set our own buffer
    u8g2_SetupDisplay(&display, u8x8_d_sh1122_256x64, u8x8_cad_001, u8x8_byte_hw_spi_pico, u8x8_gpio_and_delay_pico);
    u8g2_SetupBuffer(&display, frame_buffer->pbuf, DISPLAY_PAGE_SIZE, u8g2_ll_hvline_horizontal_right_lsb, U8G2_R0);

    u8g2_InitDisplay(&display);
    u8g2_SetPowerSave(&display, 0);

    // Draw a rectangle
    u8g2_ClearBuffer(&display);
    u8g2_SetFont(&display, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&display, 0, 20, "Initialized");
    flush_frame_buffer();
}

void setup_tinygl()
{
    glInit(frame_buffer);
    glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    // Set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
    // Set up the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up the dithering map
    ZB_setDitheringMap(frame_buffer, DITHER_MAP_BAYER4);
}

void loop_noop()
{
    while (true)
    {
        sleep_ms(1000);
    }
}

void loop_triangles()
{
    // Clear the buffer using tinygl
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the modelview matrix
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f); // Move the triangle back so it's visible

    // Draw a simple triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glColor3f(0, 1, 0);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glColor3f(0, 0, 1);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();

    // Send the buffer to the display
    while (true)
    {
        flush_frame_buffer();
    }
}

void loop_cubes()
{
    int width = u8g2_GetDisplayWidth(&display);
    int height = u8g2_GetDisplayHeight(&display);
    int t = 0;
    while (true)
    {
        sleep_ms(1000 / 60);
        u8g2_ClearBuffer(&display);
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                if ((x + t) % 8 < 4 && (y + t) % 8 < 4)
                {
                    u8g2_DrawPixel(&display, x, y);
                }
            }
        }
        flush_frame_buffer();
        t = t < 7 ? t + 1 : 0;
    }
}

int main()
{
    stdio_init_all();

    // Create the shared frame buffer
    frame_buffer = ZB_open(DISPLAY_WIDTH, DISPLAY_HEIGHT, ZB_MODE_INDEX, 0);
    setup_u8g2();
    setup_tinygl();
    loop_triangles();

    return 0;
}
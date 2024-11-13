#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/spi.h>

#include "u8g2.h"
#include "zbuffer.h"
#include "GL/gl.h"

static u8g2_t u8g2;
static ZBuffer *frame_buffer;

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
        printf("Start SPI transfer\n");
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        // End SPI transfer
        printf("End SPI transfer\n");
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
        printf("CS handled by HW SPI\n");
        break;
    case U8X8_MSG_GPIO_DC:
        printf("Set GPIO DC: %d\n", arg_int);
        gpio_put(6, arg_int);
        break;
    case U8X8_MSG_GPIO_RESET:
        printf("Set Reset: %d\n", arg_int);
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

void setup_u8g2()
{
    u8g2_Setup_sh1122_256x64_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_hw_spi_pico,
        u8x8_gpio_and_delay_pico);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    printf("Display initialized\n");

    // Draw a rectangle
    printf("Drawing\n");
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 20, "Hello, World!!");
    u8g2_SendBuffer(&u8g2);
    printf("Drawing finished\n");
}

void setup_tinygl()
{
    int width = u8g2_GetDisplayWidth(&u8g2);
    int height = u8g2_GetDisplayHeight(&u8g2);
    frame_buffer = ZB_open(width, height, ZB_MODE_INDEX, 0);
    glInit(frame_buffer);
    glViewport(0, 0, width, height);
}

void print_frame_buffer()
{
    int width = u8g2_GetDisplayWidth(&u8g2);
    assert(width == frame_buffer->xsize);

    int height = u8g2_GetDisplayHeight(&u8g2);
    assert(height == frame_buffer->ysize);

    u8g2_ClearBuffer(&u8g2);
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            if (frame_buffer->pbuf[x + y * width] != 0)
            {
                u8g2_DrawPixel(&u8g2, x, y);
            }
        }
        printf("\n");
    }
    u8g2_SendBuffer(&u8g2);
}

void loop_triangles()
{
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glPushMatrix();
    glRotatef(1.5, 0, 0, 1);

    glBegin(GL_TRIANGLES);
    glColor3f(1, 1, 1);
    glVertex3f(-0.9, -0.9, 0.0);
    glVertex3f(0.9, -0.9, 0.0);
    glVertex3f(0, 0.9, 0.0);
    glEnd();
    glPopMatrix();

    ZB_setDitheringMap(frame_buffer, 3);

    print_frame_buffer();
    loop_noop();
}

void loop_cubes()
{
    int width = u8g2_GetDisplayWidth(&u8g2);
    int height = u8g2_GetDisplayHeight(&u8g2);
    int t = 0;
    while (true)
    {
        sleep_ms(1000 / 60);
        u8g2_ClearBuffer(&u8g2);
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                if ((x + t) % 8 < 4 && (y + t) % 8 < 4)
                {
                    u8g2_DrawPixel(&u8g2, x, y);
                }
            }
        }
        u8g2_SendBuffer(&u8g2);
        t = t < 7 ? t + 1 : 0;
    }
}

void loop_noop()
{
    while (true)
    {
        sleep_ms(1000);
    }
}

int main()
{
    stdio_init_all();
    setup_u8g2();
    setup_tinygl();
    loop_triangles();

    return 0;
}
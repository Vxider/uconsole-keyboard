#include "main.h"
#include "keyboard_matrix.h"
#include "keyboard_state.h"
#include "keymaps.h"
#include "stm32f1xx_hal.h"
#include <assert.h>

#define KEYBOARD_PULL 0 // 0 for PULLDOWN

static uint8_t matrix[MATRIX_ROWS];
static uint8_t matrix_prev[MATRIX_ROWS];
static uint8_t acive_column;

// Pin definitions for rows and cols
static GPIO_TypeDef* matrix_rows_port[MATRIX_ROWS] = {
    ROW1_GPIO_Port, ROW2_GPIO_Port, ROW3_GPIO_Port, ROW4_GPIO_Port,
    ROW5_GPIO_Port, ROW6_GPIO_Port, ROW7_GPIO_Port, ROW8_GPIO_Port
};

static uint16_t matrix_rows_pin[MATRIX_ROWS] = {
    ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin,
    ROW5_Pin, ROW6_Pin, ROW7_Pin, ROW8_Pin
};

static GPIO_TypeDef* matrix_cols_port[MATRIX_COLS] = {
    COL1_GPIO_Port, COL2_GPIO_Port, COL3_GPIO_Port, COL4_GPIO_Port,
    COL5_GPIO_Port, COL6_GPIO_Port, COL7_GPIO_Port, COL8_GPIO_Port
};

static uint16_t matrix_cols_pin[MATRIX_COLS] = {
    COL1_Pin, COL2_Pin, COL3_Pin, COL4_Pin,
    COL5_Pin, COL6_Pin, COL7_Pin, COL8_Pin
};

static uint8_t read_kbd_io(uint8_t row)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(matrix_rows_port[row], matrix_rows_pin[row]);
    
#if KEYBOARD_PULL == 0
    return (state == GPIO_PIN_SET) ? 1 : 0;
#else
    return (state == GPIO_PIN_RESET) ? 1 : 0;
#endif
}

static void next_column(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_TypeDef* port = matrix_cols_port[0];

    acive_column = (acive_column + 1) % MATRIX_COLS;

    // set other column pins as input
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        if (col == acive_column) continue;
        GPIO_InitStruct.Pin |= matrix_cols_pin[col];
        assert(port == matrix_cols_port[col]);
    }
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
#if KEYBOARD_PULL == 1
        GPIO_InitStruct.Pull = GPIO_PULLUP;
#else
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
#endif
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // set the new column pin as output
    GPIO_InitStruct.Pin = matrix_cols_pin[acive_column];
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(matrix_cols_port[acive_column], &GPIO_InitStruct);
#if KEYBOARD_PULL == 1
    HAL_GPIO_WritePin(matrix_cols_port[acive_column], matrix_cols_pin[acive_column], GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(matrix_cols_port[acive_column], matrix_cols_pin[acive_column], GPIO_PIN_SET);
#endif
}

void matrix_init(void)
{
    // Columns are already configured as outputs in MX_GPIO_Init
    // Rows are already configured as inputs with pulldown in MX_GPIO_Init
    
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
        matrix_prev[i] = 0;
    }

    next_column();

    HAL_Delay(500);
}

static bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & (1 << col));
}

static uint8_t matrix_scan(void)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        if (read_kbd_io(row)) {
            if (!matrix_is_on(row, acive_column)) {
                matrix[row] |= (uint8_t)(1 << acive_column);
                matrix_action(row, acive_column, KEY_PRESSED);
                keyboard_state.last_activity_time = HAL_GetTick();
            }
        } else {
            if (matrix_is_on(row, acive_column)) {
                matrix[row] &= ~(uint8_t)(1 << acive_column);
                matrix_action(row, acive_column, KEY_RELEASED);
                keyboard_state.last_activity_time = HAL_GetTick();
            }
        }
    }

    // Set the next column as active, for the next scan
    next_column();

    return 1;
}

void matrix_task(void)
{
    matrix_scan();    
}

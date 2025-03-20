// https://sourcevu.sysprogs.com/stm32/HAL/files/Src/stm32f4xx_hal_spi.c
// https://sourcevu.sysprogs.com/stm32/HAL/files/Src/stm32f4xx_hal_gpio.c

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// pytest_c_testrunner
#include "test_macros_uint32.h"

#define DEBUG //printf ("Call %s\n", __FUNCTION__)

static struct device_context {
  uint8_t command_register;
  uint8_t buffer[2];
  //bool in_progress;
  uint8_t store_one_byte;
  bool hold_bar;
  // JEDEC
  #define JEDEC 0x9F
  #define JEDEC_COUNT 3
  #define JEDEC_BYTE_1 0xBF
  #define JEDEC_BYTE_2 0x25
  #define JEDEC_BYTE_3 0x8D
  uint8_t jedec [JEDEC_COUNT];
  uint8_t jedec_counter_internal;
  bool jedec_in_progress;
  // READ-ID
  #define READ_ID_1 0x90
  #define READ_ID_2 0xAB
  #define READ_ID_M 0
  #define READ_ID_D 1
  uint8_t read_id [2];
  bool read_id_zero;
  uint8_t read_id_type;
  bool read_id_receive;
  uint8_t read_id_count;
} dev_ctx = {
  .read_id = { 0xBF, 0x8D },
  .jedec = { JEDEC_BYTE_1, JEDEC_BYTE_2, JEDEC_BYTE_3 },
  .hold_bar = true
};
  
// HAL emulation

#define GPIOD NULL
#define GPIO_PIN_7 7
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0

typedef uint32_t SPI_HandleTypeDef;
typedef uint32_t GPIO_TypeDef;
typedef uint32_t GPIO_PinState;

typedef enum 
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

HAL_StatusTypeDef
HAL_SPI_Transmit (SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
  DEBUG;
  printf ("-> [S] BYTE = 0x%x\n", *pData);
  if (dev_ctx.hold_bar == 0) { // XXX HOLD# is enable, nothing to do
    printf ("HOLD# mode, SPI SO HIGH IMPEDANCE\n");
    return HAL_ERROR;
  }
/*  if (dev_ctx.in_progress == true) {*/
/*    printf ("Device in progress\n");*/
/*    return HAL_BUSY;*/
/*  }*/
  switch (*pData) {
    case JEDEC :
      //printf ("-> [S] Command Register 0x%x\n", *pData);
      //dev_ctx.in_progress = true;
      dev_ctx.jedec_in_progress = true;
      dev_ctx.command_register = *pData;
      break;
    case READ_ID_1:
    case READ_ID_2:
      //printf ("-> [S] Command Register 0x%x\n", *pData);
      dev_ctx.command_register = *pData;
      break;
    case 0x00 :
    case 0x01 :
      if (dev_ctx.command_register == READ_ID_1 || dev_ctx.command_register == READ_ID_2) {
        if (*pData == 0x00 && dev_ctx.read_id_count == 2) {
          printf ("ADD type 0\n");
          dev_ctx.read_id_count = 0;
          //dev_ctx.in_progress = true;
          dev_ctx.read_id_type = 1;
          dev_ctx.read_id_zero = false;
        }
        else
        if (*pData == 0x01 && dev_ctx.read_id_count == 2) {
          printf ("ADD type 1\n");
          dev_ctx.read_id_count = 0;
          //dev_ctx.in_progress = true;
          dev_ctx.read_id_type = 2;
          dev_ctx.read_id_zero = false;
        }
        else
        if (*pData == 0x00) {
          dev_ctx.read_id_count++;
          printf ("Receive 0x00\n");
        }
        else
        dev_ctx.read_id_count = 0;
      }
      break;
    default :  // When receive, read byte value +1
      //dev_ctx.read_id_count = 0;
      //dev_ctx.read_id_type = 0;
      dev_ctx.command_register = 0x00;
      dev_ctx.store_one_byte = *pData;
      dev_ctx.store_one_byte++;
      if (dev_ctx.store_one_byte > 255)
        dev_ctx.store_one_byte = 0;
      break;
  }
  return HAL_OK;
}

HAL_StatusTypeDef
HAL_SPI_Receive (SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
  DEBUG;
  if (dev_ctx.hold_bar == 0) { // XXX HOLD# is enable, nothing to do
    printf ("HOLD# mode, SPI SO HIGH IMPEDANCE\n");
    return HAL_ERROR; // XXX When HOLD, memory can receive and buffer some commands ?
  }
/*  if (dev_ctx.in_progress == true) {*/
/*    printf ("Device in progress\n");*/
/*    return HAL_BUSY;*/
/*  }*/
  //printf ("-> [R] Command Register 0x%x\n", dev_ctx.command_register);
  switch (dev_ctx.command_register) {
    case JEDEC :
      if (dev_ctx.jedec_in_progress == true) {
        *pData = dev_ctx.jedec [dev_ctx.jedec_counter_internal];
        dev_ctx.jedec_counter_internal++;
        if (dev_ctx.jedec_counter_internal == JEDEC_COUNT) {
          dev_ctx.command_register = 0x00;
          dev_ctx.jedec_in_progress = false;
          dev_ctx.jedec_counter_internal = 0;
          //dev_ctx.in_progress = false;
        }
      }
      break;
    case READ_ID_1 :
    case READ_ID_2 :
      //if (dev_ctx.in_progress == true) {
        switch (dev_ctx.read_id_type) {
          case 1 :
            if (dev_ctx.read_id_zero == false) {
              *pData = dev_ctx.read_id [READ_ID_M];
              dev_ctx.read_id_zero = true;
            }
            else {
              *pData = dev_ctx.read_id [READ_ID_D];
              dev_ctx.read_id_type = 0;
              dev_ctx.command_register = 0x00;
              //dev_ctx.in_progress = false;
            }
            break;
          case 2 :
            if (dev_ctx.read_id_zero == false) {
              *pData = dev_ctx.read_id [READ_ID_D];
              dev_ctx.read_id_zero = true;
            }
            else {
              *pData = dev_ctx.read_id [READ_ID_M];
              dev_ctx.read_id_type = 0;
              dev_ctx.command_register = 0x00;
              //dev_ctx.in_progress = false;
            }
            break;
          default :
            printf ("dev_ctx.read_id_type");
        }
      //}
      break;
    default :
      *pData = dev_ctx.store_one_byte;
      break;
  }
  printf ("-> [R] BYTE = 0x%x\n", *pData);
  return HAL_OK;
}

void
HAL_GPIO_WritePin (GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
  DEBUG;
  (void)GPIOx;
  (void)GPIO_Pin;
  (void)PinState;
  if (GPIO_Pin == GPIO_PIN_7) {
    switch (PinState) {
      case GPIO_PIN_SET : // DISABLE
        dev_ctx.hold_bar = 1;
        break;
      case GPIO_PIN_RESET : // ENABLE
        dev_ctx.hold_bar = 0;
        break;
      default:
        printf ("Hold error\n");
    }
  }
} 

// assistances:
// airc flash_SST25VF016B.c

SPI_HandleTypeDef hspi1;

void disableFlash(void)
{
  DEBUG;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
}

void enableFlash(void)
{
  DEBUG;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}

void SendByte(uint8_t data)
{
  DEBUG;
  HAL_SPI_Transmit(&hspi1, &data, 1, 0xffff);
}

uint8_t ReceiveByte(void)
{
  DEBUG;
  uint8_t data;
  HAL_SPI_Receive(&hspi1, &data, 1, 0xffff);
  return data;
}

// From datasheet

// 4.4.15 JEDEC READ-ID (p. 17)
uint32_t read_id_jedec_normal_flow_1 (void) { // normal flow
  DEBUG;
  uint8_t data = 0;
  uint32_t jedec = 0x00000000;
  enableFlash();
  SendByte(JEDEC);
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  disableFlash();
  return jedec;
}

uint32_t read_id_jedec_break_flow_1 (void) { // break flow
  DEBUG;
  uint8_t data = 0;
  uint32_t jedec = 0x00000000;
  enableFlash();
  SendByte(JEDEC);
  //enableFlash();
  disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  disableFlash();
  return jedec;
}

uint32_t read_id_jedec_break_flow_2 (void) { // break flow
  DEBUG;
  uint8_t data = 0;
  uint32_t jedec = 0x00000000;
  enableFlash();
  SendByte(JEDEC);
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  disableFlash();
  return jedec;
}

uint32_t read_id_jedec_break_flow_3 (void) { // break flow
  DEBUG;
  uint8_t data = 0;
  uint32_t jedec = 0x00000000;
  enableFlash();
  SendByte(JEDEC);
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  //disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  //enableFlash();
  disableFlash();
  data = ReceiveByte();
  jedec = (jedec << 8) | data;
  disableFlash();
  return jedec;
}

// 4.4.16 READ-ID (RDID) (p. 18)

uint32_t read_id_1a (uint8_t read_id_command, uint8_t read_id_type) { // SEND 0x90, READ_ID ADD 1
  DEBUG;
  uint8_t data = 0;
  uint32_t read_id = 0x00000000;
  enableFlash();
  SendByte(read_id_command);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(read_id_type); // 0x01,0x00
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  disableFlash();
  return read_id;
}

uint32_t read_id_1b (uint8_t read_id_command, uint8_t read_id_type) { // SEND 0x90, READ_ID ADD 1, break flow 1
  DEBUG;
  uint8_t data = 0;
  uint32_t read_id = 0x00000000;
  enableFlash();
  SendByte(read_id_command);
  SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(read_id_type); // 0x01,0x00
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  disableFlash();
  return read_id;
}

uint32_t read_id_1c (uint8_t read_id_command, uint8_t read_id_type) { // SEND 0x90, READ_ID ADD 1, break flow 2
  DEBUG;
  uint8_t data = 0;
  uint32_t read_id = 0x00000000;
  enableFlash();
  SendByte(read_id_command);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(read_id_type); // 0x01,0x00
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  disableFlash();
  return read_id;
}

uint32_t read_id_1d (uint8_t read_id_command, uint8_t read_id_type) { // SEND 0x90, READ_ID ADD 1, break flow 3
  DEBUG;
  uint8_t data = 0;
  uint32_t read_id = 0x00000000;
  enableFlash();
  SendByte(read_id_command);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  SendByte(0xff); // Break READ_ID command
  SendByte(read_id_type); // 0x01,0x00
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  disableFlash();
  return read_id;
}

uint32_t read_id_1e (uint8_t read_id_command, uint8_t read_id_type) { // SEND 0x90, READ_ID ADD 1, send FF after 0x90,0x00,0x00,0x01
  DEBUG;
  uint8_t data = 0;
  uint32_t read_id = 0x00000000;
  enableFlash();
  SendByte(read_id_command);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  //SendByte(0xff); // Break READ_ID command
  SendByte(0x00);
  SendByte(read_id_type); // 0x01,0x00
  SendByte(0xff); // Break READ_ID command
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  data = ReceiveByte();
  read_id = read_id << 8 | data;
  disableFlash();
  return read_id;
}










/*uint32_t read_id_2 (void) { // SEND 0xAB*/
/*  DEBUG;*/
/*  uint8_t data = 0;*/
/*  uint32_t read_id = 0x00000000;*/
/*  enableFlash();*/
/*  SendByte(READ_ID_2);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x00);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x00);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x01); // 0x01,0x00*/
/*  data = ReceiveByte();*/
/*  read_id = read_id << 8 | data;*/
/*  data = ReceiveByte();*/
/*  read_id = read_id << 8 | data;*/
/*  return read_id;*/
/*}*/

/*uint32_t read_id_2 (void) { // */
/*  DEBUG;*/
/*  uint8_t data = 0;*/
/*  uint32_t read_id = 0x00000000;*/
/*  enableFlash();*/
/*  SendByte(READ_ID_2);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x00);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x00);*/
/*  //SendByte(0xff); // Break READ_ID command*/
/*  SendByte(0x01); // 0x01,0x00*/
/*  data = ReceiveByte();*/
/*  read_id = read_id << 8 | data;*/
/*  data = ReceiveByte();*/
/*  read_id = read_id << 8 | data;*/
/*  return read_id;*/
/*}*/

int
main (int argc, char *argv[]) {
  // JEDEC
  ASSERT_EQUAL_UINT32     (read_id_jedec_normal_flow_1 (),  0x00bf258d,   "read jedec - normal flow   - good value");
  ASSERT_NOT_EQUAL_UINT32 (read_id_jedec_normal_flow_1 (),  0x00bf258f,   "read jedec - normal flow   - bad value, noise on SPI DO");
  ASSERT_EQUAL_UINT32     (read_id_jedec_break_flow_1 (),   0x00000000,   "read jedec - break flow 1  - disableFlash() at top");
  ASSERT_EQUAL_UINT32     (read_id_jedec_break_flow_2 (),   0x00000000,   "read jedec - break flow 2  - disableFlash() in middle");
  ASSERT_EQUAL_UINT32     (read_id_jedec_break_flow_3 (),   0x00000000,   "read jedec - break flow 3  - disableFlash() at bottom");
  // READ-ID ADD 1
  ASSERT_EQUAL_UINT32     (read_id_1a (READ_ID_1, 0x01), 0x00008dbf, "read id 1a - SEND 0x90, READ_ID ADD 1");
  ASSERT_EQUAL_UINT32     (read_id_1b (READ_ID_1, 0x01), 0x00000000, "read id 1b - SEND 0x90, READ_ID ADD 1, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1c (READ_ID_1, 0x01), 0x00000000, "read id 1c - SEND 0x90, READ_ID ADD 1, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1d (READ_ID_1, 0x01), 0x00000000, "read id 1d - SEND 0x90, READ_ID ADD 1, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1e (READ_ID_1, 0x01), 0x00008dbf, "read_id_1e - SEND 0x90, READ_ID ADD 1, send FF after 0x90,0x00,0x00,0x01");
  // READ-ID ADD 0
  ASSERT_EQUAL_UINT32     (read_id_1a (READ_ID_1, 0x00), 0x0000bf8d, "read id 1a - SEND 0x90, READ_ID ADD 0");
  ASSERT_EQUAL_UINT32     (read_id_1b (READ_ID_1, 0x00), 0x00000000, "read id 1b - SEND 0x90, READ_ID ADD 0, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1c (READ_ID_1, 0x00), 0x00000000, "read id 1c - SEND 0x90, READ_ID ADD 0, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1d (READ_ID_1, 0x00), 0x00000000, "read id 1d - SEND 0x90, READ_ID ADD 0, break flow 1");
  ASSERT_EQUAL_UINT32     (read_id_1e (READ_ID_1, 0x00), 0x0000bf8d, "read_id_1e - SEND 0x90, READ_ID ADD 0, send FF after 0x90,0x00,0x00,0x00");
  // *** Debug ***
  // return -1
  int8_t data;
  enableFlash();
  SendByte (0xfe);
  data = ReceiveByte ();
  printf ("%d\n", data);
  disableFlash();
  // Flash is disabled now
  SendByte (0xfe);
  data = ReceiveByte ();
  printf ("%d\n", data);
  return 0;
}


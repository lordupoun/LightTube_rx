#ifndef SI4432_IO
#define SI4432_IO

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"

void SI44_Read(uint8_t reg, uint8_t * buf, uint8_t length);
void SI44_Write(uint8_t reg, uint8_t * buf, uint8_t length);
void SI44_IO_Init(SPI_HandleTypeDef * hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void SI44_Read_IT(uint8_t reg, uint8_t * buf, uint8_t length);
void SI44_Write_IT(uint8_t reg, uint8_t * buf, uint8_t length);
void SI44_NSS_Set(void);
#endif

#include "si4432_io_hal.h"
#include <string.h> //for memcpy

static SPI_HandleTypeDef * spi_interface; //static
static GPIO_TypeDef * nss_port;
static uint16_t nss_pin;

//static uint8_t si44_read_reg;
//static uint8_t *si44_read_buf;
//static uint8_t si44_read_length;

void SI44_Read(uint8_t reg, uint8_t * buf, uint8_t length)
{
	//addreses range of SI is only 7 bit 0b0xxxxxxx/0b1xxxxxxx -> MSB 8bit defines whether its read from register or write to register operation
	//reg = reg & 0b01111111; //makes sure its read operation, not nessesarry; predano hodnotou, muzu menit
	//ToDo: Change to IT (DMA has greater overhead and IT will be more efficeint for <64 bytes)
    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_RESET);

    HAL_SPI_Transmit(spi_interface, &reg, 1, 100);

    HAL_SPI_Receive(spi_interface, buf, length, 100);

    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_SET);
}


void SI44_Write(uint8_t reg, uint8_t * buf, uint8_t length)
{
    uint8_t b[length + 1]; //ToDo: allocate non variable; ToDo: memcpy
    b[0] = reg | 0b10000000; //write operation; (MSB=0 mean read)
    memcpy(&b[1], buf, length);
    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi_interface, b, length + 1, 100);
    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_SET);
}

//Not working
void SI44_Read_IT(uint8_t reg, uint8_t * buf, uint8_t length)
{
	//addreses range of SI is only 7 bit 0b0xxxxxxx/0b1xxxxxxx -> MSB 8bit defines whether its read from register or write to register operation
	//reg = reg & 0b01111111; //makes sure its read operation, not nessesarry; predano hodnotou, muzu menit
	//ToDo: Change to IT (DMA has greater overhead and IT will be more efficeint for <64 bytes)
    si44_read_reg = reg;
    si44_read_buf = buf;
    si44_read_length = length;

	HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_RESET);


    HAL_SPI_Transmit(spi_interface, &si44_read_reg, 1, 100);


    HAL_SPI_Receive_IT(spi_interface, si44_read_buf, si44_read_length);
}

//Not working
void SI44_Write_IT(uint8_t reg, uint8_t * buf, uint8_t length)
{
    /*static uint8_t b[length + 1]; //ToDo: allocate non variable; ToDo: memcpy
    b[0] = reg | 0b10000000; //write operation; (MSB=0 mean read)
    memcpy(&b[1], buf, length);
    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit_IT(spi_interface, b, length + 1);*/
    si44_tx_buf[0] = reg | 0b10000000;
    memcpy(&si44_tx_buf[1], buf, length);

    HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit_IT(spi_interface, si44_tx_buf, length + 1);
}

void SI44_IO_Init(SPI_HandleTypeDef * hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    spi_interface = hspi;
    nss_port = GPIOx;
    nss_pin = GPIO_Pin;
}

void SI44_NSS_Set(void)
{
	HAL_GPIO_WritePin(nss_port, nss_pin, GPIO_PIN_SET);
}

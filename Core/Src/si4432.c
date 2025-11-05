#include <si4432.h>
#include "si4432_config.h"
#include "si4432_io_hal.h"
#include <stdio.h>
//#include "stm32f1xx_hal_spi.h"

/*------Private functions -------*/
void SI44_ClearTXFIFO(void);
//-----new------
void SI44_ClearRXFIFO(void);
//-------------------------------*/


uint8_t SI44_ReadStatus(void)
{
    uint8_t buf[1];
    SI44_Read(SI44_REG_STATUS, buf, 1);
    return buf[0];
}

void SI44_Reset(void)
{
    uint8_t reset = SI44_OPERATION_MODE_DEFAULT | 0b10000000;
    SI44_Write(SI44_REG_CTRL1, &reset, 1);
}

void SI44_Init(SPI_HandleTypeDef * hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    SI44_IO_Init(hspi, GPIOx, GPIO_Pin);
    SI44_Reset();
    HAL_Delay(1000); //ToDo: Replace with registry check for Reset done
}

void SI44_SetTXPower(SI44_TX_POWER power)
{
    uint8_t reg_6d;
    //cte aktualni hodnotu
    SI44_Read(SI44_REG_TX_POWER, &reg_6d, 1);
    //vynuluje TX cast
    reg_6d &= 0b11111000;

    reg_6d |= power;

    SI44_Write(SI44_REG_TX_POWER, &reg_6d, 1);
}

void SI44_SendPacket(uint8_t * buf, uint8_t length)
{
    uint8_t b;
    SI44_ClearTXFIFO();
    //SI44_Read(0x03, &b, 1); //IRQ for send isnt used, no need to clear IRQ when sending
    //SI44_Read(0x04, &b, 1);
    SI44_Write(0x3e, &length, 1);
    SI44_Write(SI44_REG_FIFO_ACCESS, buf, length);
    SI44_ResendPacket(); //Musí být samostatně aby se znovu zapnulo a vypnulo NSS - jinak nepoběží
}

void SI44_ResendPacket(void)
{
    uint8_t txon = SI44_OPERATION_MODE_DEFAULT | 0b00001000; //ToDo: Pozor aby se to nepremazlo s vysilanim
    SI44_Write(SI44_REG_CTRL1, &txon, 1);
}

void SI44_ClearTXFIFO(void)
{
    uint8_t a = 0b00000001;
    SI44_Write(SI44_REG_CTRL2, &a, 1);
    a = 0b00000000;
    SI44_Write(SI44_REG_CTRL2, &a, 1);
}

/*uint8_t SI44_ReadBatteryVoltage(void) //from former library
{
    uint8_t value;
    SI44_Read(SI44_REG_BATTERY_VOLTAGE, &value, 1);
    return value;
}*/

/*uint8_t SI44_ReadTemperature(void) //from former library
{
    uint8_t t = 0;
    SI44_Write(SI44_REG_ADC_CONFIG, &t, 1);
    t = 0b00100000;
    SI44_Write(SI44_REG_TEMP_CONFIG, &t, 1);
    t = 0b10000000;
    SI44_Write(SI44_REG_ADC_CONFIG, &t, 1);
    HAL_Delay(2);
    SI44_Read(SI44_REG_ADC_VALUE, &t, 1);
    return t;
}*/

//--------------RX_NEW--------------- //ToDo: try greater SPI clock
void SI44_PresetConfig(void)
{
	for(uint16_t i = 0; i<SI4432_CONFIG_LEN;i++)
	{
		SI44_Write(address[i], &byte[i], 1);
		HAL_Delay(1);
	}
}

void SI44_SetRxBandwidth(uint8_t bw_reg)
{
    SI44_Write(SI44_REG_RX_BW, &bw_reg, 1);  // RX_BW register
}

void SI44_SetAGCMode(uint8_t agc_sett)
{
    SI44_Write(SI44_AGC_OVERRIDE_1, &agc_sett, 1);
}

void SI44_SetInterrupts1(uint8_t it1)
{
    SI44_Write(SI44_INTERRUPT_ENABLE_1, &it1, 1);
}

void SI44_SetInterrupts2(uint8_t it2)
{
    SI44_Write(SI44_INTERRUPT_ENABLE_2, &it2, 1);
}

void SI44_ClearRXFIFO(void)
{
    uint8_t a = 0b00000010;
    SI44_Write(SI44_REG_CTRL2, &a, 1);
    a = 0b00000000; //can be set directly, we dont use any of these settings
    SI44_Write(SI44_REG_CTRL2, &a, 1);
}

void SI44_ReadPacket(uint8_t * buf)
{
	uint8_t length=0;
	SI44_Read(SI44_RECEIVED_PACKET_LENGTH, &length, 1); //ToDo: Pokud length neni vetsi jak 64
	SI44_Read(SI44_REG_FIFO_ACCESS, buf, length);

	SI44_ClearRXFIFO();

	SI44_SetRXon(); //ToDo: const static?
    //přečíst délku
	//přečíst paket
	//znovu zapnout RX //ToDo: optimalizovat pro spotřebu baterie
	//Z registru se cte tak, ze po docteni jednoho prekroci automaticky na dalsi adresu
}

void SI44_SetRXon(void)
{
	uint8_t rxon = SI44_OPERATION_MODE_DEFAULT | 0b00000100; //ToDo: const static?
	SI44_Write(SI44_REG_CTRL1, &rxon, 1);
}

void SI44_ResetIRQ(void)
{
	uint8_t b; //jen pro reset 0x03 registru
	SI44_Read(0x03, &b, 1); //jinak by uz neaktivoval IRQ
	SI44_Read(0x04, &b, 1);
}

////Dont use - not working
void SI44_SendPacket_IT(uint8_t * buf, uint8_t length)
{
    //uint8_t b;
    SI44_ClearTXFIFO();
    //SI44_Read(0x03, &b, 1); //IRQ for send isnt used, no need to clear IRQ when sending
    //SI44_Read(0x04, &b, 1);
    SI44_Write_IT(0x3e, &length, 1);
    SI44_Write_IT(SI44_REG_FIFO_ACCESS, buf, length);
}

////Dont use - not working
void SI44_ReadPacket_IT(uint8_t * buf)
{
	static uint8_t length=0;
	SI44_Read(SI44_RECEIVED_PACKET_LENGTH, &length, 1); //ToDo: Pokud length neni vetsi jak 64
	//receive=0; ToDo: Replace by states
	SI44_Read_IT(SI44_REG_FIFO_ACCESS, buf, length);
}

//Dont use
void SI44_PacketFetchedFromFIFO_FinishIRQ(void)
{
	  SI44_NSS_Set();
	  SI44_ClearRXFIFO();
	  SI44_SetRXon(); //ToDo: const static? //ToDo: optimalizovat pro spotřebu baterie

}
//Dont use
void SI44_PacketSentToFIFO_FinishIRQ(void)
{
	  SI44_NSS_Set(); //ToDo: Optimalizovat ->
	  SI44_ResendPacket();
}

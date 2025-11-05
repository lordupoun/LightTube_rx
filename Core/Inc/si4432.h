#ifndef SI4432
#define SI4432

#include <si4432_constants.h>
#include "si4432_io_hal.h" //imports HAL types

uint8_t SI44_ReadStatus(void);
void SI44_Reset(void);
void SI44_Init(SPI_HandleTypeDef * hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void SI44_SetTXPower(SI44_TX_POWER power);
void SI44_SendPacket(uint8_t * buffer, uint8_t length);
void SI44_ResendPacket(void);
//uint8_t SI44_ReadBatteryVoltage(void);
//uint8_t SI44_ReadTemperature(void);
void SI44_SetSyncBytes(uint8_t * bytes, uint8_t length);
void SI44_ForceRecalibrate(void);
//-------------------
void SI44_PresetConfig(void);
void SI44_SetRxBandwidth(uint8_t bw_reg);
void SI44_SetAGCMode(uint8_t agc_sett);
void SI44_SetInterrupts1(uint8_t it1);
void SI44_SetInterrupts2(uint8_t it2);
void SI44_ReadPacket(uint8_t * buf);
void SI44_SetRXon(void);
void SI44_ResetIRQ(void);
void SI44_Read_IT(uint8_t reg, uint8_t * buf, uint8_t length);
void SI44_ReadPacket_IT(uint8_t * buf);
void SI44_PacketFetchedFromFIFO_FinishIRQ(void);
void SI44_PacketSentToFIFO_FinishIRQ(void);

#endif

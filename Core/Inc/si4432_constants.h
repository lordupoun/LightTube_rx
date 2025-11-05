#ifndef SI4432_CONSTANTS
#define SI4432_CONSTANTS

typedef enum {
    SI44_TX_POWER_1dBm   = 0b00000000,
    SI44_TX_POWER_2dBm   = 0b00000001,
    SI44_TX_POWER_5dBm   = 0b00000010,
    SI44_TX_POWER_8dBm   = 0b00000011,
	SI44_TX_POWER_11dBm	 = 0b00000100,
	SI44_TX_POWER_14dBm	 = 0b00000101,
	SI44_TX_POWER_17dBm	 = 0b00000110,
	SI44_TX_POWER_20dBm	 = 0b00000111,

} SI44_TX_POWER;

//READY mode
#define SI44_OPERATION_MODE_DEFAULT     0b00000001 //1bit - Tune mode; 0bit - READY mode

//Force recalibration
//#define SI44_FORCE_RACALIBRATION_VAL    0b00011110

//Registers

#define SI44_REG_STATUS 0x02
#define SI44_REG_CTRL1  0x07
#define SI44_REG_CTRL2  0x08

#define SI44_REG_CONF1  0x70
#define SI44_REG_CONF2  0x71

#define SI44_REG_DATA_ACCESS_CONTROL    0x30
#define SI44_REG_HEADER_CONTROL         0x33
#define SI44_REG_PREAMBULE_LENGTH       0x34

#define SI44_REG_TX_POWER               0x6d

#define SI44_REG_FIFO_ACCESS            0x7f

#define SI44_REG_BATTERY_VOLTAGE        0x1b
#define SI44_REG_ADC_CONFIG             0x0f
#define SI44_REG_ADC_VALUE              0x11
#define SI44_REG_TEMP_CONFIG            0x12

//#define SI44_REG_SYNC3                  0x36

#define SI44_REG_CALIBRATION            0x55
//-----------------RX-NEW--------------------
#define SI44_REG_RX_BW					0x1C

#define SI44_AGC_OVERRIDE_1			  	0x69

#define SI44_INTERRUPT_ENABLE_1			0x05
#define SI44_INTERRUPT_ENABLE_2			0x06

#define SI44_RECEIVED_PACKET_LENGTH		0x4B


#endif

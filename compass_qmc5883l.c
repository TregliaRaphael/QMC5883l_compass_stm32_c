#include "compass_qmc5883l.h"

static float xMax = 0;
static float xMin = 0;
static float yMax = 0;
static float yMin = 0;


bool qmc5883lInit(I2C_HandleTypeDef *hi2c)
{

    bool ack = true;
    uint8_t trash = 0x01;
    uint8_t ini = QMC5883L_MODE_CONTINUOUS | QMC5883L_ODR_200HZ | QMC5883L_OSR_512 | QMC5883L_RNG_8G;
    ack = ack && !HAL_I2C_Mem_Write(hi2c, QMC5883L_MAG_I2C_ADDRESS, 0x0B, I2C_MEMADD_SIZE_8BIT, &trash, 1, TM_GLOB);
    ack = ack && !HAL_I2C_Mem_Write(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_CONF1, I2C_MEMADD_SIZE_8BIT, &ini, 1, TM_GLOB);

    if (!ack) {
        return false;
    }

    return true;
}

bool qmc5883lRead(I2C_HandleTypeDef *hi2c, int16_t magData[3])
{
    uint8_t status;
    uint8_t buf[6];

    // set magData to zero for case of failed read
    magData[0] = 0;
    magData[1] = 0;
    magData[2] = 0;

    bool ack = true;
    ack = ack && !HAL_I2C_Mem_Read(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_STATUS, I2C_MEMADD_SIZE_8BIT, &status, 1, TM_GLOB);

    if (!ack || (status & 0x04) == 0) {
        return false;
    }

    ack = ack && !HAL_I2C_Mem_Read(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_DATA_OUTPUT_X, I2C_MEMADD_SIZE_8BIT, buf, 6, TM_GLOB);
    if (!ack) {
        return false;
    }

    magData[0] = (int16_t)(buf[1] << 8 | buf[0]);
    magData[1] = (int16_t)(buf[3] << 8 | buf[2]);
    magData[2] = (int16_t)(buf[5] << 8 | buf[4]);

    return true;
}

bool qmc5883lDetect(I2C_HandleTypeDef *hi2c)
{

    // Must write reset first  - don't care about the result
    //busWriteRegister(busdev, QMC5883L_REG_CONF2, QMC5883L_RST);
    uint8_t reset = QMC5883L_RST;
    HAL_I2C_Mem_Write(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_CONF2, I2C_MEMADD_SIZE_8BIT, &reset, 1, TM_GLOB);
    
    //delay lol
    int tt = 0;
    while (tt < 1000)
	    tt++;

    uint8_t sig = 0;

    //HAL_StatusTypeDef det = HAL_I2C_Mem_Read(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_ID, I2C_MEMADD_SIZE_8BIT, &sig, 1, TM_GLOB);
    bool ack = true;
    ack = ack && !HAL_I2C_Mem_Read(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_ID, I2C_MEMADD_SIZE_8BIT, &sig, 1, TM_GLOB);
    if (ack && sig == QMC5883_ID_VAL) {
        // Should be in standby mode after soft reset and sensor is really present
        // Reading ChipID of 0xFF alone is not sufficient to be sure the QMC is present
        //ack = busReadRegisterBuffer(busdev, QMC5883L_REG_CONF1, &sig, 1);
        ack = ack && !HAL_I2C_Mem_Read(hi2c, QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_CONF1, I2C_MEMADD_SIZE_8BIT, &sig, 1, TM_GLOB);
        if (ack && sig != QMC5883L_MODE_STANDBY) {
            return false;
        }
        //magDev->init = qmc5883lInit;
        //magDev->read = qmc5883lRead;
	//lets do some mess
        return true;
    }

    return false;
}

bool qmc5883lReadHeading(I2C_HandleTypeDef *hi2c, float *heading)
{ 
	int16_t magData[3];
	if (!qmc5883lRead(hi2c, magData))
	  return false;

	float X = 0, Y = 0;
	float xRaw = magData[0];
	float yRaw = magData[1];

  	if (xRaw < xMin)
       	  xMin = xRaw;
    	else if (xRaw > xMax)
	  xMax = xRaw;

  	if (yRaw < yMin)
          yMin = yRaw;
    	else if (yRaw > yMax) 
	  yMax = yRaw; 

  
  	if (xMin == xMax || yMin == yMax) 
	  return false;

 	X -= (xMax + xMin) / 2; 
  	Y -= (yMax + yMin) / 2; 
    
  	X = X / (xMax - xMin); 
  	Y = Y / (yMax - yMin); 

  	*heading = atan2(Y, X);
	//heading = atan2(Y, X) * 180 / M_PI

	//EAST
	*heading += QMC5883L_DECLINATION_ANGLE;
	//WEST
	//heading -= QMC5883L_DECLINATION_ANGLE;
		
	if (*heading < 0)
   	  *heading += 2 * M_PI;
	else if (*heading > 2 * M_PI)
   	  *heading -= 2 * M_PI;
 
 return true; 
} 


uint16_t searchDevice(I2C_HandleTypeDef *hi2c)
{
  HAL_StatusTypeDef result;
  uint8_t i;
  for (i=1; i<128; i++)
  {
          result = HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(i<<1), 2, 2);
          if (result == HAL_OK)
		return i;
  }
  return 0;
}

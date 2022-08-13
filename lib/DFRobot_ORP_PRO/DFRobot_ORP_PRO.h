/*!
  * @file DFRobot_ORP_PRO.h
  * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
  * @licence     The MIT License (MIT)
  * @author      PengKaixing(kaixing.peng@dfrobot.com)
  * @version     V0.1
  * @date        2021-05-17
  * @get         from https://www.dfrobot.com
  * @url         https://github.com/dfrobot/DFRobot_ORP_PRO
  */
#ifndef __DFRobot_ORP_PRO_H__
#define __DFRobot_ORP_PRO_H__
#include "Arduino.h"

#define BASE_VOLTAGE 2480

// Open this macro to see the program running in detail
//#define ENABLE_DBG

#ifdef ENABLE_DBG
#define DBG(...)                      \
  {                                   \
    Serial.print("[");                \
    Serial.print(__FUNCTION__);       \
    Serial.print("(): ");             \
    Serial.print(__LINE__);           \
    Serial.print(" ] 0x");            \
    Serial.println(__VA_ARGS__, HEX); \
  }
#else
#define DBG(...)
#endif

class DFRobot_ORP_PRO
{
  public:
    /*
     * @brief不提供校准值的初始化，默认参考电压为 +2480mV
     * @param NULL
     * @return 没有返回值
     */
    DFRobot_ORP_PRO(/* args */);
    /*
     * @brief:初始化，calibrate为校准偏移值，即实际参考电压=2480mV+calibration
     * 例：当 calibration = -10 时，参考电压为 +2470mV
     */
    DFRobot_ORP_PRO(int calibration);

    /*
     * @brief设置校准值
     * @param NULL
     * @return 没有返回值
     */
    float setCalibration(float voltage);

    /*
     * @brief获取当前校准值
     * @param NULL
     * @return 没有返回值
     */
    float getCalibration();

    /*
     * @brief根据已设置的参考偏置电压，返回计算的ORP值
     * 注意，输入与输出均为mV
     * @param NULL
     * @return 没有返回值
     */
     float getORP(float voltage);

    /*
     * @brief短接 ORP 输入即 0mV 时执行校准
     * @返回值为相对 +2480mV 的偏差值
     * @例：短接后输入 voltage = 2450mV, 则返回值为 -30
     * @param NULL
     * @return可以直接填入 setCalibration
     */
    float calibrate(float voltage);
  private:
    int _ref_voltage = BASE_VOLTAGE/*mV*/;
    int _calibration;
};
#endif
#include "DFRobot_ORP_PRO.h"

DFRobot_ORP_PRO::DFRobot_ORP_PRO(int calibration)
{
  this->_ref_voltage += calibration;
}

float DFRobot_ORP_PRO::setCalibration(float voltage)
{
  this->_calibration = voltage;
}

float DFRobot_ORP_PRO::getCalibration()
{
  return this->_calibration;
}

float DFRobot_ORP_PRO::getORP(float voltage)
{
  return (voltage - (this->_ref_voltage + this->_calibration));
}

float DFRobot_ORP_PRO::calibrate(float voltage)
{
  return (voltage - BASE_VOLTAGE);
}
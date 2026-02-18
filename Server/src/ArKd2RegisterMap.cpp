#include <ArKd2RegisterMap.hpp>

MotorRegisterMap makeArKd2RegisterMap() {
  MotorRegisterMap map;

  // 0x007D: driver input command (16-bit)
  map.driverInputCommandLower = 0x007D;
  map.driverOutputCommandLower = 0x007F;

  // 0x0080/0x0081: present alarm (32-bit)
  map.presentAlarm = 0x0080;
  map.presentWarning = 0x0096;
  map.communicationErrorCode = 0x00AC;
  map.directIoAndBrakeStatus = 0x00D4;

  // 0x0180/0x0181: alarm reset command (used by existing lab script)
  map.alarmResetCommand = 0x0180;

  // Monitor registers (32-bit)
  map.commandPosition = 0x00C6;
  map.commandSpeed = 0x00C8;
  map.actualPosition = 0x00CC;
  map.actualSpeed = 0x00CE;
  map.runCurrent = 0x0240;
  map.stopCurrent = 0x0242;

  // Operation data No.0 registers (32-bit)
  map.positionNo0 = 0x0400;
  map.speedNo0 = 0x0480;
  map.operationModeNo0 = 0x0500;
  map.accelerationNo0 = 0x0600;
  map.decelerationNo0 = 0x0680;

  return map;
}

#pragma once

struct MotorRegisterMap {
  // 16-bit control/status
  int driverInputCommandLower{0x007d};
  int driverOutputCommandLower{0x007f};

  // 32-bit (2x16) registers, upper word first
  int presentAlarm{0x0080};
  int presentWarning{0x0096};
  int communicationErrorCode{0x00AC};
  int directIoAndBrakeStatus{0x00D4};

  // From lab validation script (vendor docs should be preferred for production reset flow).
  int alarmResetCommand{0x0180};

  int commandPosition{0x00C6};
  int commandSpeed{0x00C8};
  int actualPosition{0x00CC};
  int actualSpeed{0x00CE};

  // Operation data No.0 base addresses (each entry is 32-bit, upper/lower).
  int positionNo0{0x0400};
  int speedNo0{0x0480};
  int operationModeNo0{0x0500};
  int accelerationNo0{0x0600};
  int decelerationNo0{0x0680};
};

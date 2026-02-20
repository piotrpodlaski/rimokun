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
  // 32-bit entries (upper/lower words), function code in lower word.
  int outputFunctionSelectBase{0x1140};     // OUT0..OUT5 direct output assignment
  int inputFunctionSelectBase{0x1100};      // IN0..IN7 direct input assignment
  int netInputFunctionSelectBase{0x1160};   // NET-IN0..NET-IN15 assignment
  int netOutputFunctionSelectBase{0x1180};  // NET-OUT0..NET-OUT15 assignment

  // From lab validation script (vendor docs should be preferred for production reset flow).
  int alarmResetCommand{0x0180};

  int commandPosition{0x00C6};
  int commandSpeed{0x00C8};
  int actualPosition{0x00CC};
  int actualSpeed{0x00CE};
  int runCurrent{0x0240};
  int stopCurrent{0x0242};

  // Operation data No.0 base addresses (each entry is 32-bit, upper/lower).
  int positionNo0{0x0400};
  int speedNo0{0x0480};
  int operationModeNo0{0x0500};
  int accelerationNo0{0x0600};
  int decelerationNo0{0x0680};
};

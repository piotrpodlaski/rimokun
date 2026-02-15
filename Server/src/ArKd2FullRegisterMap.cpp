#include <ArKd2FullRegisterMap.hpp>
#include <array>

namespace {
constexpr std::array<ArKd2RegisterEntry, 372> kRegisters{{
    ArKd2RegisterEntry{0x0030, "Group (upper)"}, // 48
    ArKd2RegisterEntry{0x0031, "Group (lower)"}, // 49
    ArKd2RegisterEntry{0x007C, "Driver input command (upper)"}, // 124
    ArKd2RegisterEntry{0x007D, "Driver input command (lower)"}, // 125
    ArKd2RegisterEntry{0x007E, "Driver output command (upper)"}, // 126
    ArKd2RegisterEntry{0x007F, "Driver output command (lower)"}, // 127
    ArKd2RegisterEntry{0x0080, "Present alarm (upper)"}, // 128
    ArKd2RegisterEntry{0x0081, "Present alarm (lower)"}, // 129
    ArKd2RegisterEntry{0x0082, "Alarm record 1 (upper)"}, // 130
    ArKd2RegisterEntry{0x0083, "Alarm record 1 (lower)"}, // 131
    ArKd2RegisterEntry{0x0084, "Alarm record 2 (upper)"}, // 132
    ArKd2RegisterEntry{0x0085, "Alarm record 2 (lower)"}, // 133
    ArKd2RegisterEntry{0x0086, "Alarm record 3 (upper)"}, // 134
    ArKd2RegisterEntry{0x0087, "Alarm record 3 (lower)"}, // 135
    ArKd2RegisterEntry{0x0088, "Alarm record 4 (upper)"}, // 136
    ArKd2RegisterEntry{0x0089, "Alarm record 4 (lower)"}, // 137
    ArKd2RegisterEntry{0x008A, "Alarm record 5 (upper)"}, // 138
    ArKd2RegisterEntry{0x008B, "Alarm record 5 (lower)"}, // 139
    ArKd2RegisterEntry{0x008C, "Alarm record 6 (upper)"}, // 140
    ArKd2RegisterEntry{0x008D, "Alarm record 6 (lower)"}, // 141
    ArKd2RegisterEntry{0x008E, "Alarm record 7 (upper)"}, // 142
    ArKd2RegisterEntry{0x008F, "Alarm record 7 (lower)"}, // 143
    ArKd2RegisterEntry{0x0090, "Alarm record 8 (upper)"}, // 144
    ArKd2RegisterEntry{0x0091, "Alarm record 8 (lower)"}, // 145
    ArKd2RegisterEntry{0x0092, "Alarm record 9 (upper)"}, // 146
    ArKd2RegisterEntry{0x0093, "Alarm record 9 (lower)"}, // 147
    ArKd2RegisterEntry{0x0094, "Alarm record 10 (upper)"}, // 148
    ArKd2RegisterEntry{0x0095, "Alarm record 10 (lower)"}, // 149
    ArKd2RegisterEntry{0x0096, "Present warning (upper)"}, // 150
    ArKd2RegisterEntry{0x0097, "Present warning (lower)"}, // 151
    ArKd2RegisterEntry{0x0098, "Warning record 1 (upper)"}, // 152
    ArKd2RegisterEntry{0x0099, "Warning record 1 (lower)"}, // 153
    ArKd2RegisterEntry{0x009A, "Warning record 2 (upper)"}, // 154
    ArKd2RegisterEntry{0x009B, "Warning record 2 (lower)"}, // 155
    ArKd2RegisterEntry{0x009C, "Warning record 3 (upper)"}, // 156
    ArKd2RegisterEntry{0x009D, "Warning record 3 (lower)"}, // 157
    ArKd2RegisterEntry{0x009E, "Warning record 4 (upper)"}, // 158
    ArKd2RegisterEntry{0x009F, "Warning record 4 (lower)"}, // 159
    ArKd2RegisterEntry{0x00A0, "Warning record 5 (upper)"}, // 160
    ArKd2RegisterEntry{0x00A1, "Warning record 5 (lower)"}, // 161
    ArKd2RegisterEntry{0x00A2, "Warning record 6 (upper)"}, // 162
    ArKd2RegisterEntry{0x00A3, "Warning record 6 (lower)"}, // 163
    ArKd2RegisterEntry{0x00A4, "Warning record 7 (upper)"}, // 164
    ArKd2RegisterEntry{0x00A5, "Warning record 7 (lower)"}, // 165
    ArKd2RegisterEntry{0x00A6, "Warning record 8 (upper)"}, // 166
    ArKd2RegisterEntry{0x00A7, "Warning record 8 (lower)"}, // 167
    ArKd2RegisterEntry{0x00A8, "Warning record 9 (upper)"}, // 168
    ArKd2RegisterEntry{0x00A9, "Warning record 9 (lower)"}, // 169
    ArKd2RegisterEntry{0x00AA, "Warning record 10 (upper)"}, // 170
    ArKd2RegisterEntry{0x00AB, "Warning record 10 (lower)"}, // 171
    ArKd2RegisterEntry{0x00AC, "Communication error code (upper)"}, // 172
    ArKd2RegisterEntry{0x00AD, "Communication error code (lower)"}, // 173
    ArKd2RegisterEntry{0x00AE, "Communication error code record 1 (upper)"}, // 174
    ArKd2RegisterEntry{0x00AF, "Communication error code record 1 (lower)"}, // 175
    ArKd2RegisterEntry{0x00B0, "Communication error code record 2 (upper)"}, // 176
    ArKd2RegisterEntry{0x00B1, "Communication error code record 2 (lower)"}, // 177
    ArKd2RegisterEntry{0x00B2, "Communication error code record 3 (upper)"}, // 178
    ArKd2RegisterEntry{0x00B3, "Communication error code record 3 (lower)"}, // 179
    ArKd2RegisterEntry{0x00B4, "Communication error code record 4 (upper)"}, // 180
    ArKd2RegisterEntry{0x00B5, "Communication error code record 4 (lower)"}, // 181
    ArKd2RegisterEntry{0x00B6, "Communication error code record 5 (upper)"}, // 182
    ArKd2RegisterEntry{0x00B7, "Communication error code record 5 (lower)"}, // 183
    ArKd2RegisterEntry{0x00B8, "Communication error code record 6 (upper)"}, // 184
    ArKd2RegisterEntry{0x00B9, "Communication error code record 6 (lower)"}, // 185
    ArKd2RegisterEntry{0x00BA, "Communication error code record 7 (upper)"}, // 186
    ArKd2RegisterEntry{0x00BB, "Communication error code record 7 (lower)"}, // 187
    ArKd2RegisterEntry{0x00BC, "Communication error code record 8 (upper)"}, // 188
    ArKd2RegisterEntry{0x00BD, "Communication error code record 8 (lower)"}, // 189
    ArKd2RegisterEntry{0x00BE, "Communication error code record 9 (upper)"}, // 190
    ArKd2RegisterEntry{0x00BF, "Communication error code record 9 (lower)"}, // 191
    ArKd2RegisterEntry{0x00C0, "Communication error code record 10 (upper)"}, // 192
    ArKd2RegisterEntry{0x00C1, "Communication error code record 10 (lower)"}, // 193
    ArKd2RegisterEntry{0x00C2, "Present selected data No. (upper)"}, // 194
    ArKd2RegisterEntry{0x00C3, "Present selected data No. (lower)"}, // 195
    ArKd2RegisterEntry{0x00C4, "Present operation data No. (upper)"}, // 196
    ArKd2RegisterEntry{0x00C5, "Present operation data No. (lower)"}, // 197
    ArKd2RegisterEntry{0x00C6, "Command position (upper)"}, // 198
    ArKd2RegisterEntry{0x00C7, "Command position (lower)"}, // 199
    ArKd2RegisterEntry{0x00C8, "Command speed (upper)"}, // 200
    ArKd2RegisterEntry{0x00C9, "Command speed (lower)"}, // 201
    ArKd2RegisterEntry{0x00CC, "Actual position (upper)"}, // 204
    ArKd2RegisterEntry{0x00CD, "Actual position (lower)"}, // 205
    ArKd2RegisterEntry{0x00CE, "Actual speed (upper)"}, // 206
    ArKd2RegisterEntry{0x00CF, "Actual speed (lower)"}, // 207
    ArKd2RegisterEntry{0x00D2, "Remaining dwell time (upper)"}, // 210
    ArKd2RegisterEntry{0x00D3, "Remaining dwell time (lower)"}, // 211
    ArKd2RegisterEntry{0x00D4, "Direct I/O and electromagnetic brake status (upper)"}, // 212
    ArKd2RegisterEntry{0x00D5, "Direct I/O and electromagnetic brake status (lower)"}, // 213
    ArKd2RegisterEntry{0x0180, "Reset alarm (upper)"}, // 384
    ArKd2RegisterEntry{0x0181, "Reset alarm (lower)"}, // 385
    ArKd2RegisterEntry{0x0184, "Clear alarm records (upper)"}, // 388
    ArKd2RegisterEntry{0x0185, "Clear alarm records (lower)"}, // 389
    ArKd2RegisterEntry{0x0186, "Clear warning records (upper)"}, // 390
    ArKd2RegisterEntry{0x0187, "Clear warning records (lower)"}, // 391
    ArKd2RegisterEntry{0x0188, "Clear communication error records (upper)"}, // 392
    ArKd2RegisterEntry{0x0189, "Clear communication error records (lower)"}, // 393
    ArKd2RegisterEntry{0x018A, "P-PRESET execute (upper)"}, // 394
    ArKd2RegisterEntry{0x018B, "P-PRESET execute (lower)"}, // 395
    ArKd2RegisterEntry{0x018C, "Configuration (upper)"}, // 396
    ArKd2RegisterEntry{0x018D, "Configuration (lower)"}, // 397
    ArKd2RegisterEntry{0x018E, "All data initialization (upper) * Resets the operation data and parameters saved in the non-volatile memory, to their defaults."}, // 398
    ArKd2RegisterEntry{0x018F, "All data initialization (lower) *"}, // 399
    ArKd2RegisterEntry{0x0190, "Batch non-volatile memory read (upper)"}, // 400
    ArKd2RegisterEntry{0x0191, "Batch non-volatile memory read (lower)"}, // 401
    ArKd2RegisterEntry{0x0192, "Batch non-volatile memory write (upper)"}, // 402
    ArKd2RegisterEntry{0x0193, "Batch non-volatile memory write (lower)"}, // 403
    ArKd2RegisterEntry{0x0200, "STOP input action (upper)"}, // 512
    ArKd2RegisterEntry{0x0201, "STOP input action (lower)"}, // 513
    ArKd2RegisterEntry{0x0202, "Hardware overtravel (upper)"}, // 514
    ArKd2RegisterEntry{0x0203, "Hardware overtravel (lower)"}, // 515
    ArKd2RegisterEntry{0x0204, "Overtravel action (upper)"}, // 516
    ArKd2RegisterEntry{0x0205, "Overtravel action (lower)"}, // 517
    ArKd2RegisterEntry{0x0206, "END signal range (upper)"}, // 518
    ArKd2RegisterEntry{0x0207, "END signal range (lower)"}, // 519
    ArKd2RegisterEntry{0x0208, "Positioning complete output offset (upper)"}, // 520
    ArKd2RegisterEntry{0x0209, "Positioning complete output offset (lower)"}, // 521
    ArKd2RegisterEntry{0x020A, "AREA1 positive direction position (upper)"}, // 522
    ArKd2RegisterEntry{0x020B, "AREA1 positive direction position (lower)"}, // 523
    ArKd2RegisterEntry{0x020C, "AREA1 negative direction position (upper)"}, // 524
    ArKd2RegisterEntry{0x020D, "AREA1 negative direction position (lower)"}, // 525
    ArKd2RegisterEntry{0x020E, "AREA2 positive direction position (upper)"}, // 526
    ArKd2RegisterEntry{0x020F, "AREA2 positive direction position (lower)"}, // 527
    ArKd2RegisterEntry{0x0210, "AREA2 negative direction position (upper)"}, // 528
    ArKd2RegisterEntry{0x0211, "AREA2 negative direction position (lower)"}, // 529
    ArKd2RegisterEntry{0x0212, "AREA3 positive direction position (upper)"}, // 530
    ArKd2RegisterEntry{0x0213, "AREA3 positive direction position (lower)"}, // 531
    ArKd2RegisterEntry{0x0214, "AREA3 negative direction position (upper)"}, // 532
    ArKd2RegisterEntry{0x0215, "AREA3 negative direction position (lower)"}, // 533
    ArKd2RegisterEntry{0x0216, "Minimum ON time for MOVE output (upper)"}, // 534
    ArKd2RegisterEntry{0x0217, "Minimum ON time for MOVE output (lower)"}, // 535
    ArKd2RegisterEntry{0x0218, "LS contact configration (upper)"}, // 536
    ArKd2RegisterEntry{0x0219, "LS contact configration (lower)"}, // 537
    ArKd2RegisterEntry{0x021A, "HOMES logic level (upper)"}, // 538
    ArKd2RegisterEntry{0x021B, "HOMES logic level (lower)"}, // 539
    ArKd2RegisterEntry{0x021C, "SLIT logic level (upper)"}, // 540
    ArKd2RegisterEntry{0x021D, "SLIT logic level (lower)"}, // 541
    ArKd2RegisterEntry{0x0240, "RUN current (upper)"}, // 576
    ArKd2RegisterEntry{0x0241, "RUN current (lower)"}, // 577
    ArKd2RegisterEntry{0x0242, "STOP current (upper)"}, // 578
    ArKd2RegisterEntry{0x0243, "STOP current (lower)"}, // 579
    ArKd2RegisterEntry{0x0244, "Position loop gain (upper)"}, // 580
    ArKd2RegisterEntry{0x0245, "Position loop gain (lower)"}, // 581
    ArKd2RegisterEntry{0x0246, "Speed loop gain (upper)"}, // 582
    ArKd2RegisterEntry{0x0247, "Speed loop gain (lower)"}, // 583
    ArKd2RegisterEntry{0x0248, "Speed loop integral time constant (upper)"}, // 584
    ArKd2RegisterEntry{0x0249, "Speed loop integral time constant (lower)"}, // 585
    ArKd2RegisterEntry{0x024A, "Speed filter (upper)"}, // 586
    ArKd2RegisterEntry{0x024B, "Speed filter (lower)"}, // 587
    ArKd2RegisterEntry{0x024C, "Moving average time (upper)"}, // 588
    ArKd2RegisterEntry{0x024D, "Moving average time (lower)"}, // 589
    ArKd2RegisterEntry{0x0280, "Common acceleration (upper)"}, // 640
    ArKd2RegisterEntry{0x0281, "Common acceleration (lower)"}, // 641
    ArKd2RegisterEntry{0x0282, "Common deceleration (upper)"}, // 642
    ArKd2RegisterEntry{0x0283, "Common deceleration (lower)"}, // 643
    ArKd2RegisterEntry{0x0284, "Starting speed (upper)"}, // 644
    ArKd2RegisterEntry{0x0285, "Starting speed (lower)"}, // 645
    ArKd2RegisterEntry{0x0286, "JOG operating speed (upper)"}, // 646
    ArKd2RegisterEntry{0x0287, "JOG operating speed (lower)"}, // 647
    ArKd2RegisterEntry{0x0288, "Acceleration/deceleration rate of JOG (upper)"}, // 648
    ArKd2RegisterEntry{0x0289, "Acceleration/deceleration rate of JOG (lower)"}, // 649
    ArKd2RegisterEntry{0x028A, "JOG starting speed (upper)"}, // 650
    ArKd2RegisterEntry{0x028B, "JOG starting speed (lower)"}, // 651
    ArKd2RegisterEntry{0x028C, "Acceleration/deceleration type (upper)"}, // 652
    ArKd2RegisterEntry{0x028D, "Acceleration/deceleration type (lower)"}, // 653
    ArKd2RegisterEntry{0x028E, "Acceleration/deceleration unit (upper)"}, // 654
    ArKd2RegisterEntry{0x028F, "Acceleration/deceleration unit (lower)"}, // 655
    ArKd2RegisterEntry{0x02C0, "Home-seeking mode (upper)"}, // 704
    ArKd2RegisterEntry{0x02C1, "Home-seeking mode (lower)"}, // 705
    ArKd2RegisterEntry{0x02C2, "Operating speed of home- seeking (upper)"}, // 706
    ArKd2RegisterEntry{0x02C3, "Operating speed of home- seeking (lower)"}, // 707
    ArKd2RegisterEntry{0x02C4, "Acceleration/deceleration of home-seeking (upper)"}, // 708
    ArKd2RegisterEntry{0x02C5, "Acceleration/deceleration of home-seeking (lower)"}, // 709
    ArKd2RegisterEntry{0x02C6, "Starting speed of home-seeking (upper)"}, // 710
    ArKd2RegisterEntry{0x02C7, "Starting speed of home-seeking (lower)"}, // 711
    ArKd2RegisterEntry{0x02C8, "Position offset of home-seeking (upper)"}, // 712
    ArKd2RegisterEntry{0x02C9, "Position offset of home-seeking (lower)"}, // 713
    ArKd2RegisterEntry{0x02CA, "Starting direction of home- seeking (upper)"}, // 714
    ArKd2RegisterEntry{0x02CB, "Starting direction of home- seeking (lower)"}, // 715
    ArKd2RegisterEntry{0x02CC, "SLIT detection with home- seeking (upper)"}, // 716
    ArKd2RegisterEntry{0x02CD, "SLIT detection with home- seeking (lower)"}, // 717
    ArKd2RegisterEntry{0x02CE, "TIM signal detection with home- seeking (upper)"}, // 718
    ArKd2RegisterEntry{0x02CF, "TIM signal detection with home- seeking (lower)"}, // 719
    ArKd2RegisterEntry{0x02D0, "Operating current of home- seeking with push-motion (upper)"}, // 720
    ArKd2RegisterEntry{0x02D1, "Operating current of home- seeking with push-motion (lower)"}, // 721
    ArKd2RegisterEntry{0x0300, "Overload alarm (upper)"}, // 768
    ArKd2RegisterEntry{0x0301, "Overload alarm (lower)"}, // 769
    ArKd2RegisterEntry{0x0302, "Excessive position deviation alarm at current ON (upper)"}, // 770
    ArKd2RegisterEntry{0x0303, "Excessive position deviation alarm at current ON (lower)"}, // 771
    ArKd2RegisterEntry{0x0308, "Return-to-home incomplete alarm (upper)"}, // 776
    ArKd2RegisterEntry{0x0309, "Return-to-home incomplete alarm (lower)"}, // 777
    ArKd2RegisterEntry{0x0340, "Overheat warning (upper)"}, // 832
    ArKd2RegisterEntry{0x0341, "Overheat warning (lower)"}, // 833
    ArKd2RegisterEntry{0x0342, "Overload warning (upper)"}, // 834
    ArKd2RegisterEntry{0x0343, "Overload warning (lower)"}, // 835
    ArKd2RegisterEntry{0x0344, "Overspeed warning (upper)"}, // 836
    ArKd2RegisterEntry{0x0345, "Overspeed warning (lower)"}, // 837
    ArKd2RegisterEntry{0x0346, "Overvoltage warning (upper)"}, // 838
    ArKd2RegisterEntry{0x0347, "Overvoltage warning (lower)"}, // 839
    ArKd2RegisterEntry{0x0348, "Undervoltage warning (upper)"}, // 840
    ArKd2RegisterEntry{0x0349, "Undervoltage warning (lower)"}, // 841
    ArKd2RegisterEntry{0x034A, "Excessive position deviation warning at current ON (upper)"}, // 842
    ArKd2RegisterEntry{0x034B, "Excessive position deviation warning at current ON (lower)"}, // 843
    ArKd2RegisterEntry{0x0380, "Electronic gear A (upper)"}, // 896
    ArKd2RegisterEntry{0x0381, "Electronic gear A (lower)"}, // 897
    ArKd2RegisterEntry{0x0382, "Electronic gear B (upper)"}, // 898
    ArKd2RegisterEntry{0x0383, "Electronic gear B (lower)"}, // 899
    ArKd2RegisterEntry{0x0384, "Motor rotation direction (upper)"}, // 900
    ArKd2RegisterEntry{0x0385, "Motor rotation direction (lower)"}, // 901
    ArKd2RegisterEntry{0x0386, "Software overtravel (upper)"}, // 902
    ArKd2RegisterEntry{0x0387, "Software overtravel (lower)"}, // 903
    ArKd2RegisterEntry{0x0388, "Positive software limit (upper)"}, // 904
    ArKd2RegisterEntry{0x0389, "Positive software limit (lower)"}, // 905
    ArKd2RegisterEntry{0x038A, "Negative software limit (upper)"}, // 906
    ArKd2RegisterEntry{0x038B, "Negative software limit (lower)"}, // 907
    ArKd2RegisterEntry{0x038C, "Preset position (upper)"}, // 908
    ArKd2RegisterEntry{0x038D, "Preset position (lower)"}, // 909
    ArKd2RegisterEntry{0x038E, "Wrap setting (upper)"}, // 910
    ArKd2RegisterEntry{0x038F, "Wrap setting (lower)"}, // 911
    ArKd2RegisterEntry{0x0390, "Wrap setting range (upper)"}, // 912
    ArKd2RegisterEntry{0x0391, "Wrap setting range (lower)"}, // 913
    ArKd2RegisterEntry{0x03C0, "Data setter speed display (upper)"}, // 960
    ArKd2RegisterEntry{0x03C1, "Data setter speed display (lower)"}, // 961
    ArKd2RegisterEntry{0x03C2, "Data setter edit (upper)"}, // 962
    ArKd2RegisterEntry{0x03C3, "Data setter edit (lower)"}, // 963
    ArKd2RegisterEntry{0x1000, "MS0 operation No. selection (upper)"}, // 4096
    ArKd2RegisterEntry{0x1001, "MS0 operation No. selection (lower)"}, // 4097
    ArKd2RegisterEntry{0x1002, "MS1 operation No. selection (upper)"}, // 4098
    ArKd2RegisterEntry{0x1003, "MS1 operation No. selection (lower)"}, // 4099
    ArKd2RegisterEntry{0x1004, "MS2 operation No. selection (upper)"}, // 4100
    ArKd2RegisterEntry{0x1005, "MS2 operation No. selection (lower)"}, // 4101
    ArKd2RegisterEntry{0x1006, "MS3 operation No. selection (upper)"}, // 4102
    ArKd2RegisterEntry{0x1007, "MS3 operation No. selection (lower)"}, // 4103
    ArKd2RegisterEntry{0x1008, "MS4 operation No. selection (upper)"}, // 4104
    ArKd2RegisterEntry{0x1009, "MS4 operation No. selection (lower)"}, // 4105
    ArKd2RegisterEntry{0x100A, "MS5 operation No. selection (upper)"}, // 4106
    ArKd2RegisterEntry{0x100B, "MS5 operation No. selection (lower)"}, // 4107
    ArKd2RegisterEntry{0x100C, "HOME-P output function selection (upper)"}, // 4108
    ArKd2RegisterEntry{0x100D, "HOME-P output function selection (lower)"}, // 4109
    ArKd2RegisterEntry{0x1020, "Filter selection (upper)"}, // 4128
    ArKd2RegisterEntry{0x1021, "Filter selection (lower)"}, // 4129
    ArKd2RegisterEntry{0x1022, "Speed difference gain 1 (upper)"}, // 4130
    ArKd2RegisterEntry{0x1023, "Speed difference gain 1 (lower)"}, // 4131
    ArKd2RegisterEntry{0x1024, "Speed difference gain 2 (upper)"}, // 4132
    ArKd2RegisterEntry{0x1025, "Speed difference gain 2 (lower)"}, // 4133
    ArKd2RegisterEntry{0x1026, "Control mode (upper)"}, // 4134
    ArKd2RegisterEntry{0x1027, "Control mode (lower)"}, // 4135
    ArKd2RegisterEntry{0x1028, "Smooth drive (upper)"}, // 4136
    ArKd2RegisterEntry{0x1029, "Smooth drive (lower)"}, // 4137
    ArKd2RegisterEntry{0x1040, "Automatic return operation (upper)"}, // 4160
    ArKd2RegisterEntry{0x1041, "Automatic return operation (lower)"}, // 4161
    ArKd2RegisterEntry{0x1042, "Operation speed of automatic return (upper)"}, // 4162
    ArKd2RegisterEntry{0x1043, "Operation speed of automatic return (lower)"}, // 4163
    ArKd2RegisterEntry{0x1044, "Acceleration (deceleration) of automatic return (upper)"}, // 4164
    ArKd2RegisterEntry{0x1045, "Acceleration (deceleration) of automatic return (lower)"}, // 4165
    ArKd2RegisterEntry{0x1046, "Starting speed of automatic return (upper)"}, // 4166
    ArKd2RegisterEntry{0x1047, "Starting speed of automatic return (lower)"}, // 4167
    ArKd2RegisterEntry{0x1048, "JOG travel amount (upper)"}, // 4168
    ArKd2RegisterEntry{0x1049, "JOG travel amount (lower)"}, // 4169
    ArKd2RegisterEntry{0x1080, "Excessive position deviation alarm at current OFF (upper)"}, // 4224
    ArKd2RegisterEntry{0x1081, "Excessive position deviation alarm at current OFF (lower)"}, // 4225
    ArKd2RegisterEntry{0x1100, "IN0 input function selection (upper)"}, // 4352
    ArKd2RegisterEntry{0x1101, "IN0 input function selection (lower)"}, // 4353
    ArKd2RegisterEntry{0x1102, "IN1 input function selection (upper)"}, // 4354
    ArKd2RegisterEntry{0x1103, "IN1 input function selection (lower)"}, // 4355
    ArKd2RegisterEntry{0x1104, "IN2 input function selection (upper)"}, // 4356
    ArKd2RegisterEntry{0x1105, "IN2 input function selection (lower)"}, // 4357
    ArKd2RegisterEntry{0x1106, "IN3 input function selection (upper)"}, // 4358
    ArKd2RegisterEntry{0x1107, "IN3 input function selection (lower)"}, // 4359
    ArKd2RegisterEntry{0x1108, "IN4 input function selection (upper)"}, // 4360
    ArKd2RegisterEntry{0x1109, "IN4 input function selection (lower)"}, // 4361
    ArKd2RegisterEntry{0x110A, "IN5 input function selection (upper)"}, // 4362
    ArKd2RegisterEntry{0x110B, "IN5 input function selection (lower)"}, // 4363
    ArKd2RegisterEntry{0x110C, "IN6 input function selection (upper)"}, // 4364
    ArKd2RegisterEntry{0x110D, "IN6 input function selection (lower)"}, // 4365
    ArKd2RegisterEntry{0x110E, "IN7 input function selection (upper)"}, // 4366
    ArKd2RegisterEntry{0x110F, "IN7 input function selection (lower)"}, // 4367
    ArKd2RegisterEntry{0x1120, "IN0 input logic level setting (upper)"}, // 4384
    ArKd2RegisterEntry{0x1121, "IN0 input logic level setting (lower)"}, // 4385
    ArKd2RegisterEntry{0x1122, "IN1 input logic level setting (upper)"}, // 4386
    ArKd2RegisterEntry{0x1123, "IN1 input logic level setting (lower)"}, // 4387
    ArKd2RegisterEntry{0x1124, "IN2 input logic level setting (upper)"}, // 4388
    ArKd2RegisterEntry{0x1125, "IN2 input logic level setting (lower)"}, // 4389
    ArKd2RegisterEntry{0x1126, "IN3 input logic level setting (upper)"}, // 4390
    ArKd2RegisterEntry{0x1127, "IN3 input logic level setting (lower)"}, // 4391
    ArKd2RegisterEntry{0x1128, "IN4 input logic level setting (upper)"}, // 4392
    ArKd2RegisterEntry{0x1129, "IN4 input logic level setting (lower)"}, // 4393
    ArKd2RegisterEntry{0x112A, "IN5 input logic level setting (upper)"}, // 4394
    ArKd2RegisterEntry{0x112B, "IN5 input logic level setting (lower)"}, // 4395
    ArKd2RegisterEntry{0x112C, "IN6 input logic level setting (upper)"}, // 4396
    ArKd2RegisterEntry{0x112D, "IN6 input logic level setting (lower)"}, // 4397
    ArKd2RegisterEntry{0x112E, "IN7 input logic level setting (upper)"}, // 4398
    ArKd2RegisterEntry{0x112F, "IN7 input logic level setting (lower)"}, // 4399
    ArKd2RegisterEntry{0x1140, "OUT0 output function selection (upper)"}, // 4416
    ArKd2RegisterEntry{0x1141, "OUT0 output function selection (lower)"}, // 4417
    ArKd2RegisterEntry{0x1142, "OUT1 output function selection (upper)"}, // 4418
    ArKd2RegisterEntry{0x1143, "OUT1 output function selection (lower)"}, // 4419
    ArKd2RegisterEntry{0x1144, "OUT2 output function selection (upper)"}, // 4420
    ArKd2RegisterEntry{0x1145, "OUT2 output function selection (lower)"}, // 4421
    ArKd2RegisterEntry{0x1146, "OUT3 output function selection (upper)"}, // 4422
    ArKd2RegisterEntry{0x1147, "OUT3 output function selection (lower)"}, // 4423
    ArKd2RegisterEntry{0x1148, "OUT4 output function selection (upper)"}, // 4424
    ArKd2RegisterEntry{0x1149, "OUT4 output function selection (lower)"}, // 4425
    ArKd2RegisterEntry{0x114A, "OUT5 output function selection (upper)"}, // 4426
    ArKd2RegisterEntry{0x114B, "OUT5 output function selection (lower)"}, // 4427
    ArKd2RegisterEntry{0x1160, "NET-IN0 input function selection (upper)"}, // 4448
    ArKd2RegisterEntry{0x1161, "NET-IN0 input function selection (lower)"}, // 4449
    ArKd2RegisterEntry{0x1162, "NET-IN1 input function selection (upper)"}, // 4450
    ArKd2RegisterEntry{0x1163, "NET-IN1 input function selection (lower)"}, // 4451
    ArKd2RegisterEntry{0x1164, "NET-IN2 input function selection (upper)"}, // 4452
    ArKd2RegisterEntry{0x1165, "NET-IN2 input function selection (lower)"}, // 4453
    ArKd2RegisterEntry{0x1166, "NET-IN3 input function selection (upper)"}, // 4454
    ArKd2RegisterEntry{0x1167, "NET-IN3 input function selection (lower)"}, // 4455
    ArKd2RegisterEntry{0x1168, "NET-IN4 input function selection (upper)"}, // 4456
    ArKd2RegisterEntry{0x1169, "NET-IN4 input function selection (lower)"}, // 4457
    ArKd2RegisterEntry{0x116A, "NET-IN5 input function selection (upper)"}, // 4458
    ArKd2RegisterEntry{0x116B, "NET-IN5 input function selection (lower)"}, // 4459
    ArKd2RegisterEntry{0x116C, "NET-IN6 input function selection (upper)"}, // 4460
    ArKd2RegisterEntry{0x116D, "NET-IN6 input function selection (lower)"}, // 4461
    ArKd2RegisterEntry{0x116E, "NET-IN7 input function selection (upper)"}, // 4462
    ArKd2RegisterEntry{0x116F, "NET-IN7 input function selection (lower)"}, // 4463
    ArKd2RegisterEntry{0x1170, "NET-IN8 input function selection (upper)"}, // 4464
    ArKd2RegisterEntry{0x1171, "NET-IN8 input function selection (lower)"}, // 4465
    ArKd2RegisterEntry{0x1172, "NET-IN9 input function selection (upper)"}, // 4466
    ArKd2RegisterEntry{0x1173, "NET-IN9 input function selection (lower)"}, // 4467
    ArKd2RegisterEntry{0x1174, "NET-IN10 input function selection (upper)"}, // 4468
    ArKd2RegisterEntry{0x1175, "NET-IN10 input function selection (lower)"}, // 4469
    ArKd2RegisterEntry{0x1176, "NET-IN11 input function selection (upper)"}, // 4470
    ArKd2RegisterEntry{0x1177, "NET-IN11 input function selection (lower)"}, // 4471
    ArKd2RegisterEntry{0x1178, "NET-IN12 input function selection (upper)"}, // 4472
    ArKd2RegisterEntry{0x1179, "NET-IN12 input function selection (lower)"}, // 4473
    ArKd2RegisterEntry{0x117A, "NET-IN13 input function selection (upper)"}, // 4474
    ArKd2RegisterEntry{0x117B, "NET-IN13 input function selection (lower)"}, // 4475
    ArKd2RegisterEntry{0x117C, "NET-IN14 input function selection (upper)"}, // 4476
    ArKd2RegisterEntry{0x117D, "NET-IN14 input function selection (lower)"}, // 4477
    ArKd2RegisterEntry{0x117E, "NET-IN15 input function selection (upper)"}, // 4478
    ArKd2RegisterEntry{0x117F, "NET-IN15 input function selection (lower)"}, // 4479
    ArKd2RegisterEntry{0x1180, "NET-OUT0 output function selection (upper)"}, // 4480
    ArKd2RegisterEntry{0x1181, "NET-OUT0 output function selection (lower)"}, // 4481
    ArKd2RegisterEntry{0x1182, "NET-OUT1 output function selection (upper)"}, // 4482
    ArKd2RegisterEntry{0x1183, "NET-OUT1 output function selection (lower)"}, // 4483
    ArKd2RegisterEntry{0x1184, "NET-OUT2 output function selection (upper)"}, // 4484
    ArKd2RegisterEntry{0x1185, "NET-OUT2 output function selection (lower)"}, // 4485
    ArKd2RegisterEntry{0x1186, "NET-OUT3 output function selection (upper)"}, // 4486
    ArKd2RegisterEntry{0x1187, "NET-OUT3 output function selection (lower)"}, // 4487
    ArKd2RegisterEntry{0x1188, "NET-OUT4 output function selection (upper)"}, // 4488
    ArKd2RegisterEntry{0x1189, "NET-OUT4 output function selection (lower)"}, // 4489
    ArKd2RegisterEntry{0x118A, "NET-OUT5 output function selection (upper)"}, // 4490
    ArKd2RegisterEntry{0x118B, "NET-OUT5 output function selection (lower)"}, // 4491
    ArKd2RegisterEntry{0x118C, "NET-OUT6 output function selection (upper)"}, // 4492
    ArKd2RegisterEntry{0x118D, "NET-OUT6 output function selection (lower)"}, // 4493
    ArKd2RegisterEntry{0x118E, "NET-OUT7 output function selection (upper)"}, // 4494
    ArKd2RegisterEntry{0x118F, "NET-OUT7 output function selection (lower)"}, // 4495
    ArKd2RegisterEntry{0x1190, "NET-OUT8 output function selection (upper)"}, // 4496
    ArKd2RegisterEntry{0x1191, "NET-OUT8 output function selection (lower)"}, // 4497
    ArKd2RegisterEntry{0x1192, "NET-OUT9 output function selection (upper)"}, // 4498
    ArKd2RegisterEntry{0x1193, "NET-OUT9 output function selection (lower)"}, // 4499
    ArKd2RegisterEntry{0x1194, "NET-OUT10 output function selection (upper)"}, // 4500
    ArKd2RegisterEntry{0x1195, "NET-OUT10 output function selection (lower)"}, // 4501
    ArKd2RegisterEntry{0x1196, "NET-OUT11 output function selection (upper)"}, // 4502
    ArKd2RegisterEntry{0x1197, "NET-OUT11 output function selection (lower)"}, // 4503
    ArKd2RegisterEntry{0x1198, "NET-OUT12 output function selection (upper)"}, // 4504
    ArKd2RegisterEntry{0x1199, "NET-OUT12 output function selection (lower)"}, // 4505
    ArKd2RegisterEntry{0x119A, "NET-OUT13 output function selection (upper)"}, // 4506
    ArKd2RegisterEntry{0x119B, "NET-OUT13 output function selection (lower)"}, // 4507
    ArKd2RegisterEntry{0x119C, "NET-OUT14 output function selection (upper)"}, // 4508
    ArKd2RegisterEntry{0x119D, "NET-OUT14 output function selection (lower)"}, // 4509
    ArKd2RegisterEntry{0x119E, "NET-OUT15 output function selection (upper)"}, // 4510
    ArKd2RegisterEntry{0x119F, "NET-OUT15 output function selection (lower)"}, // 4511
    ArKd2RegisterEntry{0x1200, "Communication timeout (upper)"}, // 4608
    ArKd2RegisterEntry{0x1201, "Communication timeout (lower)"}, // 4609
    ArKd2RegisterEntry{0x1202, "Communication error alarm (upper)"}, // 4610
    ArKd2RegisterEntry{0x1203, "Communication error alarm (lower)"}, // 4611
}};
}  // namespace

std::span<const ArKd2RegisterEntry> arKd2FullRegisterMap() {
  return kRegisters;
}

std::optional<std::string_view> arKd2RegisterName(std::uint16_t address) {
  for (const auto& entry : kRegisters) {
    if (entry.address == address) {
      return entry.name;
    }
  }
  return std::nullopt;
}

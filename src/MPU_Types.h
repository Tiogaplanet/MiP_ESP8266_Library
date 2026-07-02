/* Copyright (C) 2026  Samuel Trassare (https://github.com/Tiogaplanet)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Contains all the large enumerations and data structures.
*/
#ifndef MPU_TYPES_H
#define MPU_TYPES_H

#include <stdint.h>

enum MiPGestureRadarMode : uint8_t
{
    MIP_GESTURE_RADAR_DISABLED = 0x00,
    MIP_GESTURE                = 0x02,
    MIP_RADAR                  = 0x04,
};

enum MiPRadar : uint8_t
{
    MIP_RADAR_NONE      = 0x01,
    MIP_RADAR_10CM_30CM = 0x02,
    MIP_RADAR_0CM_10CM  = 0x03,
    MIP_RADAR_INVALID   = 0xFF          // Is set to this value when there are no current radar events.
};

enum MiPGesture : uint8_t
{
    MIP_GESTURE_LEFT               = 0x0A,
    MIP_GESTURE_RIGHT              = 0x0B,
    MIP_GESTURE_CENTER_SWEEP_LEFT  = 0x0C,
    MIP_GESTURE_CENTER_SWEEP_RIGHT = 0x0D,
    MIP_GESTURE_CENTER_HOLD        = 0x0E,
    MIP_GESTURE_FORWARD            = 0x0F,
    MIP_GESTURE_BACKWARD           = 0x10,
    MIP_GESTURE_INVALID            = 0xFF   // Is set to this value when there are no current gesture events.
};

enum MiPHeadLED : uint8_t
{
    MIP_HEAD_LED_OFF        = 0,
    MIP_HEAD_LED_ON         = 1,
    MIP_HEAD_LED_BLINK_SLOW = 2,
    MIP_HEAD_LED_BLINK_FAST = 3,
};

enum MiPDriveDirection : uint8_t
{
    MIP_DRIVE_FORWARD  = 0x00,
    MIP_DRIVE_BACKWARD = 0x01,
};

enum MiPTurnDirection : uint8_t
{
    MIP_TURN_LEFT  = 0x00,
    MIP_TURN_RIGHT = 0x01
};

enum MiPFallDirection : uint8_t
{
    MIP_FALL_ON_BACK   = 0x00,
    MIP_FALL_FACE_DOWN = 0x01
};

enum MiPPosition : uint8_t
{
    MIP_POSITION_ON_BACK                = 0x00,
    MIP_POSITION_FACE_DOWN              = 0x01,
    MIP_POSITION_UPRIGHT                = 0x02,
    MIP_POSITION_PICKED_UP              = 0x03,
    MIP_POSITION_HAND_STAND             = 0x04,
    MIP_POSITION_FACE_DOWN_ON_TRAY      = 0x05,
    MIP_POSITION_ON_BACK_WITH_KICKSTAND = 0x06,
};

enum MiPGetUp : uint8_t
{
    MIP_GETUP_FROM_FRONT  = 0x00,
    MIP_GETUP_FROM_BACK   = 0x01,
    MIP_GETUP_FROM_EITHER = 0x02
};

enum MiPSoundIndex : uint8_t
{
    MIP_SOUND_ONEKHZ_500MS_8K16BIT = 1,
    MIP_SOUND_ACTION_BURPING,
    MIP_SOUND_ACTION_DRINKING,
    MIP_SOUND_ACTION_EATING,
    MIP_SOUND_ACTION_FARTING_SHORT,
    MIP_SOUND_ACTION_OUT_OF_BREATH,
    MIP_SOUND_BOXING_PUNCHCONNECT_1,
    MIP_SOUND_BOXING_PUNCHCONNECT_2,
    MIP_SOUND_BOXING_PUNCHCONNECT_3,
    MIP_SOUND_FREESTYLE_TRACKING_1,
    MIP_SOUND_MIP_1,
    MIP_SOUND_MIP_2,
    MIP_SOUND_MIP_3,
    MIP_SOUND_MIP_APP,
    MIP_SOUND_MIP_AWWW,
    MIP_SOUND_MIP_BIG_SHOT,
    MIP_SOUND_MIP_BLEH,
    MIP_SOUND_MIP_BOOM,
    MIP_SOUND_MIP_BYE,
    MIP_SOUND_MIP_CONVERSE_1,
    MIP_SOUND_MIP_CONVERSE_2,
    MIP_SOUND_MIP_DROP,
    MIP_SOUND_MIP_DUNNO,
    MIP_SOUND_MIP_FALL_OVER_1,
    MIP_SOUND_MIP_FALL_OVER_2,
    MIP_SOUND_MIP_FIGHT,
    MIP_SOUND_MIP_GAME,
    MIP_SOUND_MIP_GLOAT,
    MIP_SOUND_MIP_GO,
    MIP_SOUND_MIP_GOGOGO,
    MIP_SOUND_MIP_GRUNT_1,
    MIP_SOUND_MIP_GRUNT_2,
    MIP_SOUND_MIP_GRUNT_3,
    MIP_SOUND_MIP_HAHA_GOT_IT,
    MIP_SOUND_MIP_HI_CONFIDENT,
    MIP_SOUND_MIP_HI_NOT_SURE,
    MIP_SOUND_MIP_HI_SCARED,
    MIP_SOUND_MIP_HUH,
    MIP_SOUND_MIP_HUMMING_1,
    MIP_SOUND_MIP_HUMMING_2,
    MIP_SOUND_MIP_HURT,
    MIP_SOUND_MIP_HUUURGH,
    MIP_SOUND_MIP_IN_LOVE,
    MIP_SOUND_MIP_IT,
    MIP_SOUND_MIP_JOKE,
    MIP_SOUND_MIP_K,
    MIP_SOUND_MIP_LOOP_1,
    MIP_SOUND_MIP_LOOP_2,
    MIP_SOUND_MIP_LOW_BATTERY,
    MIP_SOUND_MIP_MIPPEE,
    MIP_SOUND_MIP_MORE,
    MIP_SOUND_MIP_MUAH_HA,
    MIP_SOUND_MIP_MUSIC,
    MIP_SOUND_MIP_OBSTACLE,
    MIP_SOUND_MIP_OHOH,
    MIP_SOUND_MIP_OH_YEAH,
    MIP_SOUND_MIP_OOPSIE,
    MIP_SOUND_MIP_OUCH_1,
    MIP_SOUND_MIP_OUCH_2,
    MIP_SOUND_MIP_PLAY,
    MIP_SOUND_MIP_PUSH,
    MIP_SOUND_MIP_RUN,
    MIP_SOUND_MIP_SHAKE,
    MIP_SOUND_MIP_SIGH,
    MIP_SOUND_MIP_SINGING,
    MIP_SOUND_MIP_SNEEZE,
    MIP_SOUND_MIP_SNORE,
    MIP_SOUND_MIP_STACK,
    MIP_SOUND_MIP_SWIPE_1,
    MIP_SOUND_MIP_SWIPE_2,
    MIP_SOUND_MIP_TRICKS,
    MIP_SOUND_MIP_TRIIICK,
    MIP_SOUND_MIP_TRUMPET,
    MIP_SOUND_MIP_WAAAAA,
    MIP_SOUND_MIP_WAKEY,
    MIP_SOUND_MIP_WHEEE,
    MIP_SOUND_MIP_WHISTLING,
    MIP_SOUND_MIP_WHOAH,
    MIP_SOUND_MIP_WOO,
    MIP_SOUND_MIP_YEAH,
    MIP_SOUND_MIP_YEEESSS,
    MIP_SOUND_MIP_YO,
    MIP_SOUND_MIP_YUMMY,
    MIP_SOUND_MOOD_ACTIVATED,
    MIP_SOUND_MOOD_ANGRY,
    MIP_SOUND_MOOD_ANXIOUS,
    MIP_SOUND_MOOD_BORING,
    MIP_SOUND_MOOD_CRANKY,
    MIP_SOUND_MOOD_ENERGETIC,
    MIP_SOUND_MOOD_EXCITED,
    MIP_SOUND_MOOD_GIDDY,
    MIP_SOUND_MOOD_GRUMPY,
    MIP_SOUND_MOOD_HAPPY,
    MIP_SOUND_MOOD_IDEA,
    MIP_SOUND_MOOD_IMPATIENT,
    MIP_SOUND_MOOD_NICE,
    MIP_SOUND_MOOD_SAD,
    MIP_SOUND_MOOD_SHORT,
    MIP_SOUND_MOOD_SLEEPY,
    MIP_SOUND_MOOD_TIRED,
    MIP_SOUND_SOUND_BOOST,
    MIP_SOUND_SOUND_CAGE,
    MIP_SOUND_SOUND_GUNS,
    MIP_SOUND_SOUND_ZINGS,
    MIP_SOUND_SHORT_MUTE_FOR_STOP,
    MIP_SOUND_FREESTYLE_TRACKING_2,
    MIP_SOUND_VOLUME_OFF = 0xF7,
    MIP_SOUND_VOLUME_1   = 0xF8,
    MIP_SOUND_VOLUME_2   = 0xF9,
    MIP_SOUND_VOLUME_3   = 0xFA,
    MIP_SOUND_VOLUME_4   = 0xFB,
    MIP_SOUND_VOLUME_5   = 0xFC,
    MIP_SOUND_VOLUME_6   = 0xFD,
    MIP_SOUND_VOLUME_7   = 0xFE
};

enum MiPVolume : uint8_t
{
    MIP_VOLUME_OFF = 0,
    MIP_VOLUME_1   = 1,
    MIP_VOLUME_2   = 2,
    MIP_VOLUME_3   = 3,
    MIP_VOLUME_4   = 4,
    MIP_VOLUME_5   = 5,
    MIP_VOLUME_6   = 6,
    MIP_VOLUME_7   = 7,
    MIP_VOLUME_DEFAULT = 0xFF
};

enum MiPClapEnabled : uint8_t
{
    MIP_CLAP_DISABLED = 0x00,
    MIP_CLAP_ENABLED  = 0x01,
};

enum MiPGameMode : uint8_t
{
    MIP_APP_MODE      = 0x01,
    MIP_CAGE_MODE     = 0x02,
    MIP_TRACKING_MODE = 0x03,
    MIP_DANCE_MODE    = 0x04,
    MIP_DEFAULT_MODE  = 0x05,
    MIP_STACK_MODE    = 0x06,
    MIP_TRICK_MODE    = 0x07,
    MIP_ROAM_MODE     = 0x08
};

class MiPStatus
{
public:
    MiPStatus() { clear(); }
    void clear()
    {
        battery = 0.0f;
        position = MIP_POSITION_ON_BACK_WITH_KICKSTAND;
    }
    float       battery;
    MiPPosition position;
};

class MiPChestLED
{
public:
    MiPChestLED() { clear(); }
    void clear()
    {
        onTime = 0;
        offTime = 0;
        red = 0;
        green = 0;
        blue = 0;
    }
    uint16_t onTime;
    uint16_t offTime;
    uint8_t  red;
    uint8_t  green;
    uint8_t  blue;
};

class MiPHeadLEDs
{
public:
    MiPHeadLEDs() { clear(); }
    void clear()
    {
        led1 = MIP_HEAD_LED_OFF;
        led2 = MIP_HEAD_LED_OFF;
        led3 = MIP_HEAD_LED_OFF;
        led4 = MIP_HEAD_LED_OFF;
    }
    MiPHeadLED led1;
    MiPHeadLED led2;
    MiPHeadLED led3;
    MiPHeadLED led4;
};

class MiPSoftwareVersion
{
public:
    MiPSoftwareVersion() { clear(); }
    void clear()
    {
        year = 0;
        month = 0;
        day = 0;
        uniqueVersion = 0;
    }
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  uniqueVersion;
};

class MiPHardwareInfo
{
public:
    MiPHardwareInfo() { clear(); }
    void clear()
    {
        voiceChip = 0;
        hardware = 0;
    }
    uint8_t voiceChip;
    uint8_t hardware;
};

class MiPClapSettings
{
public:
    MiPClapSettings() { clear(); }
    void clear()
    {
        enabled = MIP_CLAP_DISABLED;
        delay = 0;
    }
    MiPClapEnabled enabled;
    uint16_t       delay;
};

#endif // MPU_TYPES_H

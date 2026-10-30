/**
 * \addtogroup CiA402appl CiA402 Sample Application
 * @{
 */

/**
\file cia402appl.h
\author EthercatSSC@beckhoff.com
\brief CiA402 specific defines; objects; Axes structure

\version 5.11

<br>Changes to version V5.01:<br>
V5.11 CiA402 1: "append padding byte to ""csv/csp"" process data (basic process data has odd word length), add pack directive"<br>
V5.11 COE1: update invalid end entry in the object dictionaries (error with some compilers)<br>
<br>Changes to version - :<br>
V5.01 : Start file change log
 */
#ifndef _CIA402_H_
#define _CIA402_H_

/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
//#include "objDict_MultiAxis.h"
#include "objDict_SingleAxis.h"

/*-----------------------------------------------------------------------------------------
------
------    Defines and Types
------
-----------------------------------------------------------------------------------------*/
//#define MAX_AXES       (8) /**< \brief Number of supported axes*/

#define MAX_AXES       (1) /**< \brief Number of supported axes*/


/*---------------------------------------------
-    CiA402 generic error option code values
        Note: Not all values are valid for each error option code.
        A detailed description of the option code values are listed in the specification IEC 61800-7-200
        0x605B    : action in state transition 8
        0x605C    : action in state transition 5
-----------------------------------------------*/
#define DISABLE_DRIVE                    0 /**< \brief Disable drive (options: 0x605B; 0x605C; 0x605E)*/
#define SLOW_DOWN_RAMP                   1 /**< \brief Slow down ramp (options: 0x605B; 0x605C; 0x605E)*/
#define QUICKSTOP_RAMP                   2 /**< \brief Quick stop ramp (options: 0x605E)*/
#define STOP_ON_CURRENT_LIMIT            3 /**< \brief Stop on current limit (options: 0x605E)*/
#define STOP_ON_VOLTAGE_LIMIT            4 /**< \brief Stop on voltage limit (options: 0x605E)*/
//5 reserved                          
#define HOMING_MODE                      6 /**< \brief Homing mode*/
#define INTERPOLATION_POSITION_MODE      7 /**< \brief Interpolation Position mode*/
#define CYCLIC_SYNC_POSITION_MODE        8 /**< \brief Cyclic Synchronous Position mode*/
#define CYCLIC_SYNC_VELOCITY_MODE        9 /**< \brief Cyclic Synchronous Velocity mode*/
#define CYCLIC_SYNC_TORQUE_MODE          10/**< \brief Cyclic Synchronous Torque mode*/
//+11 to +127 reserved

/*---------------------------------------------
-    ControlWord Commands Mask (IEC61800_184e)
-----------------------------------------------*/
#define CONTROLWORD_COMMAND_SHUTDOWN_MASK                    0x0087 /**< \brief Shutdown command mask*/
#define CONTROLWORD_COMMAND_SWITCHON_MASK                    0x00C7 /**< \brief Switch on command mask*/
#define CONTROLWORD_COMMAND_SWITCHON_ENABLEOPERATION_MASK    0x008F /**< \brief Switch on & Enable command mask*/
#define CONTROLWORD_COMMAND_DISABLEVOLTAGE_MASK              0x0082 /**< \brief Disable voltage command mask*/
#define CONTROLWORD_COMMAND_QUICKSTOP_MASK                   0x0086 /**< \brief Quickstop command mask*/
#define CONTROLWORD_COMMAND_DISABLEOPERATION_MASK            0x008F /**< \brief Disable operation command mask*/
#define CONTROLWORD_COMMAND_ENABLEOPERATION_MASK             0x008F /**< \brief Enable operation command mask*/
#define CONTROLWORD_COMMAND_FAULTRESET_MASK                  0x0080 /**< \brief Fault reset command mask*/


/*---------------------------------------------
-    ControlWord Commands (IEC61800_184e)
-----------------------------------------------*/
#define CONTROLWORD_COMMAND_SHUTDOWN                         0x0006 /**< \brief Shutdown command*/
#define CONTROLWORD_COMMAND_SWITCHON                         0x0007 /**< \brief Switch on command*/
#define CONTROLWORD_COMMAND_SWITCHON_ENABLEOPERATION         0x000F /**< \brief Switch on & Enable command*/
#define CONTROLWORD_COMMAND_DISABLEVOLTAGE                   0x0000 /**< \brief Disable voltage command*/
#define CONTROLWORD_COMMAND_QUICKSTOP                        0x0002 /**< \brief Quickstop command*/
#define CONTROLWORD_COMMAND_DISABLEOPERATION                 0x0007 /**< \brief Disable operation command*/
#define CONTROLWORD_COMMAND_ENABLEOPERATION                  0x000F /**< \brief Enable operation command*/
#define CONTROLWORD_COMMAND_FAULTRESET                       0x0080 /**< \brief Fault reset command*/


/*---------------------------------------------
-    StatusWord Masks and Flags
-----------------------------------------------*/
#define STATUSWORD_STATE_MASK                                0x006F /**< \brief State mask*/
#define STATUSWORD_VOLTAGE_ENABLED                           0x0010 /**< \brief Indicate high voltage enabled*/
#define STATUSWORD_WARNING                                   0x0080 /**< \brief Warning active*/
#define STATUSWORD_MANUFACTORSPECIFIC                        0x0100 /**< \brief Manufacturer specific*/
#define STATUSWORD_INTERNAL_LIMIT                            0x0800 /**< \brief Internal limit*/
#define STATUSWORD_REMOTE                                    0x0200 /**< \brief Set if the control word is processed*/
#define STATUSWORD_TARGET_REACHED                            0x0400 /**< \brief Target reached*/
#define STATUSWORD_INTERNALLIMITACTIVE                       0x0800 /**< \brief Internal limit active*/
#define STATUSWORD_DRIVE_FOLLOWS_COMMAND                     0x1000 /**< \brief Drive follows command (used in cyclic synchronous modes)*/

/*---------------------------------------------
-    StatusWord
-----------------------------------------------*/
#define STATUSWORD_STATE_NOTREADYTOSWITCHON                  0x0000 /**< \brief Not ready to switch on*/
#define STATUSWORD_STATE_SWITCHEDONDISABLED                  0x0020 /**< \brief Switched on but disabled*/
#define STATUSWORD_STATE_READYTOSWITCHON                     0x0021 /**< \brief Ready to switch on*/
#define STATUSWORD_STATE_SWITCHEDON                          0x0023 /**< \brief Switched on*/
#define STATUSWORD_STATE_OPERATIONENABLED                    0x0027 /**< \brief Operation enabled*/
#define STATUSWORD_STATE_QUICKSTOPACTIVE                     0x0007 /**< \brief Quickstop active*/
#define STATUSWORD_STATE_FAULTREACTIONACTIVE                 0x000F /**< \brief Fault reaction active*/
#define STATUSWORD_STATE_FAULT                               0x0008 /**< \brief Fault state*/


/*---------------------------------------------
-    CiA402 State machine
-----------------------------------------------*/
#define STATE_NOT_READY_TO_SWITCH_ON        0x0001 /**< \brief Not ready to switch on (optional)*/
#define STATE_SWITCH_ON_DISABLED            0x0002 /**< \brief Switch on but disabled (optional)*/
#define STATE_READY_TO_SWITCH_ON            0x0004 /**< \brief Ready to switch on (mandatory)*/
#define STATE_SWITCHED_ON                   0x0008 /**< \brief Switch on (mandatory)*/
#define STATE_OPERATION_ENABLED             0x0010 /**< \brief Operation enabled (mandatory)*/
#define STATE_QUICK_STOP_ACTIVE             0x0020 /**< \brief Quick stop active (optional)*/
#define STATE_FAULT_REACTION_ACTIVE         0x0040 /**< \brief Fault reaction active (mandatory)*/
#define STATE_FAULT                         0x0080 /**< \brief Fault state (mandatory)*/




/**
 * \addtogroup CiA402Objects CiA402 Axis Objects
 * @{
 */
/** \brief 0x607D (Software Position Limit) data structure*/
typedef struct {
   sint32    i32MinLimit; /**< \brief Minimum limit*/
   sint32    i32MaxLimit; /**< \brief Maximum limit*/
}TOBJ607D;


/** \brief Object 0x60C2 (Interpolation Time Period) data structure*/
typedef struct {
   uint8    u8InterpolationPeriod; /**< \brief Interpolation period*/
   sint8    i8InterpolationIndex; /**< \brief Interpolation index*/
}TOBJ60C2;
/** @}*/



/*---------------------------------------------
-    Generic Axis structure
-----------------------------------------------*/

/**
 * \addtogroup CiA402Objects CiA402 Axis Objects
 * @{
 */
/** \brief Data structure to handle the axis specific object data*/
typedef struct 
{
    uint16   objErrorCode; /**< \brief Error Code (0x603F)*/
    uint16   objControlWord; /**< \brief Control Word (0x6040)*/
    uint16   objStatusWord; /**< \brief Status Word (0x6041)*/
    sint16   objQuickStopOptionCode; /**< \brief Quick Stop Option Code (0x605A)*/
    sint16   objShutdownOptionCode; /**< \brief Shutdown Option Code (0x605B)*/
    sint16   objDisableOperationOptionCode; /**< \brief Disable Operation Option Code (0x605C)*/
    sint16   objFaultReactionCode; /**< \brief Fault Reaction Code (0x605E)*/
    sint8    objModesOfOperation; /**< \brief Modes of Operation (0x6060)*/
    sint8    objModesOfOperationDisplay; /**< \brief Mode of Operation Display (0x6061)*/
    sint32   objPositionActualValue; /**< \brief Position Actual Value (0x6064)*/
    sint32   objVelocityActualValue; /**< \brief Actual Velocity Value (0x606C)*/
    sint16   objTorqueActualValue; /**< \brief Torque Actual Value (0x6077)*/
    sint32   objTargetPosition; /**< \brief Target Position (0x607A)*/
    TOBJ607D objSoftwarePositionLimit; /**< \brief Software Position limit (0x607D)*/
    uint32   objQuickStopDeclaration; /**< \brief Quick Stop Declaration (0x6085)*/
    TOBJ60C2 objInterpolationTimePeriod; /**< \brief Interpolation Time Period (0x60C2)*/
    sint32   objTargetVelocity; /**< \brief Target Velocity (0x60FF)*/
    uint32   objSupportedDriveModes; /**< \brief Supported Drive Modes (0x6502)*/
}CiA402Objects;

/** \brief Data structure to handle an Cia402 axis(single axis information)*/
typedef struct
{
    boolean         bAxisIsActive; /**< \brief Indicates if active is active*/
    boolean         bBrakeApplied; /**< \brief Motor break is applied*/
    boolean         bLowLevelPowerApplied; /**< \brief Low Level Power applied*/
    boolean         bHighLevelPowerApplied; /**< \brief High Level Power applied*/
    boolean         bAxisFunctionEnabled; /**< \brief Axis functions enabled*/
    boolean         bConfigurationAllowed; /**< \brief Configuration allowed*/

    uint16          i16State; /**< \brief Axis state*/
    uint16          u16PendingOptionCode; /**< \brief Pending operation code*/
    double          fCurPosition; /**< \brief Current position within control loop*/
    uint32          u32CycleTime; /**< \brief Motion controller cycletime in us*/

    uint16          objDigitalInput;
    uint16          objDigitalOutput;
    CiA402Objects   Objects; /**< \brief CiA402 Axis object variable*/
}TCiA402Axis, *pTCiA402Axis;







uint16 CiA402_Init(void);
void CiA402_MainLoop(void);




#endif

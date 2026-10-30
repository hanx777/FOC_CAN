

/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include "cia402appl.h"
#include "SguanFOC.h"
//#include "bsp_led.h"

const CiA402Objects DefCiA402ObjectValues=
{
   0x0, /**< \brief Error Code (0x603F)*/
   0x0,/*(UINT16) ControlWord 0x6040*/
   0x0,/*(UINT16) StatusWord 0x6041*/
   0x2,/*(INT16) QuickStopOptionCode 0x605A*/
   DISABLE_DRIVE,/*(INT16)ShutdownOptionCode 0x605B*/
   SLOW_DOWN_RAMP, /*(INT16) DisableOperationCode 0x605C*/
   QUICKSTOP_RAMP,/*(INT16) FaultReactionCode 0x605E*/
   0x0,/*(INT16) ModeOfOperation 0x6060*/
   0x0,/*(INT16) Mode Of Operation Display 0x6061*/
   0x0,/*(INT32) Position Actual Value 0x6064*/
   0x0,/*(INT32) Velocity Actual Value 0x606C*/
   0x0,/*(INT16) Torque Actual Value 0x6077*/
   0x0,/*(INT32) Target Position 0x607A*/
   {0x88CA6C00,0x77359400},/*TOBJ607D Software Position Limit (minLimit: -2000000000 / maxLimit: 2000000000)*/
   0x0,/*(UINT32) QuickStopDeclaration 0x6085*/
   {1,-3},/*TOBJ60C2 Interpolation Time Period*/
   0x0,/*(INT32) Target Velocity    0x60FF*/
   0x0/*(UINT32) Supported Drive Modes 0x6502*/
};

TCiA402Axis  LocalAxes[MAX_AXES];

uint16 CiA402_Init(void)
{
    uint16 result = 0;
    uint16 AxisCnt = 0;
    uint32 ObjectIndex = 0;

    for(uint16 i=0; i<objDict_SingleAxis_ObjdictSize; i++)
    {
        if(objDict_SingleAxis_objdict[i].index==0x603F)
        {
            ObjectIndex = i;
            break;
        }
        if(i == objDict_SingleAxis_ObjdictSize)
        {
            while(1);//error,not find cia402 object in object dictionary
        }
    }

    objDict_SingleAxis_objdict[ObjectIndex-2].pSubindex->pObject = &LocalAxes[AxisCnt].objDigitalInput;                 /*0x2100*/
    objDict_SingleAxis_objdict[ObjectIndex-1].pSubindex->pObject = &LocalAxes[AxisCnt].objDigitalOutput;                /*0x2101*/


    for(AxisCnt = 0; AxisCnt < MAX_AXES ; AxisCnt++)
    {
        /*Reset Axis buffer*/
        memset(&LocalAxes[AxisCnt],0,sizeof(TCiA402Axis));

        LocalAxes[AxisCnt].bAxisIsActive = TRUE;
        LocalAxes[AxisCnt].bBrakeApplied = TRUE;
        LocalAxes[AxisCnt].bLowLevelPowerApplied = TRUE;
        LocalAxes[AxisCnt].bHighLevelPowerApplied = FALSE;
        LocalAxes[AxisCnt].bAxisFunctionEnabled = FALSE;
        LocalAxes[AxisCnt].bConfigurationAllowed = TRUE;

        LocalAxes[AxisCnt].i16State = STATE_NOT_READY_TO_SWITCH_ON;
        LocalAxes[AxisCnt].u16PendingOptionCode = 0x00;

        LocalAxes[AxisCnt].fCurPosition = 0;
        LocalAxes[AxisCnt].u32CycleTime = 0;

        //bind Cia402 objects(maby not all the objects) to canopen object Dictionary
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objErrorCode;                 /*0xNx3f*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objControlWord;               /*0xNx40*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objStatusWord;                /*0xNx41*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objModesOfOperation;          /*0xNx60*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objModesOfOperationDisplay;   /*0xNx61*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objPositionActualValue;       /*0xNx64*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objVelocityActualValue;       /*0xNx6c*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objTargetPosition;            /*0xNx7A*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objSoftwarePositionLimit;     /*0xNx7D*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objQuickStopDeclaration;      /*0xNx85*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objTargetVelocity;            /*0xNxFF*/
        objDict_SingleAxis_objdict[ObjectIndex++].pSubindex->pObject = &LocalAxes[AxisCnt].Objects.objSupportedDriveModes;       /*0xNx02*/
        
        /***********************************
            init objects
        *************************************/

        /*set default values*/
        memcpy(&LocalAxes[AxisCnt].Objects,&DefCiA402ObjectValues,sizeof(CiA402Objects));
        
    }
    return result;
}

void CiA402_DeallocateAxis(void)
{
    
}

void CiA402_LocalError(uint16 ErrorCode)
{
    uint16 counter = 0;
    for(counter = 0; counter < MAX_AXES; counter++)
    {
        if(LocalAxes[counter].bAxisIsActive)
        {
            LocalAxes[counter].i16State = STATE_FAULT_REACTION_ACTIVE;
            LocalAxes[counter].Objects.objErrorCode = ErrorCode;
        }
    }
}

void CiA402_StateMachine(void)/*called by main function*/
{
	TCiA402Axis *pCiA402Axis;
	
    uint16 StatusWord = 0;
    uint16 ControlWord6040 = 0;
    uint16 counter = 0;

    for(counter = 0; counter < MAX_AXES;counter++)
    {
        if(!LocalAxes[counter].bAxisIsActive)
        {
            continue;
        }

        pCiA402Axis = &LocalAxes[counter];
        StatusWord = pCiA402Axis->Objects.objStatusWord;
        ControlWord6040 = pCiA402Axis->Objects.objControlWord;

        /*clear statusword state and controlword processed complete bits*/
        StatusWord &= ~(STATUSWORD_STATE_MASK | STATUSWORD_REMOTE);

        /*skip state state transition if the previous transition is pending*/
        if(pCiA402Axis->u16PendingOptionCode!= 0x0)
        {
            return;
        }
        /*skip transition 1 and 2*/
        if(pCiA402Axis->i16State < STATE_READY_TO_SWITCH_ON )
        {
            pCiA402Axis->i16State = STATE_READY_TO_SWITCH_ON;
        }    

        switch(pCiA402Axis->i16State)
        {
            case STATE_NOT_READY_TO_SWITCH_ON:
                StatusWord |= (STATUSWORD_STATE_NOTREADYTOSWITCHON);
                //if(nAlStatus == STATE_OP)
                //{
                    // Automatic transition -> Communication shall be activated
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED; // Transition 1
                //}
                // else
                // {
                //     /*CiA402 statemachine shall stay in "STATE_NOT_READY_TO_SWITCH_ON" if EtherCAT state is not OP.*/
                //     pCiA402Axis->i16State = STATE_NOT_READY_TO_SWITCH_ON; // stay in current state
                // }
                break;
            case STATE_SWITCH_ON_DISABLED:
                StatusWord |= (STATUSWORD_STATE_SWITCHEDONDISABLED);
                if ((ControlWord6040 & CONTROLWORD_COMMAND_SHUTDOWN_MASK) == CONTROLWORD_COMMAND_SHUTDOWN)
                {
                    pCiA402Axis->i16State = STATE_READY_TO_SWITCH_ON; // Transition 2
                }
                break;
            case STATE_READY_TO_SWITCH_ON:
                StatusWord |= (STATUSWORD_STATE_READYTOSWITCHON);
                if (((ControlWord6040 & CONTROLWORD_COMMAND_QUICKSTOP_MASK) == CONTROLWORD_COMMAND_QUICKSTOP)
                    || ((ControlWord6040 & CONTROLWORD_COMMAND_DISABLEVOLTAGE_MASK) == CONTROLWORD_COMMAND_DISABLEVOLTAGE))
                {
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED; // Transition 7
                } 
                else if (((ControlWord6040 & CONTROLWORD_COMMAND_SWITCHON_MASK) == CONTROLWORD_COMMAND_SWITCHON) ||
                        ((ControlWord6040 & CONTROLWORD_COMMAND_SWITCHON_ENABLEOPERATION_MASK) == CONTROLWORD_COMMAND_SWITCHON_ENABLEOPERATION))
                {
                    pCiA402Axis->i16State = STATE_SWITCHED_ON;           // Transition 3
                }
                break;
            case STATE_SWITCHED_ON:
                StatusWord |= (STATUSWORD_STATE_SWITCHEDON);

                if ((ControlWord6040 & CONTROLWORD_COMMAND_SHUTDOWN_MASK) == CONTROLWORD_COMMAND_SHUTDOWN)
                {
                    pCiA402Axis->i16State = STATE_READY_TO_SWITCH_ON; // Transition 6

                } 
                else if (((ControlWord6040 & CONTROLWORD_COMMAND_QUICKSTOP_MASK) == CONTROLWORD_COMMAND_QUICKSTOP
                        || (ControlWord6040 & CONTROLWORD_COMMAND_DISABLEVOLTAGE_MASK) == CONTROLWORD_COMMAND_DISABLEVOLTAGE))
                {
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED; // Transition 10

                } 
                else if ((ControlWord6040 & CONTROLWORD_COMMAND_ENABLEOPERATION_MASK) == CONTROLWORD_COMMAND_ENABLEOPERATION)
                {
                    pCiA402Axis->i16State = STATE_OPERATION_ENABLED;  // Transition 4
                    //The Axis function shall be enabled and all internal set-points cleared.
                }
                break;
            case STATE_OPERATION_ENABLED:
                StatusWord |= (STATUSWORD_STATE_OPERATIONENABLED);
                if ((ControlWord6040 & CONTROLWORD_COMMAND_DISABLEOPERATION_MASK) == CONTROLWORD_COMMAND_DISABLEOPERATION)
                {
                    if(pCiA402Axis->Objects.objDisableOperationOptionCode!= DISABLE_DRIVE)
                    {
                        /*disable operation pending*/
                        pCiA402Axis->u16PendingOptionCode = 0x605C; //STATE_TRANSITION (STATE_OPERATION_ENABLED,STATE_SWITCHED_ON);
                        return;
                    }
                    pCiA402Axis->i16State = STATE_SWITCHED_ON;           // Transition 5
                } 
                else if ((ControlWord6040 & CONTROLWORD_COMMAND_QUICKSTOP_MASK) == CONTROLWORD_COMMAND_QUICKSTOP)
                {
                    pCiA402Axis->i16State = STATE_QUICK_STOP_ACTIVE;  // Transition 11
                } 
                else if ((ControlWord6040 & CONTROLWORD_COMMAND_SHUTDOWN_MASK) == CONTROLWORD_COMMAND_SHUTDOWN)
                {
                    if(pCiA402Axis->Objects.objShutdownOptionCode != DISABLE_DRIVE)
                    {
                        /*shutdown operation required*/
                        pCiA402Axis->u16PendingOptionCode = 0x605B; //STATE_TRANSITION (STATE_OPERATION_ENABLED,STATE_READY_TO_SWITCH_ON);
                        return;
                    }
                    pCiA402Axis->i16State = STATE_READY_TO_SWITCH_ON; // Transition 8
                } 
                else if ((ControlWord6040 & CONTROLWORD_COMMAND_DISABLEVOLTAGE_MASK) == CONTROLWORD_COMMAND_DISABLEVOLTAGE)
                {
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED; // Transition 9
                }
                break;
            case STATE_QUICK_STOP_ACTIVE:
                StatusWord |= STATUSWORD_STATE_QUICKSTOPACTIVE;
                if((pCiA402Axis->Objects.objQuickStopOptionCode != DISABLE_DRIVE) &&
                    ((pCiA402Axis->Objects.objStatusWord & STATUSWORD_STATE_MASK)!= STATUSWORD_STATE_QUICKSTOPACTIVE))
                {
                    /*Only execute quick stop action in state transition 11*/
                    pCiA402Axis->u16PendingOptionCode = 0x605A;//STATE_TRANSITION (STATE_OPERATION_ENABLED,STATE_QUICK_STOP_ACTIVE);
                    return;
                }

                if ((ControlWord6040 & CONTROLWORD_COMMAND_DISABLEVOLTAGE_MASK) == CONTROLWORD_COMMAND_DISABLEVOLTAGE)
                {
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED; // Transition 12
                }

                /*NOTE: it is not recommend to support transition 16 */
                break;
            case STATE_FAULT_REACTION_ACTIVE:
                StatusWord |= (STATUSWORD_STATE_FAULTREACTIONACTIVE);
                if(pCiA402Axis->Objects.objFaultReactionCode!= DISABLE_DRIVE)
                {
                    /*fault reaction pending*/
                    pCiA402Axis->u16PendingOptionCode = 0x605E;
                    return;
                }

                // Automatic transition
                pCiA402Axis->i16State = STATE_FAULT;// Transition 14
                break;
            case STATE_FAULT:
                StatusWord |= (STATUSWORD_STATE_FAULT);
                if ((ControlWord6040 & CONTROLWORD_COMMAND_FAULTRESET_MASK) == CONTROLWORD_COMMAND_FAULTRESET)
                {
                    pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED;// Transition 15
                }
                break;

            default:    //the sate variable is set to in invalid value => rest Axis
                StatusWord = (STATUSWORD_STATE_NOTREADYTOSWITCHON);
                pCiA402Axis->i16State = STATE_NOT_READY_TO_SWITCH_ON;
                break;

        }// switch(current state)

        /*Update operational functions (the low level power supply is always TRUE*/
        switch(pCiA402Axis->i16State)
        {
            case STATE_NOT_READY_TO_SWITCH_ON:
            case STATE_SWITCH_ON_DISABLED:
            case STATE_READY_TO_SWITCH_ON:
                pCiA402Axis->bBrakeApplied = TRUE;
                pCiA402Axis->bHighLevelPowerApplied =  FALSE;
                pCiA402Axis->bAxisFunctionEnabled = FALSE;
                pCiA402Axis->bConfigurationAllowed = TRUE;
                break;
            case STATE_SWITCHED_ON:
                pCiA402Axis->bBrakeApplied = TRUE;
                pCiA402Axis->bHighLevelPowerApplied =  TRUE;
                pCiA402Axis->bAxisFunctionEnabled = FALSE;
                pCiA402Axis->bConfigurationAllowed = TRUE;
                break;
            case STATE_OPERATION_ENABLED:
            case STATE_QUICK_STOP_ACTIVE:
            case STATE_FAULT_REACTION_ACTIVE:
                pCiA402Axis->bBrakeApplied = FALSE;
                pCiA402Axis->bHighLevelPowerApplied =  TRUE;
                pCiA402Axis->bAxisFunctionEnabled = TRUE;
                pCiA402Axis->bConfigurationAllowed = FALSE;
                break;
            case STATE_FAULT:
                pCiA402Axis->bBrakeApplied = TRUE;
                pCiA402Axis->bHighLevelPowerApplied =  FALSE;
                pCiA402Axis->bAxisFunctionEnabled = FALSE;
                pCiA402Axis->bConfigurationAllowed = TRUE;
                break;
            default:
                pCiA402Axis->bBrakeApplied = TRUE;
                pCiA402Axis->bHighLevelPowerApplied =  FALSE;
                pCiA402Axis->bAxisFunctionEnabled = FALSE;
                pCiA402Axis->bConfigurationAllowed = TRUE;
                break;
        }

        if(pCiA402Axis->bHighLevelPowerApplied == TRUE)
            StatusWord |= STATUSWORD_VOLTAGE_ENABLED;
        else
            StatusWord &= ~STATUSWORD_VOLTAGE_ENABLED;
        /*state transition finished set controlword complete bit and update status object 0x6041*/
        pCiA402Axis->Objects.objStatusWord = (StatusWord | STATUSWORD_REMOTE);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 \brief    CiA402_MotionControl
 \brief this functions provides an simple motor control function
*////////////////////////////////////////////////////////////////////////////////////////
void CiA402_MotionControl(TCiA402Axis *pCiA402Axis)
{
    if((pCiA402Axis->Objects.objSoftwarePositionLimit.i32MaxLimit> pCiA402Axis->Objects.objPositionActualValue
        || pCiA402Axis->Objects.objPositionActualValue > pCiA402Axis->Objects.objTargetPosition) &&
        (pCiA402Axis->Objects.objSoftwarePositionLimit.i32MinLimit < pCiA402Axis->Objects.objPositionActualValue
        || pCiA402Axis->Objects.objPositionActualValue < pCiA402Axis->Objects.objTargetPosition))
    {
        pCiA402Axis->Objects.objStatusWord &= ~STATUSWORD_INTERNAL_LIMIT;

        switch(pCiA402Axis->Objects.objModesOfOperationDisplay)
        {
        case CYCLIC_SYNC_POSITION_MODE:
            Sguan.foc.Target_Pos = (float)pCiA402Axis->Objects.objTargetPosition*0.017453f;//angle to rad
            pCiA402Axis->Objects.objPositionActualValue= (uint32)(Sguan.encoder.Real_Pos*57.2957795f);  
            break;
        case CYCLIC_SYNC_VELOCITY_MODE:
            //Motor_swtMode_C = 2;
            //Speed_nSetPoint_C = (float)pCiA402Axis->Objects.objTargetVelocity;
            //pCiA402Axis->Objects.objVelocityActualValue= (uint32)Speed_nMeas_S;  
            break;        
        default:
            break;
        }        
    }
    else
    {
        pCiA402Axis->Objects.objStatusWord |= STATUSWORD_INTERNAL_LIMIT;//over limit
    }
    /*Accept new mode of operation*/
    pCiA402Axis->Objects.objModesOfOperationDisplay = pCiA402Axis->Objects.objModesOfOperation;

}

boolean CiA402_TransitionAction(sint16 Characteristic,TCiA402Axis *pCiA402Axis)
{
   boolean ret = FALSE;
   switch(Characteristic)
   {

   case SLOW_DOWN_RAMP:
      //printf( "Implement slowdown ramp");
      ret = TRUE;
      break;
   case QUICKSTOP_RAMP:
      //printf( "Implement quick stop ramp ramp");
      ret = TRUE;
      break;
   case STOP_ON_CURRENT_LIMIT:
      //printf( "Implement slowdown on current limit ramp");
      ret = TRUE;
      break;
   case STOP_ON_VOLTAGE_LIMIT:
      //printf( "Implement slowdown on voltage limit ramp");
      ret = TRUE;
      break;
   default:
      ret = FALSE;
      break;
   }
    return ret;
}


void CiA402_Application(TCiA402Axis *pCiA402Axis)
{
    /*clear "Drive follows the command value" flag if the target values from the master overwritten by the local application*/
    if(pCiA402Axis->u16PendingOptionCode != 0 &&
        (pCiA402Axis->Objects.objModesOfOperationDisplay == CYCLIC_SYNC_POSITION_MODE ||
        pCiA402Axis->Objects.objModesOfOperationDisplay == CYCLIC_SYNC_VELOCITY_MODE))
    {
        pCiA402Axis->Objects.objStatusWord &= ~ STATUSWORD_DRIVE_FOLLOWS_COMMAND;
    }
    else
        pCiA402Axis->Objects.objStatusWord |= STATUSWORD_DRIVE_FOLLOWS_COMMAND;


    switch(pCiA402Axis->u16PendingOptionCode)
    {
        case 0x605A:
            /*state transition 11 is pending analyse quick stop option code (0x605A)*/
            {
                uint16 ramp = pCiA402Axis->Objects.objQuickStopOptionCode;
                /*masked and execute specified quick stop ramp characteristic */
                if(pCiA402Axis->Objects.objQuickStopOptionCode > 4 && pCiA402Axis->Objects.objQuickStopOptionCode <9)
                {
                    if(pCiA402Axis->Objects.objQuickStopOptionCode == 5)
                        ramp = 1;
                    if(pCiA402Axis->Objects.objQuickStopOptionCode == 6)
                        ramp = 2;
                    if(pCiA402Axis->Objects.objQuickStopOptionCode == 7)
                        ramp = 3;
                    if(pCiA402Axis->Objects.objQuickStopOptionCode == 8)
                        ramp = 4;
                }

                if(CiA402_TransitionAction(ramp,pCiA402Axis))
                {
                    /*quick stop ramp is finished complete state transition*/
                    pCiA402Axis->u16PendingOptionCode = 0x0;
                    if(pCiA402Axis->Objects.objQuickStopOptionCode > 0 && pCiA402Axis->Objects.objQuickStopOptionCode < 5)
                    {
                        pCiA402Axis->i16State = STATE_SWITCH_ON_DISABLED;    //continue state transition 12
                    }
                    else if(pCiA402Axis->Objects.objQuickStopOptionCode > 4 && pCiA402Axis->Objects.objQuickStopOptionCode < 9)
                        pCiA402Axis->Objects.objStatusWord |= STATUSWORD_TARGET_REACHED;
                }
            }
            break;
        case 0x605B:
            /*state transition 8 is pending analyse shutdown option code (0x605B)*/
            {
                if(CiA402_TransitionAction(pCiA402Axis->Objects.objShutdownOptionCode,pCiA402Axis))
                {
                    /*shutdown ramp is finished complete state transition*/
                    pCiA402Axis->u16PendingOptionCode = 0x0;
                    pCiA402Axis->i16State = STATE_READY_TO_SWITCH_ON;    //continue state transition 8
                }
            }
            break;
        case 0x605C:
            /*state transition 5 is pending analyse Disable operation option code (0x605C)*/
            {
                if(CiA402_TransitionAction(pCiA402Axis->Objects.objDisableOperationOptionCode,pCiA402Axis))
                {
                    /*disable operation ramp is finished complete state transition*/
                    pCiA402Axis->u16PendingOptionCode = 0x0;
                    pCiA402Axis->i16State = STATE_SWITCHED_ON;    //continue state transition 5
                }
            }
            break;

        case 0x605E:
            /*state transition 14 is pending analyse Fault reaction option code (0x605E)*/
            {
                if(CiA402_TransitionAction(pCiA402Axis->Objects.objFaultReactionCode,pCiA402Axis))
                {
                    /*fault reaction ramp is finished complete state transition*/
                    pCiA402Axis->u16PendingOptionCode = 0x0;
                    pCiA402Axis->i16State = STATE_FAULT;    //continue state transition 14
                }
            }
            break;
        default:
            //pending transition code is invalid => values from the master are used
            pCiA402Axis->Objects.objStatusWord |= STATUSWORD_DRIVE_FOLLOWS_COMMAND;
            break;
    }
    CiA402_MotionControl(pCiA402Axis);
}


void Multiple_Axis_Application(void)/*isr run*/
{
    uint16 i;

    for(i = 0; i < MAX_AXES;i++)
    {
        if(LocalAxes[i].bAxisIsActive)
        {
            CiA402_Application(&LocalAxes[i]);
        }    
    }
}

void CiA402_MainLoop(void)
{
    Multiple_Axis_Application();
    /* cia402 status update */
    CiA402_StateMachine();
}





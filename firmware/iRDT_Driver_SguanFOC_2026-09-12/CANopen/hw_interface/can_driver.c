/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN
AT91 Port: Peter CHRISTEN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "canfestival.h"
#include "fdcan.h"

extern FDCAN_HandleTypeDef 	hfdcan2;
FDCAN_RxHeaderTypeDef       fdcan_rx_header = {0};
uint8_t 					fdcan_rx_buffer[8] = {0};

uint16_t                    Canopen_ReceiveFlag  = 0U;
uint8_t                     Canopen_NodeId = NODE_ID;


/******************************************************************************
The driver send a CAN message passed from the CANopen stack
INPUT   CAN_PORT is not used (only 1 avaiable)
        Message *m pointer to message to send
OUTPUT  1 if  hardware -> CAN frame
******************************************************************************/
unsigned char canSend(CAN_PORT notused, Message *m)
{    
    FDCAN_TxHeaderTypeDef        fdcan_tx_header = {0};

	fdcan_tx_header.Identifier = m->cob_id;//0 and 0x7FF, if IdType is FDCAN_STANDARD_ID
	fdcan_tx_header.IdType = FDCAN_STANDARD_ID;
    if(m->rtr)
    {
        fdcan_tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
    }
    else
    {  
        fdcan_tx_header.TxFrameType = FDCAN_DATA_FRAME;
    }
	
	fdcan_tx_header.DataLength = (uint32_t)(m->len);
	fdcan_tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	fdcan_tx_header.BitRateSwitch = FDCAN_BRS_ON;
	fdcan_tx_header.FDFormat = FDCAN_CLASSIC_CAN;
	fdcan_tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	fdcan_tx_header.MessageMarker = 0;
	
	HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &fdcan_tx_header, &m->data[0]);
    return 1; // successful
}


/******************************************************************************
The driver passes a received CAN message to the stack
INPUT   none
OUTPUT  none
******************************************************************************/
void CANOpen_CAN_CallBack(void)
{
    if(Canopen_ReceiveFlag == 1)
    {
        Canopen_ReceiveFlag = 0;
        /*when receive message, process the message*/
        Message RxMSG = Message_Initializer;
        UNS32 i;

        RxMSG.cob_id = fdcan_rx_header.Identifier;
        RxMSG.len = fdcan_rx_header.DataLength;
        for(i = 0; i < RxMSG.len; i++)
        {
            RxMSG.data[i] = fdcan_rx_buffer[i];
        }
        canDispatch(&objDict_SingleAxis_Data, &RxMSG);         // process it
    }
}
/******************************************************************************
The driver interrupt callback function, when a new message is received, this function will be called
INPUT   *hfdcan pointer to CAN node
        RxFifo0ITs is FIFO0 interrupt status
OUTPUT  none
******************************************************************************/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{ 
    if(hfdcan->Instance == FDCAN2)
    {
        if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
        {
             if(HAL_FDCAN_GetRxMessage(&hfdcan2, FDCAN_RX_FIFO0, 
                                     &fdcan_rx_header, fdcan_rx_buffer) == HAL_OK)
            {
                Canopen_ReceiveFlag = 1;
            }
        }
    }
}



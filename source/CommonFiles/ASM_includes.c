void CompareNodeIdForBroadcast(void) {
#pragma asm
EXTERN  XDATA (nodeID)
									MOV      A,R7
									XRL      A,#0xff
									JNZ      NIK_EXIT_SEGMENT
									MOV      DPTR,#nodeID
									MOVX 	 A,@DPTR
									MOV 	 R7,A
NIK_EXIT_SEGMENT:					MOV      DPTR,#nodeID
#pragma endasm
}

void SendBeamBroadcastInjection(void) {
#pragma asm
EXTERN XDATA (broadcastBeamFlag)
									;
									;
									; start of my stuff
									;
									;
									; check if we need to modify
									;
									;
									MOV      DPTR,#broadcastBeamFlag
					                MOVX     A,@DPTR
					                JZ 		 NIK_FLAG_NOT_SET_SEGMENT
									;
									;
					                ; start modifications
					                MOV      DPTR,#0x042F ;bBeamAddress
					                MOV 	 A,#0xff ; Set the correct beam address
					                MOVX     @DPTR,A
					                ;
					                ;
									; reset the flag
									MOV      DPTR,#broadcastBeamFlag
									CLR 	 A
					                MOVX     @DPTR,A
					                ;
					                ;
					                ORL 	 A,#40H ; Set the Flirs flag
						            RET
					                ;
					                ; end of my modifications
					                ;
									;
									;
									;
NIK_FLAG_NOT_SET_SEGMENT:			MOV      R5,A
						            ANL      A,#060H
#pragma endasm
}

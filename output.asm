TEST     START    0x1000



         CLEAR    X
         CLEAR    A
         +LDT     #1024
$AALOOP  TD       =X'F2'
         JEQ      $AALOOP
         RD       =X'F2'
         STCH     BUFFER,X
$AAEXIT  STX      LENGTH
         CLEAR    X
         CLEAR    A
         +LDT     #4096
$ABLOOP  TD       =X'F1'
         JEQ      $ABLOOP
         RD       =X'F1'
         STCH     AREA,X
$ABEXIT  STX      LENGTH
         LDA      XA1
         ADD      XA2
         ADD      XA3
         STA      XAS
         LDA      XBETA1
         ADD      XBETA2
         ADD      XBETA3
         STA      XBETAS

         END      TEST

BUFFER   RESB     1
AREA     RESB     1
LENGTH   WORD     1

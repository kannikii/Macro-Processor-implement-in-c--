TEST     START   0x1000

RDBUFF   MACRO   &INDEV=F1, &BUFADR=, &RECLTH=, &EOR=04, &MAXLTH=
         CLEAR   X
         CLEAR   A
IF (&MAXLTH EQ ' ')
         +LDT    #4096
ELSE
         +LDT    #&MAXLTH
ENDIF
$LOOP    TD      =X'&INDEV'
         JEQ     $LOOP
         RD      =X'&INDEV'
         STCH    &BUFADR,X
$EXIT    STX     &RECLTH
         MEND

SUM      MACRO   &ID
         LDA     X&ID->1
         ADD     X&ID->2
         ADD     X&ID->3
         STA     X&ID->S
         MEND

         RDBUFF  F2, BUFFER, LENGTH, (00,03,04), 1024
         RDBUFF  reclth=LENGTH, bufadr=AREA
         SUM     A
         SUM     BETA

         END     TEST

BUFFER   RESB    1
AREA     RESB    1
LENGTH   WORD    1

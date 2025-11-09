TEST     START   0x1000
F1       BYTE    X'00'
F2       BYTE    X'05'

         CLEAR   X
TF1      TD      =X'F1'
         JEQ     TF1
         RD      =X'F1'
         STCH    BUFFER,X

         CLEAR   X
TF2      TD      =X'F2'
         JEQ     TF2
         RD      =X'F2'
         STCH    AREA,X

         END     TEST
BUFFER   RESB    1
AREA     RESB    1

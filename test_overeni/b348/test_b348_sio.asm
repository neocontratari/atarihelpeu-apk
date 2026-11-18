; test_b348_sio.asm - B348: zavadec s VLASTNIM SIO (jako xBIOS v Prince of Persia).
; Bootovaci sektory 1-3 nahraje OS (pres SIOV - v emulatoru "SIO patch"), pak tento kod
; mluvi s jednotkou D1: primo pres POKEY a PIA: ramec prikazu se stazenou linkou
; COMMAND (PBCTL $34 / $3C), odpovedi 'A' / 'C' / 'N' / 'E' a data ze SERIN
; (dotazovani IRQST bit 5). Vysledky po 4 bajtech do RES, nakonec DONE = $5A.
; Po startu nastavi READY = $A5 a ceka na GO: test mezitim vymeni disketu v D1: (jako
; "VYMENIT DISKETU BEZ RESTARTU" v appce) a zapise tabulku testu (TAB, 8 B na test:
; jednotka, prikaz, AUX1, AUX2, pocet bajtu (0 = 256), priznaky, stranka bufferu, 0).
; Preklad: python3 asm6502.py test_b348_sio.asm test_b348_program.h B348 START,TAB,CMD
SKCTL  = $D20F
SKSTAT = $D20F
SKRES  = $D20A
SEROUT = $D20D
SERIN  = $D20D
IRQEN  = $D20E
IRQST  = $D20E
AUDF3  = $D204
AUDF4  = $D206
AUDC4  = $D207
AUDCTL = $D208
STIMER = $D209
PBCTL  = $D303
NMIEN  = $D40E

PTR    = $80          ; ukazatel do bufferu (2 B)
SUM    = $82          ; kontrolni soucet
CNT    = $83          ; pocet datovych bajtu (0 = 256)
TIDX   = $84          ; index do TAB (po 8 B)
RIDX   = $85          ; index do RES (po 4 B)
FLG    = $86          ; 1 = poslat datovy ramec, 2 = prijmout data, $80 = pokazit soucet prikazu
TMP    = $87
TO     = $88          ; citac cekani (2 B)
ERRS   = $8A          ; chyby prijmu (negovane SKSTAT bity 7 = ramec, 5 = preteceni)

RES    = $0600        ; [0] odpoved na prikaz, [1] na datovy ramec, [2] 'C' / 'E', [3] soucet: 1 sedi, 2 nesedi
DONE   = $06F0        ; $5A = vse probehlo
CAS1   = $06F1        ; IRQST bit 2 po cekani v asynchronnim prijmu: $04 = casovac 4 stal (spravne)
CAS2   = $06F2        ; totez bez asynchronniho prijmu: $00 = casovac 4 prerusil (spravne)
CHYBY  = $06F3        ; ERRS na konci (0 = zadna chyba ramce / preteceni)
READY  = $06FE        ; $A5 = program bezi a ceka na GO
GO     = $06FF        ; test zapise != 0, az je disketa a tabulka pripravena
VZOR   = $3400        ; data pro zapis sektoru: i * 3 + 7

        .org $0700
        .byte 0, 5              ; priznaky, pocet sektoru bootu (5 x 128 B)
        .word $0700             ; kam se nahraji
        .word INITRTS           ; DOSINI
START:  SEI
        LDA #0
        STA NMIEN               ; bez VBI (klid)
        STA TIDX
        STA RIDX
        STA ERRS
        TAX
CLR:    STA RES,X
        INX
        BNE CLR
VZ:     TXA
        STA TMP
        ASL A
        CLC
        ADC TMP
        CLC
        ADC #7
        STA VZOR,X
        INX
        BNE VZ
        LDA #$A5
        STA READY
WG:     LDA GO
        BEQ WG
NEXT:   LDX TIDX
        LDA TAB,X               ; jednotka (0 = konec tabulky)
        BEQ TIMERS
        STA CMD
        LDA TAB+1,X
        STA CMD+1
        LDA TAB+2,X
        STA CMD+2
        LDA TAB+3,X
        STA CMD+3
        LDA TAB+4,X
        STA CNT
        LDA TAB+5,X
        STA FLG
        LDA TAB+6,X
        STA PTR+1
        LDA #0
        STA PTR
        JSR SIO
        LDA TIDX
        CLC
        ADC #8
        STA TIDX
        LDA RIDX
        CLC
        ADC #4
        STA RIDX
        JMP NEXT

; casovac 4 v asynchronnim prijmu (SKCTL bit 4) bez prijmu stoji
TIMERS: LDA #0
        STA IRQEN
        STA AUDCTL              ; 64 kHz, kanaly samostatne
        LDA #$10
        STA AUDF4               ; (16 + 1) * 28 = 476 cyklu
        LDA #$13
        STA SKCTL               ; asynchronni prijem
        LDA #$04
        STA IRQEN               ; preruseni casovace 4
        STA STIMER
        JSR WAIT
        LDA IRQST
        AND #$04
        STA CAS1
        LDA #0
        STA IRQEN
        LDA #$03
        STA SKCTL               ; bez asynchronniho prijmu
        LDA #$04
        STA IRQEN
        STA STIMER
        JSR WAIT
        LDA IRQST
        AND #$04
        STA CAS2
        LDA #0
        STA IRQEN
        LDA ERRS
        STA CHYBY
        LDA #$5A
        STA DONE
KONEC:  JMP KONEC

WAIT:   LDY #10                 ; ~13 000 cyklu
W0:     LDX #0
W1:     DEX
        BNE W1
        DEY
        BNE W0
        RTS

; jeden prikaz SIO: CMD (4 B), CNT, FLG, PTR -> RES + RIDX
SIO:    LDA #$28
        STA AUDCTL              ; kanal 3 na 1,79 MHz, 3+4 spojene
        STA AUDF3               ; 19200 Bd jako OS a xBIOS
        LDA #0
        STA AUDF4
        STA AUDC4
        STA SUM
        TAY
CS:     LDA CMD,Y
        JSR ADDS
        INY
        CPY #4
        BNE CS
        LDA SUM
        BIT FLG
        BPL CS2
        EOR #$FF                ; schvalne spatny soucet
CS2:    STA CMD+4
        LDA #$34
        STA PBCTL               ; linka COMMAND stazena
        LDX #0
D1:     INX
        BPL D1
        LDA #$23
        STA SKCTL               ; vysilani (hodiny = casovac 4)
        STA SKRES
        LDA #$10
        STA IRQEN               ; preruseni "chci dalsi bajt"
        LDY #0
SC:     LDA CMD,Y
        JSR SENDB
        INY
        CPY #5
        BNE SC
        JSR TXEND
        LDA #$3C
        STA PBCTL               ; linka COMMAND uvolnena -> jednotka odpovi
        LDY RIDX
        JSR GETB
        BCS SIOX
        TXA
        STA RES,Y
        CMP #'A'
        BNE SIOX
        LDA FLG
        AND #1
        BEQ NOWR
        JSR SENDF               ; zapis: datovy ramec
        JSR GETB
        BCS SIOX
        TXA
        STA RES+1,Y
        CMP #'A'
        BNE SIOX
NOWR:   JSR GETB
        BCS SIOX
        TXA
        STA RES+2,Y
        LDA FLG
        AND #2
        BEQ SIOX
        LDA #0
        STA SUM
        STY TMP
        LDY #0
RD:     JSR GETB
        BCS RDX
        TXA
        STA (PTR),Y
        JSR ADDS
        INY
        CPY CNT
        BNE RD
        JSR GETB                ; kontrolni soucet
        BCS RDX
        LDY TMP
        LDA #2
        CPX SUM
        BNE RD2
        LDA #1
RD2:    STA RES+3,Y
SIOX:   LDA #0
        STA IRQEN
        RTS
RDX:    LDY TMP
        JMP SIOX

; bajt ze SERIN do X (C = 1: nic neprislo za ~0,6 s)
GETB:   LDA #0
        STA TO
        STA TO+1
GB1:    LDA #$20
        AND IRQST
        BEQ GB2
        INC TO
        BNE GB1
        INC TO+1
        BNE GB1
        SEC
        RTS
GB2:    LDX SERIN
        LDA SKSTAT
        EOR #$FF
        AND #$A0
        ORA ERRS
        STA ERRS
        STA SKRES
        LDA #0
        STA IRQEN               ; potvrdit preruseni ...
        LDA #$20
        STA IRQEN               ; ... a znovu povolit
        CLC
        RTS

; poslat bajt v A (SEROUT), pockat, az si ho POKEY vezme
SENDB:  STA SEROUT
SB1:    LDA #$10
        AND IRQST
        BNE SB1
        LDA #0
        STA IRQEN
        LDA #$10
        STA IRQEN
        RTS

; konec vysilani -> prijem
TXEND:  LDA #$08
        AND IRQST
        BNE TXEND
        LDA #$13
        STA SKCTL
        STA SKRES
        LDA #$20
        STA IRQEN
        RTS

; datovy ramec z (PTR), CNT bajtu + soucet
SENDF:  LDA #$23
        STA SKCTL
        STA SKRES
        LDA #$10
        STA IRQEN
        LDA #0
        STA SUM
        STY TMP
        LDY #0
SF1:    LDA (PTR),Y
        PHA
        JSR ADDS
        PLA
        JSR SENDB
        INY
        CPY CNT
        BNE SF1
        LDA SUM
        JSR SENDB
        LDY TMP
        JMP TXEND

; soucet s prenosem dokola (jako SIO)
ADDS:   CLC
        ADC SUM
        ADC #0
        STA SUM
        RTS
INITRTS: RTS
CMD:    .byte 0, 0, 0, 0, 0
TAB:    .byte 0                 ; tabulku testu zapise test v C++ (az za koncem programu)

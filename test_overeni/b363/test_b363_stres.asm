; test_b363_stres.asm - B363: zatez pro ulozeni / nahrani stavu Atari (online hra): kazdy snimek POKEY 1 i 2
; (stereo, casovac druheho), rozsirena pamet (16 bank, v 1088K jine), VBXE (MEMAC A okno $9000 do VRAM, blitter,
; paleta 1), vstupy (joysticky, FIRE, konzole, klavesa) do pameti a do barvy pozadi. OS bezi (VBI, klavesnice).
; XEX: segment $2000 + RUNAD. Preklad: python3 ../b348/asm6502.py test_b363_stres.asm test_b363_stres.h B363S START,PBTAB
NMIEN   = $D40E
VCOUNT  = $D40B
COLBK   = $D01A
RANDOM  = $D20A
KBCODE  = $D209
AUDF1   = $D200
AUDC1   = $D201
AUDF1B  = $D210
AUDC1B  = $D211
AUDF2B  = $D212
AUDC2B  = $D213
AUDCTLB = $D218
STIMERB = $D219
SKCTLB  = $D21F
PORTA   = $D300
PORTB   = $D301
TRIG0   = $D010
TRIG1   = $D011
CONSOL  = $D01F
VCTL    = $D640
CSEL    = $D644
PSEL    = $D645
CR      = $D646
CG      = $D647
CB      = $D648
BLADR0  = $D650
BLADR1  = $D651
BLADR2  = $D652
BLSTART = $D653
MEMACC  = $D65E
MEMACA  = $D65F
SDMCTL  = $022F        ; OS stin DMACTL
PCOLR0  = $02C0
GRACTL  = $D01D
PMBASE  = $D407
HPOSP0  = $D000
HPOSM0  = $D004
SIZEP0  = $D008
P0PF    = $D004
P0PL    = $D00C
M0PF    = $D000
HITCLR  = $D01E
PRIOR   = $D01B
POKMSK  = $10          ; OS stin IRQEN
VTIMR1  = $0210        ; vektor preruseni casovace 1
IRQEN   = $D20E
AUDCTL  = $D208
STIMER  = $D209
CNT     = $80          ; citac snimku (2 B)
TMP     = $82

        .org $2000
START:  LDA #0
        STA CNT
        STA CNT+1
        LDA #3
        STA SKCTLB              ; POKEY 2 bezi (bez preruseni)
        LDA #0
        STA AUDCTLB
        ; BCB pro blitter na VRAM $00000 (okno MEMAC A $9000, banka 0)
        LDA #$98
        STA MEMACC
        LDA #$80
        STA MEMACA
        LDX #20
BI:     LDA BCB,X
        STA $9000,X
        DEX
        BPL BI
        LDA #0
        STA MEMACA
        ; hraci a strely: PMBASE $3000 (dvojradkove), hraci 0-3 = pruhy, vic barev, kolize s polem i mezi sebou
        LDA #$30
        STA PMBASE
        LDX #0
PI:     TXA
        AND #$3C
        STA $3200,X             ; strely
        TXA
        STA $3200+256,X         ; (stranka $33 = nic, jen vyplne)
        EOR #$5A
        STA $3400,X             ; hrac 0
        STA $3480,X             ; hrac 1
        STA $3500,X             ; hrac 2
        STA $3580,X             ; hrac 3
        INX
        BNE PI
        LDA #$2E
        STA SDMCTL              ; pole + hraci a strely (dvojradkove)
        LDA #3
        STA GRACTL
        LDX #3
PC:     TXA
        ASL A
        ASL A
        ASL A
        ASL A
        ORA #$0A
        STA PCOLR0,X
        DEX
        BPL PC
        ; preruseni casovace 1 (POKEY 1, 15 kHz, AUDF1 >= $80: 1-2x za snimek na ruznych radcich)
        SEI
        LDA #<TIMIRQ
        STA VTIMR1
        LDA #>TIMIRQ
        STA VTIMR1+1
        LDA #1
        STA AUDCTL
        LDA POKMSK
        ORA #1
        STA POKMSK
        STA IRQEN
        STA STIMER
        CLI
LOOP:
W1:     LDA VCOUNT              ; jednou za snimek (radek 100)
        CMP #50
        BNE W1
W2:     LDA VCOUNT
        CMP #50
        BEQ W2
        INC CNT
        BNE L1
        INC CNT+1
L1:     LDA RANDOM              ; POKEY 1 (casovac 1 s prerusenim: AUDF1 >= $80)
        ORA #$80
        STA AUDF1
        LDA #$A4
        STA AUDC1
        LDA CNT                 ; POKEY 2 (stereo)
        STA AUDF1B
        LDA #$A6
        STA AUDC1B
        LDA RANDOM
        STA AUDF2B
        LDA #$C3
        STA AUDC2B
        LDA CNT
        AND #15
        BNE L2
        STA STIMERB             ; casovace POKEY 2 znovu
L2:     LDA PORTA               ; vstupy -> pamet a barva pozadi
        STA $0600
        EOR CNT
        STA COLBK
        LDA TRIG0
        STA $0601
        LDA TRIG1
        STA $0602
        LDA CONSOL
        STA $0603
        LDA KBCODE
        STA $0604
        LDA CNT                 ; rozsirena pamet: banka podle snimku
        AND #$0F
        TAX
        LDA PBTAB,X
        STA PORTB
        LDA CNT
        STA $4000,X
        LDA RANDOM
        STA $5000,X
        LDA $0600
        STA $7F00,X
        LDA $4010,X
        STA $0605
        LDA #$FF
        STA PORTB
        LDA #$98                ; VBXE: okno MEMAC A na $9000, 4 KB banka podle snimku
        STA MEMACC
        LDA CNT
        AND #$7F
        ORA #$80
        STA MEMACA
        LDA RANDOM
        STA $9800,X
        LDA CNT
        STA $9900,X
        LDA $9820,X
        STA $0606
        LDA #0
        STA MEMACA
        LDA CNT                 ; blitter kazdych 8 snimku
        AND #7
        BNE L3
        STA BLADR0
        STA BLADR1
        STA BLADR2
        LDA #1
        STA BLSTART
L3:     LDA CNT                 ; hraci se hybou (kolize s polem a mezi sebou)
        STA HPOSP0
        EOR #$40
        STA HPOSP0+1
        CLC
        ADC #33
        STA HPOSP0+2
        LDA $0600
        STA HPOSP0+3
        STA HPOSM0
        LDA CNT
        AND #3
        STA SIZEP0
        LDA P0PF                ; kolize do pameti (meni stav)
        STA $0607
        LDA P0PL
        STA $0608
        LDA M0PF
        STA $0609
        LDA CNT
        AND #$3F
        BNE L4
        STA HITCLR              ; kolize smazat jen jednou za 64 snimku
L4:     LDA CNT
        AND #$C0
        ORA #$01
        STA PRIOR
        LDA #1                  ; paleta 1
        STA PSEL
        LDA CNT
        STA CSEL
        STA CR
        EOR #$FF
        STA CG
        STA CB
        JMP LOOP
; preruseni casovace 1: pocitat, znovu povolit (OS uz A ulozil na zasobnik)
TIMIRQ: INC $0610
        BNE TI1
        INC $0611
TI1:    LDA POKMSK
        AND #$FE
        STA IRQEN
        LDA POKMSK
        STA IRQEN
        PLA
        RTI
; PORTB pro banky 0-15 (320K Compy Shop): bity 2-3 = banka, 6-7 obracene = vyssi bity, bit 4 = 0 procesor, OS zap
PBTAB:  .byte $E3,$E7,$EB,$EF,$A3,$A7,$AB,$AF,$63,$67,$6B,$6F,$23,$27,$2B,$2F
; BCB: zdroj $01000 (krok radku 32, bodu 1), cil $02000 (32, 1), sirka 32, vyska 8, AND $FF, XOR 0, kolize 0,
; zoom 0, vzor 0, rizeni 0 (jeden BCB, kopie)
BCB:    .byte $00,$10,$00,32,0,1,$00,$20,$00,32,0,1,31,0,7,$FF,$00,$00,$00,$00,$00

# Generates cpu.js: complete documented 6502 (incl. NMOS BCD) from a table.
OPS = """
00 BRK imp 7|01 ORA izx 6|05 ORA zp 3|06 ASL zp 5|08 PHP imp 3|09 ORA imm 2|0A ASL acc 2|0D ORA abs 4|0E ASL abs 6
10 BPL rel 2|11 ORA izy 5|15 ORA zpx 4|16 ASL zpx 6|18 CLC imp 2|19 ORA aby 4|1D ORA abx 4|1E ASL abx 7
20 JSR abs 6|21 AND izx 6|24 BIT zp 3|25 AND zp 3|26 ROL zp 5|28 PLP imp 4|29 AND imm 2|2A ROL acc 2|2C BIT abs 4|2D AND abs 4|2E ROL abs 6
30 BMI rel 2|31 AND izy 5|35 AND zpx 4|36 ROL zpx 6|38 SEC imp 2|39 AND aby 4|3D AND abx 4|3E ROL abx 7
40 RTI imp 6|41 EOR izx 6|45 EOR zp 3|46 LSR zp 5|48 PHA imp 3|49 EOR imm 2|4A LSR acc 2|4C JMP abs 3|4D EOR abs 4|4E LSR abs 6
50 BVC rel 2|51 EOR izy 5|55 EOR zpx 4|56 LSR zpx 6|58 CLI imp 2|59 EOR aby 4|5D EOR abx 4|5E LSR abx 7
60 RTS imp 6|61 ADC izx 6|65 ADC zp 3|66 ROR zp 5|68 PLA imp 4|69 ADC imm 2|6A ROR acc 2|6C JMP ind 5|6D ADC abs 4|6E ROR abs 6
70 BVS rel 2|71 ADC izy 5|75 ADC zpx 4|76 ROR zpx 6|78 SEI imp 2|79 ADC aby 4|7D ADC abx 4|7E ROR abx 7
81 STA izx 6|84 STY zp 3|85 STA zp 3|86 STX zp 3|88 DEY imp 2|8A TXA imp 2|8C STY abs 4|8D STA abs 4|8E STX abs 4
90 BCC rel 2|91 STA izy 6|94 STY zpx 4|95 STA zpx 4|96 STX zpy 4|98 TYA imp 2|99 STA aby 5|9A TXS imp 2|9D STA abx 5
A0 LDY imm 2|A1 LDA izx 6|A2 LDX imm 2|A4 LDY zp 3|A5 LDA zp 3|A6 LDX zp 3|A8 TAY imp 2|A9 LDA imm 2|AA TAX imp 2|AC LDY abs 4|AD LDA abs 4|AE LDX abs 4
B0 BCS rel 2|B1 LDA izy 5|B4 LDY zpx 4|B5 LDA zpx 4|B6 LDX zpy 4|B8 CLV imp 2|B9 LDA aby 4|BA TSX imp 2|BC LDY abx 4|BD LDA abx 4|BE LDX aby 4
C0 CPY imm 2|C1 CMP izx 6|C4 CPY zp 3|C5 CMP zp 3|C6 DEC zp 5|C8 INY imp 2|C9 CMP imm 2|CA DEX imp 2|CC CPY abs 4|CD CMP abs 4|CE DEC abs 6
D0 BNE rel 2|D1 CMP izy 5|D5 CMP zpx 4|D6 DEC zpx 6|D8 CLD imp 2|D9 CMP aby 4|DD CMP abx 4|DE DEC abx 7
E0 CPX imm 2|E1 SBC izx 6|E4 CPX zp 3|E5 SBC zp 3|E6 INC zp 5|E8 INX imp 2|E9 SBC imm 2|EA NOP imp 2|EC CPX abs 4|ED SBC abs 4|EE INC abs 6
F0 BEQ rel 2|F1 SBC izy 5|F5 SBC zpx 4|F6 INC zpx 6|F8 SED imp 2|F9 SBC aby 4|FD SBC abx 4|FE INC abx 7
"""
EA = {  # effective-address snippets; 'pc' already past opcode
 'imm':'ea=c.pc; c.pc=(c.pc+1)&0xFFFF;',
 'zp':'ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF;',
 'zpx':'ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF;',
 'zpy':'ea=(rd(c.pc)+c.y)&0xFF; c.pc=(c.pc+1)&0xFFFF;',
 'abs':'ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF;',
 'abx':'ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF;',
 'aby':'ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF;',
 'izx':'t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8);',
 'izy':'t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF;',
 'ind':'t=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; ea=rd(t)|(rd((t&0xFF00)|((t+1)&0xFF))<<8);', # 6502 page-wrap bug
 'rel':'t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=(c.pc+((t&0x80)?t-256:t))&0xFFFF;',
 'acc':'', 'imp':'',
}
BODY = {
 'LDA':'c.a=rd(ea); nz(c.a);', 'LDX':'c.x=rd(ea); nz(c.x);', 'LDY':'c.y=rd(ea); nz(c.y);',
 'STA':'wr(ea,c.a);', 'STX':'wr(ea,c.x);', 'STY':'wr(ea,c.y);',
 'ADC':'adc(rd(ea));', 'SBC':'sbc(rd(ea));',
 'AND':'c.a&=rd(ea); nz(c.a);', 'ORA':'c.a|=rd(ea); nz(c.a);', 'EOR':'c.a^=rd(ea); nz(c.a);',
 'CMP':'t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF);',
 'CPX':'t=rd(ea); c.cf=c.x>=t?1:0; nz((c.x-t)&0xFF);',
 'CPY':'t=rd(ea); c.cf=c.y>=t?1:0; nz((c.y-t)&0xFF);',
 'BIT':'t=rd(ea); c.zf=(c.a&t)===0?1:0; c.nf=(t>>7)&1; c.vf=(t>>6)&1;',
 'INC':'t=(rd(ea)+1)&0xFF; wr(ea,t); nz(t);', 'DEC':'t=(rd(ea)-1)&0xFF; wr(ea,t); nz(t);',
 'INX':'c.x=(c.x+1)&0xFF; nz(c.x);', 'DEX':'c.x=(c.x-1)&0xFF; nz(c.x);',
 'INY':'c.y=(c.y+1)&0xFF; nz(c.y);', 'DEY':'c.y=(c.y-1)&0xFF; nz(c.y);',
 'TAX':'c.x=c.a; nz(c.x);', 'TXA':'c.a=c.x; nz(c.a);', 'TAY':'c.y=c.a; nz(c.y);', 'TYA':'c.a=c.y; nz(c.a);',
 'TSX':'c.x=c.sp; nz(c.x);', 'TXS':'c.sp=c.x;',
 'PHA':'push(c.a);', 'PLA':'c.a=pop(); nz(c.a);', 'PHP':'push(packP(1));', 'PLP':'unpackP(pop());',
 'CLC':'c.cf=0;', 'SEC':'c.cf=1;', 'CLI':'c.if_=0;', 'SEI':'c.if_=1;', 'CLD':'c.df=0;', 'SED':'c.df=1;', 'CLV':'c.vf=0;',
 'NOP':'',
 'JMP':'c.pc=ea;', 'JSR':'t=(c.pc-1)&0xFFFF; push(t>>8); push(t&0xFF); c.pc=ea;',
 'RTS':'t=pop(); c.pc=((t|(pop()<<8))+1)&0xFFFF;',
 'RTI':'unpackP(pop()); t=pop(); c.pc=t|(pop()<<8);',
 'BRK':'t=(c.pc+1)&0xFFFF; push(t>>8); push(t&0xFF); push(packP(1)); c.if_=1; c.pc=rd(0xFFFE)|(rd(0xFFFF)<<8);',
 'BPL':'if(!c.nf)c.pc=ea;', 'BMI':'if(c.nf)c.pc=ea;', 'BVC':'if(!c.vf)c.pc=ea;', 'BVS':'if(c.vf)c.pc=ea;',
 'BCC':'if(!c.cf)c.pc=ea;', 'BCS':'if(c.cf)c.pc=ea;', 'BNE':'if(!c.zf)c.pc=ea;', 'BEQ':'if(c.zf)c.pc=ea;',
}
RMW = {
 'ASL':'c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v);',
 'LSR':'c.cf=v&1; v=v>>1; nz(v);',
 'ROL':'t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v);',
 'ROR':'t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v);',
}
cases=[]
for entry in OPS.replace('\n','|').split('|'):
    entry=entry.strip()
    if not entry: continue
    op,mn,am,cyc=entry.split()
    code=EA[am]
    if mn in RMW:
        if am=='acc': body='v=c.a; '+RMW[mn]+' c.a=v;'
        else: body='v=rd(ea); '+RMW[mn]+' wr(ea,v);'
    else:
        body=BODY[mn]
    cases.append('case 0x%s: %s %s cyc=%s; break; // %s %s'%(op,code,body,cyc,mn,am))
js_switch='\n      '.join(cases)
core=r"""// cpu6502.js - complete documented 6502 with NMOS BCD. Generated by gen_cpu.py.
'use strict';
function CPU6502(read, write){
  const c={a:0,x:0,y:0,sp:0xFF,pc:0,nf:0,vf:0,df:0,if_:1,zf:0,cf:0,
           nmiLine:0,nmiPrev:0,irqLine:0,jam:false,cycles:0};
  const rd=read, wr=write;
  function nz(v){ c.zf=v===0?1:0; c.nf=(v>>7)&1; }
  function push(v){ wr(0x100|c.sp, v&0xFF); c.sp=(c.sp-1)&0xFF; }
  function pop(){ c.sp=(c.sp+1)&0xFF; return rd(0x100|c.sp); }
  function packP(b){ return (c.nf<<7)|(c.vf<<6)|0x20|(b?0x10:0)|(c.df<<3)|(c.if_<<2)|(c.zf<<1)|c.cf; }
  function unpackP(p){ c.nf=(p>>7)&1; c.vf=(p>>6)&1; c.df=(p>>3)&1; c.if_=(p>>2)&1; c.zf=(p>>1)&1; c.cf=p&1; }
  function adc(v){
    if(!c.df){ const t=c.a+v+c.cf;
      c.vf=(~(c.a^v)&(c.a^t)&0x80)?1:0; c.cf=t>0xFF?1:0; c.a=t&0xFF; nz(c.a);
    } else { // NMOS decimal ADC
      let lo=(c.a&0x0F)+(v&0x0F)+c.cf;
      let hi=(c.a>>4)+(v>>4);
      c.zf=((c.a+v+c.cf)&0xFF)===0?1:0;
      if(lo>9){ lo+=6; hi++; }
      c.nf=(hi&0x08)?1:0;
      c.vf=(~(c.a^v)&(c.a^(hi<<4))&0x80)?1:0;
      if(hi>9) hi+=6;
      c.cf=hi>15?1:0;
      c.a=((hi<<4)|(lo&0x0F))&0xFF;
    }
  }
  function sbc(v){
    if(!c.df){ adc(v^0xFF); }
    else { // NMOS decimal SBC: flags from binary, result decimal
      const bt=c.a-v-(1-c.cf);
      let lo=(c.a&0x0F)-(v&0x0F)-(1-c.cf);
      let hi=(c.a>>4)-(v>>4);
      if(lo&0x10){ lo-=6; hi--; }
      if(hi&0x10) hi-=6;
      c.vf=((c.a^v)&(c.a^bt)&0x80)?1:0;
      c.cf=bt>=0?1:0; nz(bt&0xFF);
      c.a=((hi<<4)|(lo&0x0F))&0xFF;
    }
  }
  function service(){
    if(c.nmiLine && !c.nmiPrev){ c.nmiPrev=1;
      push(c.pc>>8); push(c.pc&0xFF); push(packP(0)); c.if_=1;
      c.pc=rd(0xFFFA)|(rd(0xFFFB)<<8); c.cycles+=7; return true; }
    c.nmiPrev=c.nmiLine;
    if(c.irqLine && !c.if_){
      push(c.pc>>8); push(c.pc&0xFF); push(packP(0)); c.if_=1;
      c.pc=rd(0xFFFE)|(rd(0xFFFF)<<8); c.cycles+=7; return true; }
    return false;
  }
  c.reset=function(){ c.sp=0xFD; c.if_=1; c.df=0; c.nmiLine=0; c.nmiPrev=0; c.jam=false;
    c.pc=rd(0xFFFC)|(rd(0xFFFD)<<8); };
  c.step=function(){
    service();
    if(c.jam) { c.cycles+=2; return; }
    const op=rd(c.pc); c.pc=(c.pc+1)&0xFFFF;
    let ea=0,t=0,v=0,cyc=2;
    switch(op){
      %SWITCH%
      default:
        // undocumented: common NOPs consume operands; JAM opcodes halt
        if(op===0x02||op===0x12||op===0x22||op===0x32||op===0x42||op===0x52||
           op===0x62||op===0x72||op===0x92||op===0xB2||op===0xD2||op===0xF2){ c.jam=true; c.jamOp=op; c.jamPc=(c.pc-1)&0xFFFF; }
        else if(op===0x80||op===0x82||op===0x89||op===0xC2||op===0xE2||op===0x04||op===0x44||op===0x64||
                op===0x14||op===0x34||op===0x54||op===0x74||op===0xD4||op===0xF4){ c.pc=(c.pc+1)&0xFFFF; cyc=3; }
        else if(op===0x0C||op===0x1C||op===0x3C||op===0x5C||op===0x7C||op===0xDC||op===0xFC){ c.pc=(c.pc+2)&0xFFFF; cyc=4; }
        else cyc=2; // 1-byte undocumented NOPs
        break;
    }
    c.cycles+=cyc;
  };
  return c;
}
if(typeof module!=='undefined') module.exports=CPU6502;
"""
open('cpu.js','w').write(core.replace('%SWITCH%',js_switch))
print('cpu.js generated, opcodes:',len(cases))

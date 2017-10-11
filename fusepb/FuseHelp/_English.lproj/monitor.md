---
title: Monitor/Debugger
description: This section describes the Fuse machine code monitor and debugger.
order: 150
---

Fuse features a moderately powerful, completely transparent monitor/debugger,
which can be activated via the *Machine > Debugger…* menu option. A debugger
window will appear, showing the current state of the emulated machine: the
top-left 'pane' shows the current state of the Z80 and the last bytes written to
any emulated peripherals. The bottom-left pane lists any active breakpoints.
Moving right, the next pane shows a disassembly, which by default starts at the
current program counter, although this can be modified either by the
'disassemble' command (see below) or by dragging the scrollbar next to it. The
next pane shows the current stack, and the next pane has any 'events' which are
due to occur and could affect emulation. Below the events pane is the Spectrum's
64K memory map (the W? and C? indicate whether each displayed chunk is writable
or contended respectively). Fuse tracks the memory mapping of the overall
address space in 2KB chunks but will summarise the mapped pages where they are
part of the same page of the underlying memory source (e.g. 8KB page sizes in
the Spectrum 128K and 4KB pages in the Timex clones' DOCK and EXROM banks).
Below the displays are an entry box for debugger commands, and five buttons for
controlling the debugger:

BUTTON | DESCRIPTION
:--- | :---
*Evaluate* | Evaluate the command currently in the entry box.
*Single* | Step Run precisely one Z80 opcode and then stop emulation again.
*Continue* | Restart emulation, but leave the debugger window open. Note that the debugger window will not be updated while emulation is running.
*Break* | Stop emulation and return to the debugger.
*Close* | Close the debugger window and restart emulation.

<br>
Double-clicking on an entry in the stack pane will cause emulation to run until
the program counter reaches the value stored at that address, while
double-clicking on an entry in the 'events' pane will cause emulation to run
until that time is reached.

The main power of the debugger is via the commands entered into the entry box,
which are similar in nature (but definitely not identical to or as powerful as)
to those in the gdb debugger. In general, the debugger is case-insensitive, and
numbers will be interpreted as decimal, unless prefixed by either '0x' or '$'
when they will be interpreted as hex. Each command can be abbreviated to the
portion not in curly braces.

COMMAND | DESCRIPTION
:--- | :---
*ba{se} number* | Change the debugger window to displaying output in base number. Available values are 10 (decimal) or 16 (hex).
*br{eakpoint} [address] [if condition]* | Set a breakpoint to stop emulation and return to the debugger whenever an opcode is executed at address and condition evaluates true. If address is omitted, it defaults to the current value of PC.
br{eakpoint} p{ort} (re{ad}\|w{rite}) port [if condition] | Set a breakpoint to trigger whenever IO port port is read from or written to and condition evaluates true.
br{eakpoint} (re{ad}\|w{rite}) [address] [if condition] | Set a breakpoint to trigger whenever memory location address is read from (other than via an opcode fetch) or written to and condition evaluates true. Address again defaults to the current value of PC if omitted.
*br{eakpoint} ti{me} time [if condition]* | Set a breakpoint to occur time tstates after the start of every frame, assuming condition evaluates true (if one is given).
*br{eakpoint} ev{ent} area:detail [if condition]* | Set a breakpoint to occur when the event specified by area detail occurs and condition evaluates to true. The events which can be caught are listed below.
*cl{ear} [address]* | Remove all breakpoints at address or the current value of PC if address is omitted. Port read/write breakpoints are unaffected.
*cond{ition} id [condition]* | Set breakpoint id to trigger only when condition is true, or unconditionally if condition is omitted.
*co{ntinue}* | Equivalent to the Continue button.
*del{ete} [id]* | Remove breakpoint id, or all breakpoints if id is omitted.
*di{sassemble} address* | Set the centre panel disassembly to begin at address.
*fi{nish}* | Exit from the current CALL or equivalent. This isn't infallible: it works by setting a temporary breakpoint at the current contents of the stack pointer, so will not function correctly if the code returns to some other point or plays with its stack in other ways. Also, setting this breakpoint doesn't disable other breakpoints, which may trigger before this one. In that case, the temporary breakpoint remains, and the `continue' command can be used to return to it.
*i{gnore} id count* | Do not trigger the next count times that breakpoint id would have triggered.
*n{ext}* | Step to the opcode following the current one. As with the `finish' command, this works by setting a temporary breakpoint at the next opcode, so is not infalliable.
*o{ut} port value* | Write value to IO port port.
*se{t} address value* | Poke value into memory at address.
*se{t} $variable value* | Set the value of the debugger variable variable to value.
*se{t} area:detail value* | Set the value of the system variable area : detail to value. The available system variables are listed below.
*s{tep}* | Equivalent to the Single Step button.
*t{breakpoint} [options]* | This is the same as the `breakpoint` command in its various forms, except that the breakpoint is temporary: it will trigger once and once only, and then be removed.

<br>
Addresses can be specified in one of two forms: either an absolute addresses, specifed by an integer in the range 0x0000 to 0xFFFF or as a 'source:page:offset' combination, which refers to a location offset bytes into the memory bank page, independent of where that bank is currently paged into memory. RAM and ROM pages are indicated, respectively, by 'RAM' and 'ROM' sources (e.g. offset 0x1234 in ROM 1 is specified as `ROM:1:0x1234`). Other available sources are: 'Betadisk', '"Didaktik 80 RAM"', '"Didaktik 80 ROM"', '"DISCiPLE RAM"', '"DISCiPLE ROM"', '"DivIDE EPROM"', '"DivIDE RAM"', '"DivMMC EPROM"', '"DivMMC RAM"', 'If1', 'If2', '"Multiface RAM"', '"Multiface ROM"', '"Opus RAM"', '"Opus ROM"', 'PlusD RAM', 'PlusD ROM', 'SpeccyBoot', 'Spectranet', '"Timex Dock"', '"Timex EXROM"', 'ZXATASP' and 'ZXCF'.

Please, note that spaces in memory sources should be escaped, e.g.,`break Didaktik\ 80\ ROM:0:0x1234`. The 48K machines are treated as having a permanent mapping of page 5 at 0x4000, page 2 at 0x8000 and page 0 at 0xC000; the 16K Spectrum is treated as having page 5 at 0x4000 and no page at 0x8000 and 0xC000.

Anywhere the debugger is expecting a numeric value, except where it expects a breakpoint id, you can instead use a numeric expression, which uses a restricted version of C's syntax; exactly the same syntax is used for conditional breakpoints, with '0' being false and any other value being true. In numeric expressions, you can use integer constants (all calculations are done in integers), system variables, debugger variables, parentheses, the standard four numeric operations ('+', '-', '\*' and '/'), the (non-)equality operators '==' and '!=', the comparision operators '>', '<', '>=' and '<=', bitwise and ('&'), or ('\|') and exclusive or ('^') and logical and ('&&') and or ('\|\|').

## System Events

EVENT | DESCRIPTION
:--- | :---
*beta128:page* | The Beta 128 interface is paged into memory.
*beta128:unpage* | The Beta 128 interface is paged out of memory.
*didaktik80:page* | The Didaktik 80 interface is paged into memory.
*didaktik80:unpage* | The Didaktik 80 interface is paged out of memory.
*disciple:page* | The DISCiPLE interface is paged into memory.
*disciple:unpage* | The DISCiPLE interface is paged out of memory.
*divide:page* | The DivIDE interface is paged into memory.
*divide:unpage* | The DivIDE interface is paged out of memory.
*divmmc:page* | The DivMMC interface is paged into memory.
*divmmc:unpage* | The DivMMC interface is paged out of memory.
*if1:page* | The Interface 1 shadow ROM is paged into memory.
*if1:unpage* | The Interface 1 shadow ROM is paged out of memory.  
*multiface:page* | The Multiface One/128/3 is paged into memory.
*multiface:unpage* | The Multiface One/128/3 is paged out of memory.
*opus:page* | The Opus Discovery is paged into memory.
*opus:unpage* | The Opus Discovery is paged out of memory.
*plusd:page* | The +D interface is paged into memory.
*plusd:unpage* | The +D interface is paged into memory.  
*speccyboot:page* | The SpeccyBoot interface is paged into memory.
*speccyboot:unpage* | The SpeccyBoot interface is paged out of memory.
*spectranet:page* | The Spectranet interface is paged into memory.
*spectranet:unpage* | The Spectranet interface is paged out of memory.
*rzx:end* | An RZX recording finishes playing.
*tape:play* | The emulated tape starts playing.
*tape:stop* | The emulated tape stops playing.
*zxatasp:page* | The ZXATASP interface is paged into memory.
*zxatasp:unpage* | The ZXATASP interface is paged out of memory.
*zxcf:page* | The ZXCF interface is paged into of memory.
*zxcf:unpage* | The ZXCF interface is paged out of memory.

<br>
In all cases, the event can be specified as area:\* to catch all events from
that area.

## System Variables

System variables are specified via an ‘area:detail’ syntax. The available system variables are:

SYSTEM VARIABLE | DESCRIPTION
:--- | :---
*ay:current* | The current AY‐3‐8912 register.
*divmmc:control* | The last byte written to DivMMC control port.
*spectrum:frames* | The frame count since reset. Note that this variable can only be read, not written to.
*tape:microphone* | The current level of the tape input connected to the 'EAR' port. Note that this variable can only be read, not written to.
*ula:last* | The last byte written to the ULA. Note that this variable can only be read, not written to.
*ula:mem1ffd* | The last byte written to memory control port used by the ZX Spectrum +2A/3; normally addressed at 0x1ffd, hence the name.
*ula:mem7ffd* | The last byte written to primary memory control port used by the ZX Spectrum 128 and later; normally addressed at 0x7ffd, hence the name.
*ula:tstates* | The number of tstates since the last interrupt.
*z80: register name* | The value of the specified register. Both 8‐bit registers and 16‐bit register pairs are supported. The MEMPTR / WZ hidden register is also supported.
*z80:im* | The current interrupt mode of the Z80.
*z80:iff1* | 1 if the interrupt flip‐flop is currently set, or 0 if it is not set.
*z80:iff2* | 1 if the interrupt flip‐flop is currently set, or 0 if it is not set.

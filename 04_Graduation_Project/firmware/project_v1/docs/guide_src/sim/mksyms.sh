#!/bin/sh
# Regenerate syms.h from the ELF: the harness needs the addresses of the three ISRs,
# of SCHED_u8IsTaskDue (slot start) and of the two SCHED globals.
ELF=${1:-app.elf}
a() { avr-nm "$ELF" | awk -v s="$1" '$3==s{print "0x"$1; exit}'; }
cat <<H
#define A_VEC4 $(a __vector_4)
#define A_VEC13 $(a __vector_13)
#define A_VEC14 $(a __vector_14)
#define A_ISDUE $(a SCHED_u8IsTaskDue.part.0)
#define A_FLAGS ($(a Global_u8Flags) - 0x800000)
#define A_OVR ($(a Global_u16Overruns) - 0x800000)
H

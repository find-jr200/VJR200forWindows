// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*** m6800: Portable 6800 class  emulator *************************************

    m68xx.c

    References:

        6809 Simulator V09, By L.C. Benschop, Eindhoven The Netherlands.

        m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
            the 6809 Simulator V09)

        6809 Microcomputer Programming & Interfacing with Experiments"
            by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

    System dependencies:    uint16_t must be 16 bit unsigned int
                            uint8_t must be 8 bit unsigned int
                            uint32_t must be more than 16 bits
                            arrays up to 65536 bytes must be supported
                            machine must be twos complement

History
991031  ZV
    Added NSC-8105 support

990319  HJB
    Fixed wrong LSB/MSB order for push/pull word.
    Subtract .extra_cycles at the beginning/end of the exectuion loops.

990316  HJB
    Renamed to 6800, since that's the basic CPU.
    Added different cycle count tables for M6800/2/8, M6801/3 and m68xx.

990314  HJB
    Also added the M6800 subtype.

990311  HJB
    Added _info functions. Now uses static m6808_Regs struct instead
    of single statics. Changed the 16 bit registers to use the generic
    PAIR union. Registers defined using macros. Split the core into
    four execution loops for M6802, M6803, M6808 and HD63701.
    TST, TSTA and TSTB opcodes reset carry flag.
TODO:
    Verify invalid opcodes for the different CPU types.
    Add proper credits to _info functions.
    Integrate m6808_Flags into the registers (multiple m6808 type CPUs?)

990301  HJB
    Modified the interrupt handling. No more pending interrupt checks.
    WAI opcode saves state, when an interrupt is taken (IRQ or OCI),
    the state is only saved if not already done by WAI.

*****************************************************************************/

/*

    Chip                RAM     NVRAM   ROM     SCI     r15-f   ports
    -----------------------------------------------------------------
    MC6800              -       -       -       no      no      4
    MC6802              128     32      -       no      no      4
    MC6802NS            128     -       -       no      no      4
    MC6808              -       -       -       no      no      4

    MC6801              128     64      2K      yes     no      4
    MC68701             128     64      -       yes     no      4
    MC6803              128     64      -       yes     no      4

    MC6801U4            192     32      4K      yes     yes     4
    MC6803U4            192     32      -       yes     yes     4

    HD6801              128     64      2K      yes     no      4
    HD6301V             128     -       4K      yes     no      4
    HD63701V            192     -       4K      yes     no      4
    HD6303R             128     -       -       yes     no      4

    HD6301X             192     -       4K      yes     yes     6
    HD6301Y             256     -       16K     yes     yes     6
    HD6303X             192     -       -       yes     yes     6
    HD6303Y             256     -       -       yes     yes     6

    NSC8105
    MS2010-A

*/
#include "stdafx.h"
#include "JRSystem.h"
#include "m6800.h"
#include "VJR200.h"

#define VERBOSE 0
#define LOG(x)  do { if (VERBOSE) logerror x; } while (0)

extern JRSystem sys;

CEREAL_CLASS_VERSION(m6800_cpu_device, CEREAL_VER);

#if 0
/* CPU subtypes, needed for extra insn after TAP/CLI/SEI */
enum
{
	SUBTYPE_M6800,
	SUBTYPE_M6801,
	SUBTYPE_M6802,
	SUBTYPE_M6803,
	SUBTYPE_M6808,
	SUBTYPE_HD6301,
	SUBTYPE_HD63701,
	SUBTYPE_NSC8105
};
#endif


#if 0
static void hd63701_trap_pc();
#endif

#define pPPC    m_ppc
#define pPC     m_pc
#define pS      m_s
#define pX      m_x
#define pD      m_d

#define PC      m_pc.w.l
#define PCD     m_pc.d
#define S       m_s.w.l
#define SD      m_s.d
#define X       m_x.w.l
#define D       m_d.w.l
#define A       m_d.b.h
#define B       m_d.b.l
#define CC      m_cc

#define CT      m_counter.w.l
#define CTH     m_counter.w.h
#define CTD     m_counter.d
#define OC      m_output_compare.w.l
#define OCH     m_output_compare.w.h
#define OCD     m_output_compare.d
#define TOH     m_timer_over.w.l
#define TOD     m_timer_over.d

#define EAD     m_ea.d
#define EA      m_ea.w.l

/* point of next timer event */
static uint32_t timer_next;

/* memory interface */

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
#define RM(Addr) ((unsigned)m_program->ReadByte(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define WM(Addr,Value) (m_program->WriteByte(Addr,Value))

/****************************************************************************/
/* M6800_RDOP() is identical to M6800_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M_RDOP(Addr) ((unsigned)m_decrypted_opcodes_direct->ReadByte(Addr))

/****************************************************************************/
/* M6800_RDOP_ARG() is identical to M6800_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M_RDOP_ARG(Addr) ((unsigned)m_direct->ReadByte(Addr))

/* macros to access memory */
#define IMMBYTE(b)  b = M_RDOP_ARG(PCD); PC++
#define IMMWORD(w)  w.d = (M_RDOP_ARG(PCD)<<8) | M_RDOP_ARG((PCD+1)&0xffff); PC+=2

#define PUSHBYTE(b) WM(SD,b); --S
#define PUSHWORD(w) WM(SD,w.b.l); --S; WM(SD,w.b.h); --S
#define PULLBYTE(b) S++; b = RM(SD)
#define PULLWORD(w) S++; w.d = RM(SD)<<8; S++; w.d |= RM(SD)

#define MODIFIED_tcsr { \
	m_irq2 = (m_tcsr&(m_tcsr<<3))&(TCSR_ICF|TCSR_OCF|TCSR_TOF); \
}

#define SET_TIMER_EVENT {                   \
	timer_next = (OCD - CTD < TOD - CTD) ? OCD : TOD;   \
}

/* cleanup high-word of counters */
#define CLEANUP_COUNTERS() {                        \
	OCH -= CTH;                                 \
	TOH -= CTH;                                 \
	CTH = 0;                                    \
	SET_TIMER_EVENT;                            \
}

/* when change freerunningcounter or outputcapture */
#define MODIFIED_counters {                     \
	OCH = (OC >= CT) ? CTH : CTH+1;             \
	SET_TIMER_EVENT;                            \
}

// I/O registers

enum
{
    IO_P1DDR = 0,
    IO_P2DDR,
    IO_P1DATA,
    IO_P2DATA,
    IO_P3DDR,
    IO_P4DDR,
    IO_P3DATA,
    IO_P4DATA,
    IO_TCSR,
    IO_CH,
    IO_CL,
    IO_OCRH,
    IO_OCRL,
    IO_ICRH,
    IO_ICRL,
    IO_P3CSR,
    IO_RMCR,
    IO_TRCSR,
    IO_RDR,
    IO_TDR,
    IO_RCR,
    IO_CAAH,
    IO_CAAL,
    IO_TCR1,
    IO_TCR2,
    IO_TSR,
    IO_OCR2H,
    IO_OCR2L,
    IO_OCR3H,
    IO_OCR3L,
    IO_ICR2H,
    IO_ICR2L
};

// serial I/O

#define M6800_RMCR_SS_MASK      0x03 // Speed Select
#define M6800_RMCR_SS_4096      0x03 // E / 4096
#define M6800_RMCR_SS_1024      0x02 // E / 1024
#define M6800_RMCR_SS_128       0x01 // E / 128
#define M6800_RMCR_SS_16        0x00 // E / 16
#define M6800_RMCR_CC_MASK      0x0c // Clock Control/Format Select

#define M6800_TRCSR_RDRF        0x80 // Receive Data Register Full
#define M6800_TRCSR_ORFE        0x40 // Over Run Framing Error
#define M6800_TRCSR_TDRE        0x20 // Transmit Data Register Empty
#define M6800_TRCSR_RIE         0x10 // Receive Interrupt Enable
#define M6800_TRCSR_RE          0x08 // Receive Enable
#define M6800_TRCSR_TIE         0x04 // Transmit Interrupt Enable
#define M6800_TRCSR_TE          0x02 // Transmit Enable
#define M6800_TRCSR_WU          0x01 // Wake Up

#define M6800_PORT2_IO4         0x10
#define M6800_PORT2_IO3         0x08

#define M6801_P3CSR_LE          0x08
#define M6801_P3CSR_OSS         0x10
#define M6801_P3CSR_IS3_ENABLE  0x40
#define M6801_P3CSR_IS3_FLAG    0x80

static const int M6800_RMCR_SS[] = { 16, 128, 1024, 4096 };

#define M6800_SERIAL_START      0
#define M6800_SERIAL_STOP       9

enum
{
    M6800_TX_STATE_INIT = 0,
    M6800_TX_STATE_READY
};

/* operate one instruction for */
#define ONE_MORE_INSN() {       \
	uint8_t ireg;                             \
	pPPC = pPC;                             \
	ireg=M_RDOP(PCD);                       \
	PC++;                                   \
	(this->*m_insn[ireg])();               \
	increment_counter(m_cycles[ireg]);    \
}

/* CC masks                       HI NZVC
                                7654 3210   */
#define CLR_HNZVC   CC&=0xd0
#define CLR_NZV     CC&=0xf1
#define CLR_HNZC    CC&=0xd2
#define CLR_NZVC    CC&=0xf0
#define CLR_Z       CC&=0xfb
#define CLR_NZC     CC&=0xf2
#define CLR_ZC      CC&=0xfa
#define CLR_C       CC&=0xfe

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)        if(!(a))SEZ
#define SET_Z8(a)       SET_Z((uint8_t)(a))
#define SET_Z16(a)      SET_Z((uint16_t)(a))
#define SET_N8(a)       CC|=(((a)&0x80)>>4)
#define SET_N16(a)      CC|=(((a)&0x8000)>>12)
#define SET_H(a,b,r)    CC|=((((a)^(b)^(r))&0x10)<<1)
#define SET_C8(a)       CC|=(((a)&0x100)>>8)
#define SET_C16(a)      CC|=(((a)&0x10000)>>16)
#define SET_V8(a,b,r)   CC|=((((a)^(b)^(r)^((r)>>1))&0x80)>>6)
#define SET_V16(a,b,r)  CC|=((((a)^(b)^(r)^((r)>>1))&0x8000)>>14)

const uint8_t m6800_cpu_device::flags8i[256]=     /* increment */
        {
                0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
        };


const uint8_t m6800_cpu_device::flags8d[256]= /* decrement */
        {
                0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
                0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
        };

#define SET_FLAGS8I(a)      {CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)      {CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)          {SET_N8(a);SET_Z8(a);}
#define SET_NZ16(a)         {SET_N16(a);SET_Z16(a);}
#define SET_FLAGS8(a,b,r)   {SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)  {SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an uint8_t as a signed int16_t */
#define SIGNED(b) ((int16_t)(b&0x80?b|0xff00:b))

/* Macros for addressing modes */
#define DIRECT IMMBYTE(EAD)
#define IMM8 EA=PC++
#define IMM16 {EA=PC;PC+=2;}
#define EXTENDED IMMWORD(m_ea)
#define INDEXED {EA=X+(uint8_t)M_RDOP_ARG(PCD);PC++;}

/* macros to set status flags */
#if defined(SEC)
#undef SEC
#endif
#define SEC CC|=0x01
#define CLC CC&=0xfe
#define SEZ CC|=0x04
#define CLZ CC&=0xfb
#define SEN CC|=0x08
#define CLN CC&=0xf7
#define SEV CC|=0x02
#define CLV CC&=0xfd
#define SEH CC|=0x20
#define CLH CC&=0xdf
#define SEI CC|=0x10
#define CLI CC&=~0x10

/* mnemonicos for the Timer Control and Status Register bits */
#define TCSR_OLVL 0x01
#define TCSR_IEDG 0x02
#define TCSR_ETOI 0x04
#define TCSR_EOCI 0x08
#define TCSR_EICI 0x10
#define TCSR_TOF  0x20
#define TCSR_OCF  0x40
#define TCSR_ICF  0x80

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;w.d=RM16(EAD);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;w.d=RM16(EAD);}

#define IDXBYTE(b) {INDEXED;b=RM(EAD);}
#define IDXWORD(w) {INDEXED;w.d=RM16(EAD);}

/* Macros for branch instructions */
#define BRANCH(f) {IMMBYTE(t);if(f){PC+=SIGNED(t);}}
#define NXORV  ((CC&0x08)^((CC&0x02)<<2))

#define M6800_WAI       8           /* set when WAI is waiting for an interrupt */
//#define M6800_SLP       0x10        /* HD63701 only */

/* Note: don't use 0 cycles here for invalid opcodes so that we don't */
/* hang in an infinite loop if we hit one */
#define XX 5 // invalid opcode unknown cc
const uint8_t m6800_cpu_device::cycles_6800[256] =
        {
                    /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
                /*0*/ XX, 2,XX,XX,XX,XX, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,
                /*1*/  2, 2,XX,XX,XX,XX, 2, 2, 2, 2, 2, 2,XX,XX,XX,XX,
                /*2*/  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                /*3*/  4, 4, 4, 4, 4, 4, 4, 4,XX, 5,10,10,XX,XX, 9,12,
                /*4*/  2,XX, 2, 2, 2,XX, 2, 2, 2, 2, 2, 2, 2, 2,XX, 2,
                /*5*/  2,XX, 2, 2, 2,XX, 2, 2, 2, 2, 2, 2, 2, 2,XX, 2,
                /*6*/  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 7,
                /*7*/  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 6,
                /*8*/  2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 3, 8, 3, 5,
                /*9*/  3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 4, 6, 4, 5,
                /*A*/  5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,
                /*B*/  4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 5, 9, 5, 6,
                /*C*/  2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,XX,XX, 3, 5,
                /*D*/  3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3,XX,XX, 4, 5,
                /*E*/  5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5,XX, 8, 6, 7,
                /*F*/  4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4,XX, 9, 5, 6
        };

#undef XX // /invalid opcode unknown cc

#define EAT_CYCLES                                                  \
{                                                                   \
	int cycles_to_eat;                                              \
																	\
	cycles_to_eat = timer_next - CTD;                               \
	if( cycles_to_eat > m_icount) cycles_to_eat = m_icount; \
	if (cycles_to_eat > 0)                                          \
	{                                                               \
		increment_counter(cycles_to_eat);                         \
	}                                                               \
}


m6800_cpu_device::m6800_cpu_device()
        : m_has_io(false)
        , m_insn(m6800_insn)
        , m_cycles(cycles_6800)
{
    m_clock_divider = 1;

    address = sys.pAddress;
}

uint16_t m6800_cpu_device::RM16(uint16_t Addr )
{
    uint16_t result = RM(Addr) << 8;
    return result | RM((Addr+1)&0xffff);
}

void m6800_cpu_device::WM16(uint16_t Addr, PAIR *p )
{
    WM( Addr, p->b.h );
    WM( (Addr+1)&0xffff, p->b.l );
}

/* IRQ enter */
void m6800_cpu_device::enter_interrupt(const char *message,uint16_t irq_vector)
{
    if( m_wai_state & M6800_WAI )
    {
        m_icount -= 4;
        m_wai_state &= ~M6800_WAI;
    }
    else
    {
        PUSHWORD(pPC);
        PUSHWORD(pX);
        PUSHBYTE(A);
        PUSHBYTE(B);
        PUSHBYTE(CC);
        m_icount -= 26; // JR-200

    }
    SEI;
    PCD = RM16( irq_vector );
}

void m6800_cpu_device::GetRegister(uint16_t & pc, uint16_t & sp, uint16_t & x, uint8_t & a, uint8_t & b, uint8_t & cc)
{
    pc = m_pc.d; sp = m_s.d; x = pX.d, a = A, b = B; cc = m_cc;
}


void m6800_cpu_device::irq()
{
    execute_set_input(INPUT_LINE_IRQ0, ASSERT_LINE);
}

void m6800_cpu_device::nmi()
{
    execute_set_input(INPUT_LINE_NMI, ASSERT_LINE);
}


/* check the IRQ lines for pending interrupts */
void m6800_cpu_device::CHECK_IRQ_LINES()
{
    // TODO: IS3 interrupt

    if (m_nmi_pending)
    {
        m_nmi_pending = false;
        enter_interrupt("M6800 '%s' take NMI\n",0xfffc);
        m_nmi_state = CLEAR_LINE;

    }
    else
    {
        if (m_irq_state[M6800_IRQ_LINE] != CLEAR_LINE)
        {   /* standard IRQ */
            if (!(CC & 0x10))
            {
                enter_interrupt("M6800 '%s' take IRQ1\n", 0xfff8);
                //standard_irq_callback(M6800_IRQ_LINE);
                m_irq_state[M6800_IRQ_LINE] = CLEAR_LINE;
            }
        }
    }
}


void m6800_cpu_device::increment_counter(int amount)
{
    m_icount -= amount;
    CTD += amount;
}


/* include the opcode prototypes and function pointer tables */
#include "6800tbl.hxx"

///* include the opcode functions */
#include "6800ops.hxx"



void m6800_cpu_device::device_start()
{
    m_program = address;
    m_direct = address;
    m_decrypted_opcodes = address;
    m_decrypted_opcodes_direct = address;

    m_input_capture = 0;
    m_rdr = 0;
    m_tdr = 0;
    m_rmcr = 0;
    m_ram_ctrl = 0;

    m_pc.d = 0;
    m_s.d = 0;
    m_x.d = 0;
    m_d.d = 0;
    m_cc = 0;
    m_wai_state = 0;
    m_irq_state[0] = m_irq_state[1] = m_irq_state[2] = 0;

    m_icount = 0;
}


void m6800_cpu_device::device_reset()
{
    m_cc = 0xc0;
    SEI;                /* IRQ disabled */
    PCD = RM16( 0xfffe );

    m_wai_state = 0;
    m_nmi_state = 0;
    m_nmi_pending = 0;
    m_sc1_state = 0;
    m_irq_state[M6800_IRQ_LINE] = 0;
    m_ic_eddge = 0;

    CTD = 0x0000;
    OCD = 0xffff;
    TOD = 0xffff;
    m_ram_ctrl |= 0x40;
    m_latch09 = 0;

    m_trcsr = M6800_TRCSR_TDRE;

    m_txstate = M6800_TX_STATE_INIT;
    m_txbits = m_rxbits = 0;
    m_tx = 1;
    m_trcsr_read_tdre = 0;
    m_trcsr_read_orfe = 0;
    m_trcsr_read_rdrf = 0;
    m_ext_serclock = 0;
    m_use_ext_serclock = false;
}


void m6800_cpu_device::execute_set_input(int irqline, int state)
{
    switch (irqline)
    {
        case INPUT_LINE_NMI:
            if (!m_nmi_state && state != CLEAR_LINE)
                m_nmi_pending = true;
            m_nmi_state = state;
            break;

        default:
            m_irq_state[irqline] = state;
    }
}

int m6800_cpu_device::run(int steps)
{
    uint8_t ireg;
    int c = 0;

    m_icount = steps;
    CLEANUP_COUNTERS();

    do
    {
        if (m_wai_state & M6800_WAI)
        {
            // EAT_CYCLES;
            c = 10;
        }
        else
        {
			g_dramWait = 0;
			pPPC = pPC;
			ireg = M_RDOP(PCD);
			PC++;
            (this->*m_insn[ireg])();
            c = m_cycles[ireg] + g_dramWait;
        }

        increment_counter(c);
        // 周辺チップのカウンタの操作
        sys.pMn1271->TickTimerCounter(c);
        sys.pCrtc->TickCounter(c);
        sys.pMn1544->TickCounter(c);

        CHECK_IRQ_LINES();

		for (int i = 0; i < BREAKPOINT_NUM; ++i) {
			if (PCD == g_breakPoint[i]) { // ブレークポイントチェック
				g_debug = 0;
			}
		}

		if (g_debug != -1) {
			SetWatchList();
			break;
		}


    } while (m_icount>0);

    return c;
}


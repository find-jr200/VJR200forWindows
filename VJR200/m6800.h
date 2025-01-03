// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*** m6800: Portable 6800 class emulator *************************************/
#ifndef __M6800_H__
#define __M6800_H__

#include "emu.h"
#include "Address.h"

enum
{
    M6800_PC=1, M6800_S, M6800_A, M6800_B, M6800_X, M6800_CC,
    M6800_WAI_STATE
};

// I/O line states
enum line_state
{
    CLEAR_LINE = 0,             // clear (a fired or held) line
    ASSERT_LINE,                // assert an interrupt immediately
    HOLD_LINE,                  // hold interrupt line until acknowledged
    PULSE_LINE                  // pulse interrupt line instantaneously (only for NMI, RESET)
};

enum
{
    M6800_IRQ_LINE = 0,             /* IRQ line number */
    M6801_TIN_LINE,                 /* P20/Tin Input Capture line (eddge sense)     */
    /* Active eddge is selecrable by internal reg.  */
    /* raise eddge : CLEAR_LINE  -> ASSERT_LINE     */
    /* fall  eddge : ASSERT_LINE -> CLEAR_LINE      */
    /* it is usuali to use PULSE_LINE state         */
            M6801_SC1_LINE
};

enum
{
    // input lines
            MAX_INPUT_LINES = 32 + 3,
    INPUT_LINE_IRQ0 = 0,
    INPUT_LINE_IRQ1 = 1,
    INPUT_LINE_IRQ2 = 2,
    INPUT_LINE_IRQ3 = 3,
    INPUT_LINE_IRQ4 = 4,
    INPUT_LINE_IRQ5 = 5,
    INPUT_LINE_IRQ6 = 6,
    INPUT_LINE_IRQ7 = 7,
    INPUT_LINE_IRQ8 = 8,
    INPUT_LINE_IRQ9 = 9,
    INPUT_LINE_NMI = MAX_INPUT_LINES - 3,

    // special input lines that are implemented in the core
            INPUT_LINE_RESET = MAX_INPUT_LINES - 2,
    INPUT_LINE_HALT = MAX_INPUT_LINES - 1
};



enum
{
    M6802_IRQ_LINE = M6800_IRQ_LINE
};


class m6800_cpu_device
{
public:
    typedef void (m6800_cpu_device::*op_func)();

    // construction/destruction
    m6800_cpu_device();
    virtual int run(int steps);

    virtual void device_start();
    virtual void device_reset();
    void irq();
    void nmi();
    void GetRegister(uint16_t& pc, uint16_t& sp, uint16_t& x, uint8_t& a, uint8_t& b, uint8_t& cc, uint16_t & ppc);
    void CHECK_IRQ_LINES();

    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version);

protected:
    // device_execute_interface overrides
    virtual uint16_t execute_min_cycles() const { return 1; }
    virtual uint16_t execute_max_cycles() const { return 12; }
    virtual uint16_t execute_input_lines() const { return 2; }
    virtual uint16_t execute_default_irq_vector() const{ return 0; }
    virtual void execute_set_input(int inputnum, int state);
    void enter_interrupt(const char *message, uint16_t irq_vector);
    bool m_has_io;
    PAIR    m_ppc;            /* Previous program dCounter */
    PAIR    m_pc;             /* Program dCounter */
    PAIR    m_s;              /* Stack pointer */
    PAIR    m_x;              /* Index register */
    PAIR    m_d;              /* Accumulators */
    uint8_t   m_cc;             /* Condition codes */
    uint8_t   m_wai_state;      /* WAI opcode state ,(or sleep opcode state) */
    uint8_t   m_nmi_state;      /* NMI line state */
    uint8_t   m_nmi_pending;    /* NMI pending */
    uint8_t   m_irq_state[3];   /* IRQ line state [IRQ1,TIN,SC1] */
    uint8_t   m_ic_eddge;       /* InputCapture eddge , b.0=fall,b.1=raise */
    int     m_sc1_state;

    /* Memory spaces */
    Address* address;
    Address *m_program, *m_decrypted_opcodes;
    Address *m_direct, *m_decrypted_opcodes_direct;
    Address *m_io;

    const op_func *m_insn;
    const uint8_t *m_cycles;            /* clock cycle of instruction table */
    /* internal registers */
    uint8_t   m_ram_ctrl;
    PAIR    m_counter;        /* free running dCounter */
    PAIR    m_output_compare; /* output compare       */
    uint16_t  m_input_capture;  /* input capture        */
    int     m_p3csr_is3_flag_read;
    int     m_port3_latched;
    int     m_clock_divider;
    uint8_t   m_trcsr, m_rmcr, m_rdr, m_tdr, m_rsr, m_tsr;
    int     m_rxbits, m_txbits, m_txstate, m_trcsr_read_tdre, m_trcsr_read_orfe, m_trcsr_read_rdrf, m_tx, m_ext_serclock;
    bool    m_use_ext_serclock;
    int     m_port2_written;
    int     m_icount;
    int     m_latch09;

    PAIR    m_timer_over;
    PAIR m_ea;        /* effective address */

    static const uint8_t flags8i[256];
    static const uint8_t flags8d[256];
    static const uint8_t cycles_6800[256];
    static const uint8_t cycles_6800D[256];
    static const op_func m6800_insn[256];

    uint16_t RM16(uint16_t Addr );
    void WM16(uint16_t Addr, PAIR *p );
    void increment_counter(int amount);

    void aba();
    void abx();
    void adca_di();
    void adca_ex();
    void adca_im();
    void adca_ix();
    void adcb_di();
    void adcb_ex();
    void adcb_im();
    void adcb_ix();
    void adcx_im();
    void adda_di();
    void adda_ex();
    void adda_im();
    void adda_ix();
    void addb_di();
    void addb_ex();
    void addb_im();
    void addb_ix();
    void addd_di();
    void addd_ex();
    void addx_ex();
    void addd_im();
    void addd_ix();
    void aim_di();
    void aim_ix();
    void anda_di();
    void anda_ex();
    void anda_im();
    void anda_ix();
    void andb_di();
    void andb_ex();
    void andb_im();
    void andb_ix();
    void asl_ex();
    void asl_ix();
    void asla();
    void aslb();
    void asld();
    void asr_ex();
    void asr_ix();
    void asra();
    void asrb();
    void bcc();
    void bcs();
    void beq();
    void bge();
    void bgt();
    void bhi();
    void bita_di();
    void bita_ex();
    void bita_im();
    void bita_ix();
    void bitb_di();
    void bitb_ex();
    void bitb_im();
    void bitb_ix();
    void ble();
    void bls();
    void blt();
    void bmi();
    void bne();
    void bpl();
    void bra();
    void brn();
    void bsr();
    void bvc();
    void bvs();
    void cba();
    void clc();
    void cli();
    void clr_ex();
    void clr_ix();
    void clra();
    void clrb();
    void clv();
    void cmpa_di();
    void cmpa_ex();
    void cmpa_im();
    void cmpa_ix();
    void cmpb_di();
    void cmpb_ex();
    void cmpb_im();
    void cmpb_ix();
    void cmpx_di();
    void cmpx_ex();
    void cmpx_im();
    void cmpx_ix();
    void com_ex();
    void com_ix();
    void coma();
    void comb();
    void daa();
    void dec_ex();
    void dec_ix();
    void deca();
    void decb();
    void des();
    void dex();
    void eim_di();
    void eim_ix();
    void eora_di();
    void eora_ex();
    void eora_im();
    void eora_ix();
    void eorb_di();
    void eorb_ex();
    void eorb_im();
    void eorb_ix();
    void illegal();
    void inc_ex();
    void inc_ix();
    void inca();
    void incb();
    void ins();
    void inx();
    void jmp_ex();
    void jmp_ix();
    void jsr_di();
    void jsr_ex();
    void jsr_ix();
    void lda_di();
    void lda_ex();
    void lda_im();
    void lda_ix();
    void ldb_di();
    void ldb_ex();
    void ldb_im();
    void ldb_ix();
    void ldd_di();
    void ldd_ex();
    void ldd_im();
    void ldd_ix();
    void lds_di();
    void lds_ex();
    void lds_im();
    void lds_ix();
    void ldx_di();
    void ldx_ex();
    void ldx_im();
    void ldx_ix();
    void lsr_ex();
    void lsr_ix();
    void lsra();
    void lsrb();
    void lsrd();
    void mul();
    void nba();
    void neg_ex();
    void neg_ix();
    void nega();
    void negb();
    void nop();
    void oim_di();
    void oim_ix();
    void ora_di();
    void ora_ex();
    void ora_im();
    void ora_ix();
    void orb_di();
    void orb_ex();
    void orb_im();
    void orb_ix();
    void psha();
    void pshb();
    void pshx();
    void pula();
    void pulb();
    void pulx();
    void rol_ex();
    void rol_ix();
    void rola();
    void rolb();
    void ror_ex();
    void ror_ix();
    void rora();
    void rorb();
    void rti();
    void rts();
    void sba();
    void sbca_di();
    void sbca_ex();
    void sbca_im();
    void sbca_ix();
    void sbcb_di();
    void sbcb_ex();
    void sbcb_im();
    void sbcb_ix();
    void sec();
    void sei();
    void sev();
    void slp();
    void sta_di();
    void sta_ex();
    void sta_im();
    void sta_ix();
    void stb_di();
    void stb_ex();
    void stb_im();
    void stb_ix();
    void std_di();
    void std_ex();
    void std_im();
    void std_ix();
    void sts_di();
    void sts_ex();
    void sts_im();
    void sts_ix();
    void stx_di();
    void stx_ex();
    void stx_im();
    void stx_ix();
    void suba_di();
    void suba_ex();
    void suba_im();
    void suba_ix();
    void subb_di();
    void subb_ex();
    void subb_im();
    void subb_ix();
    void subd_di();
    void subd_ex();
    void subd_im();
    void subd_ix();
    void swi();
    void tab();
    void tap();
    void tba();
    void tim_di();
    void tim_ix();
    void tpa();
    void tst_ex();
    void tst_ix();
    void tsta();
    void tstb();
    void tsx();
    void txs();
    void undoc1();
    void undoc2();
    void wai();
    void xgdx();
    void cpx_di();
    void cpx_ex();
    void cpx_im();
    void cpx_ix();
    void trap();
    void btst_ix();
    void stx_nsc();
};



template<class Archive>
inline void m6800_cpu_device::serialize(Archive & ar, std::uint32_t const version)
{
    ar(m_has_io);
    ar(cereal::binary_data(&m_ppc, sizeof(PAIR)));
    ar(cereal::binary_data(&m_pc, sizeof(PAIR)));
    ar(cereal::binary_data(&m_s, sizeof(PAIR)));
    ar(cereal::binary_data(&m_x, sizeof(PAIR)));
    ar(cereal::binary_data(&m_d, sizeof(PAIR)));
    ar(m_cc, m_wai_state, m_nmi_state, m_nmi_pending);
    ar(cereal::binary_data(m_irq_state, sizeof(uint8_t) * 3));
    ar(m_ic_eddge, m_sc1_state);

    ar(m_ram_ctrl);
    ar(cereal::binary_data(&m_counter, sizeof(PAIR)));
    ar(cereal::binary_data(&m_output_compare, sizeof(PAIR)));
    ar(m_input_capture, m_p3csr_is3_flag_read, m_port3_latched);

    ar(m_clock_divider);
    ar(m_trcsr, m_rmcr, m_rdr, m_tdr, m_rsr, m_tsr);
    ar(m_rxbits, m_txbits, m_txstate, m_trcsr_read_tdre, m_trcsr_read_orfe, m_trcsr_read_rdrf, m_tx, m_ext_serclock);
    ar(m_use_ext_serclock);
    ar(m_port2_written);

    ar(m_icount, m_latch09);
    ar(cereal::binary_data(&m_timer_over, sizeof(PAIR)));
    ar(cereal::binary_data(&m_ea, sizeof(PAIR)));

}

#endif /* __M6800_H__ */


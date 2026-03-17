
#ifndef ZEPHYR_DRIVERS_PWM_SPI_CONTEXT_H_
#define ZEPHYR_DRIVERS_PWM_SPI_CONTEXT_H_
/* pwm reg list */
#define PWM0_BASE(base)               (0 + base)
#define PWM1_BASE(base)               (0x100 + base)
#define PWM2_BASE(base)               (0x200 + base)
#define PWM3_BASE(base)               (0x300 + base)
#define PWM4_BASE(base)               (0x400 + base)
#define PWM5_BASE(base)               (0x500 + base)
#define PWM_FIFO(base)                (0xa00 + base)
#define PWM_IR(base)                  (0xb00 + base)
#define PWM_INT_CTL(base)             (0xc00 + base)
#define PWM_PENDING(base)             (0xc04 + base)
#define PWM_BREATH(base)              (0x30 + base)
#define PWM_BREATH_REG_SIZE           (0x20)

#define pwm_clk_rate                  (12000000)
#define pwm_normal_clk_rate           (32000)

/* pwm control registers */
struct acts_pwm_group0 {
	u32_t ctrl;
	u32_t ch_ctrl0;
	u32_t ch_ctrl1;
	u32_t repeat;
	u32_t cntmax;
	u32_t cmp[6];
	u32_t dt;
};

/* group1 */
struct acts_pwm_group1 {
	u32_t ctrl;
	u32_t ch_ctrl0;
	u32_t ch_ctrl1;
	u32_t repeat;
	u32_t cntmax;
	u32_t cmp[6];
};

/* group 1 breath mode reg */
struct acts_pwm_breath_mode {
	u32_t pwm_bth_a;
	u32_t pwm_bth_b;
	u32_t pwm_bth_c;
	u32_t pwm_bth_d;
	u32_t pwm_bth_e;
	u32_t pwm_bth_f;
	u32_t pwm_bth_hl;
	u32_t pwm_bth_st;
};

/* group 2~5 */
struct acts_pwm_groupx {
	u32_t ctrl;
	u32_t ch_ctrl0;
	u32_t ch_ctrl1;
	u32_t repeat;
	u32_t cntmax;
	u32_t cmp[1];
};

/* fifo */
struct acts_pwm_fifo {
	u32_t fifoctl;
	u32_t fifodat;
	u32_t fifosta;
};

/* IR */
struct pwm_ir_mode_param_t {
	u32_t ir_period;
	u32_t ir_duty;
	u32_t ir_lc;
	u32_t ir_pl0_pre;
	u32_t ir_pl0_post;
	u32_t ir_pl1_pre;
	u32_t ir_pl1_post;
	u32_t ir_ll;
	u32_t ir_ld;
	u32_t ir_pl;
	u32_t ir_pd0;
	u32_t ir_pd1;
	u32_t ir_sl;
	u32_t ir_asc;
	u32_t buf_num;
	u16_t mode;
	u32_t ir_Tf;
	u8_t ir_stop_bit;
	
};

struct acts_pwm_ir {
	u32_t ir_period;
	u32_t ir_duty;
	u32_t ir_lc;
	u32_t ir_pl0_pre;
	u32_t ir_pl0_post;
	u32_t ir_pl1_pre;
	u32_t ir_pl1_post;
	u32_t ir_ll;
	u32_t ir_ld;
	u32_t ir_pl;
	u32_t ir_pd0;
	u32_t ir_pd1;
	u32_t reserve[2];
	u32_t ir_sl;
	u32_t ir_asc;
	u32_t ir_ctl;
};

/* pwm ctl */

#define PWMx_CTRL_HUC(x)                     (1 << (22 + x))//chan : 0~5
#define PWMx_CTRL_HUA                        (1 << 21)
#define PWMx_CTRL_CU                         (1 << 20)
#define PWMx_CTRL_GSM(X)                     (X << 17)//slave mode master source sel: 0 normal, 1 pwm1 co4,2 pwm1 co5,3~6 pwm2~pwm5
#define PWMx_CTRL_SMP                        (1 << 16)
#define PWMx_CTRL_OSM                        (1 << 15)
#define PWMx_CTRL_CHx_MODE_SEL(chan,X)            (X << (3 + chan*2))//0:disable,1:normal,2:programmable ,chan : 0~5
#define PWMx_CTRL_CM                         (1 << 2)
#define PWMx_CTRL_RM                         (1 << 1)
#define PWMx_CTRL_CNT_EN                     (1 << 0)

/* pwm ch_ctl0 */

#define PWMx_CH_CTL0_CHx_OL(chan, X)         (X << (4 + chan*8))//chan : 0~3
#define PWMx_CH_CTL0_CHx_SLM(chan, X)        (X << (1 + chan*8))//chan : 0~3
#define PWMx_CH_CTL0_CHx_POL_SEL(chan)       (1 << (chan*8))//chan : 0~3

/* pwm ch_ctl1 */

#define PWMx_CH_CTL1_CHx_SLM(chan, X)        (X << (1 + (chan - 4)*8))//chan : 4~5
#define PWMx_CH_CTL1_CHx_POL_SEL(chan)       (1 << ((chan - 4)*8))//chan : 4~5

/* pwm dt*/

#define PWMx_DT_CHxP_NEG(chan)                         (1 << (17 + (2*chan)))
#define PWMx_DT_CHxN_NEG(chan)                         (1 << (16 + (2*chan)))
#define PWMx_DT_CHx_DTEN(chan)                         (1 << (10 + chan))
#define PWMx_DT_DT(time)                               (time << 0)

/* pwm bthxy, x:0~5, y:a~f */
#define PWMx_BTHxy_EN                                  (1 << 24)
#define PWMx_BTHxy_REPEAT(X)                           (X << 16)
#define PWMx_BTHxy_STEP(X)                             (X << 8)
#define PWMx_BTHxy_XS(X)                               (X << 0)

/* pwm bth HL */
#define PWMx_BTHx_HL_HEN                               (1 << 21)
#define PWMx_BTHx_HL_LEN                               (1 << 20)
#define PWMx_BTHx_HL_H(X)                              (1 << 10)
#define PWMx_BTHx_HL_L(X)                              (1 << 0)

/* pwm bth ST */
#define PWMx_BTHx_ST_DIR                               (1 << 8)
#define PWMx_BTHx_ST_ST(X)                             (X << 0)

/* pwm fifo ctl */
#define PWM_FIFOCTL_PWM_SEL(X)                         (X << 1)
#define PWM_FIFOCTL_START                              (1 << 0)

/* pwm fifosta */
#define PWM_FIFOSTA_LEVEL                              (3)
#define PWM_FIFOSTA_EMPTY                              (2)
#define PWM_FIFOSTA_FULL                               (1)
#define PWM_FIFOSTA_ERROR                              (0)

/*pwm ir duty*/
#define PWM_IRDUTY_DUTY1(X)                            (X << 16)
#define PWM_IRDUTY_DUTY0(X)                            (X << 0)

/* pwm ir plx pre */
#define PWM_IRPLxPRE_OUT(X)                            (X << 16)
#define PWM_IRPLxPRE_CYCLE(X)                          (X << 0)

/* pwm ir plx post */
#define PWM_IRPLxPOST_OUT(X)                           (X << 16)
#define PWM_IRPLxPOST_CYCLE(X)                         (X << 16)

/*pwm ir ctl*/
#define PWM_IRCTL_PLED                                 (1 << 3)
#define PWM_IRCTL_CU                                   (1 << 2)
#define PWM_IRCTL_STOP                                 (1 << 1)
#define PWM_IRCTL_START                                (1 << 0)

/* pwm interrupt ctl */
#define PWM_INTCTL_IRAE                                (1 << 24)
#define PWM_INTCTL_IRSS                                (1 << 23)
#define PWM_INTCTL_FIFOHE                              (1 << 22)
#define PWM_INTCTL_G5C0                                (1 << 21)
#define PWM_INTCTL_G5REPEAT                            (1 << 20)
#define PWM_INTCTL_G4C0                                (1 << 19)
#define PWM_INTCTL_G4REPEAT                            (1 << 18)
#define PWM_INTCTL_G3C0                                (1 << 17)
#define PWM_INTCTL_G3REPEAT                            (1 << 16)
#define PWM_INTCTL_G2C0                                (1 << 15)
#define PWM_INTCTL_G2REPEAT                            (1 << 14)
#define PWM_INTCTL_G1C(X)                              (1 << (8 + X))
#define PWM_INTCTL_G1REPEAT                            (1 << 7)
#define PWM_INTCTL_G0C(X)                              (1 << (1 + X))
#define PWM_INTCTL_G0REPEAT                            (1 << 0)

/* pwm pending ctl */
#define PWM_PENDING_IRAE                               (1 << 24)
#define PWM_PENDING_IRSS                               (1 << 23)
#define PWM_PENDING_FIFOHE                             (1 << 22)
#define PWM_PENDING_G5C0                               (1 << 21)
#define PWM_PENDING_G5REPEAT                           (1 << 20)
#define PWM_PENDING_G4C0                               (1 << 19)
#define PWM_PENDING_G4REPEAT                           (1 << 18)
#define PWM_PENDING_G3C0                               (1 << 17)
#define PWM_PENDING_G3REPEAT                           (1 << 16)
#define PWM_PENDING_G2C0                               (1 << 15)
#define PWM_PENDING_G2REPEAT                           (1 << 14)
#define PWM_PENDING_G1C(X)                             (1 << (8 + X))
#define PWM_PENDING_G1REPEAT                           (1 << 7)
#define PWM_PENDING_G0C(X)                             (1 << (1 + X))
#define PWM_PENDING_G0REPEAT                           (1 << 0)
#define PWM_PENDING_REPEAT_MASK                        (0x3fffff)


#endif /* ZEPHYR_DRIVERS_PWM_SPI_CONTEXT_H_ */

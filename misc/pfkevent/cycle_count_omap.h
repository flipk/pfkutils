
static inline u32 pfk_read_CCNT(void) {
   u32 val;
   asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
   return val;
}
static inline u32 read_PMNC(void) {
   u32 val;
   asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r" (val));
   return val;
}
static inline void write_PMNC(u32 val) {
   asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r" (val));
}
static inline u32 read_CNTENS(void) {
   u32 val;
   asm volatile("MRC p15, 0, %0, c9, c12, 1" : "=r" (val));
   return val;
}
static inline void write_CNTENS(u32 val) {
   asm volatile("MCR p15, 0, %0, c9, c12, 1" : : "r" (val));
}
static inline void enable_CCNT(void) {
   u32 val = read_PMNC();
   write_PMNC(val | 9);
   val = read_CNTENS();
   write_CNTENS(val | 0x80000000UL);
}

#define PFK_ENABLE_CYCLE_COUNT() enable_CCNT()
#define PFK_CYCLE_COUNT() pfk_read_CCNT()

   
#ifndef __PFK_EVENT_H__
#define __PFK_EVENT_H__

// NOTE, DO NOT USE COMMAS, IT SCREWS UP THE CSV FORMAT
// add your events here

#define PFK_EVENT_LIST                             \
   PFK_EVENT(NULL, "null")                         \
   PFK_EVENT(TEST, "test event")                   \
   PFK_EVENT(SETUP_RX_BUF, "setup rx buf")         \
   PFK_EVENT(TASKLET_START, "tasklet start")       \
   PFK_EVENT(TASKLET_CHECK_IND, "check index")     \
   PFK_EVENT(TASKLET_DONE, "tasklet done")         \
   PFK_EVENT(ISR, "isr event")                     \
   PFK_EVENT(ISR_AFTER, "isr event")               \
   PFK_EVENT(FREE_SKBUFFS, "free skbuffs")         \
   PFK_EVENT(TEARDOWN_DESC, "teardown descs")      \
   PFK_EVENT(RATE_LENGTH, "rate & length")         \
   PFK_EVENT(RX_DATA_HEAD, "rx data head")         \
   PFK_EVENT(PCI_MAP_DATA, "pci map status")       \
   PFK_EVENT(PCI_UNMAP_DATA, "pci unmap status")   \
   PFK_EVENT(DMADESC_STATUS_DATA, "desc status")   \
   PFK_EVENT(DMA_RESTART, "dma was restarted")     \

enum pfk_event_enum {
#define PFK_EVENT(enum,text) PFK_EVENT_##enum,
   PFK_EVENT_LIST
#undef  PFK_EVENT
   PFK_EVENT_LAST,
   PFK_EVENT_MASK = 0x00ffffff
};

extern void pfk_event_log_init(char *name);
extern void pfk_event_log_exit(void);
extern void pfk_event_enable(int value); // 0=disable 1=1 loop 2=continuous
extern void pfk_event_max_events(int max_events);
extern void pfk_event_log(enum pfk_event_enum evt, int v1, int v2,
                          const char *file, const char *func, int line);
extern int pfk_event_enabled(void);

#define PFK_EVENT_LOG(evt,v1,v2)                                        \
    do {                                                                \
        if (pfk_event_enabled())                                        \
            pfk_event_log( PFK_EVENT_##evt,(int)(v1), (int)(v2),        \
                           __FILE__,__PRETTY_FUNCTION__,__LINE__);      \
    } while(0)

#endif /* __PFK_EVENT_H__ */

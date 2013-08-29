
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "redmalloc.h"

#define REGION_VALUE1 0xd1 // 1st red zones right after malloc
#define REGION_VALUE2 0xd2 // user space right after malloc
#define REGION_VALUE3 0xd3 // 2nd red zone right after malloc
#define REGION_VALUE4 0xd4 // 1st red zone after free
#define REGION_VALUE5 0xd5 // user space right after free
#define REGION_VALUE6 0xd6 // 2nd red zone after free

#define REGION1_BODY_SIZE  64
#define REGION1_SIZE (REGION1_BODY_SIZE + sizeof(struct redhdr))
#define REGION2_SIZE  64

struct redhdr {
    struct redhdr * next;
    struct redhdr * prev;
    char * filename;
    uint32_t line_number;
    uint32_t user_size;
    uintptr_t checksum;
};

static struct redhdr * head;
static struct redhdr * tail;

struct region1 {
    struct redhdr hdr;
    unsigned char body[REGION1_BODY_SIZE];
};

struct region2 {
    unsigned char body[REGION2_SIZE];
};

static unsigned char body1ref[REGION1_BODY_SIZE];
static unsigned char body2ref[REGION2_SIZE];

// xxx lock def

static inline void lock(void)
{
    // xxx
}

static inline void unlock(void)
{
    // xxx
}

void
redmallocinit(void)
{
    head = tail = NULL;
    memset(body1ref, REGION_VALUE1, REGION1_BODY_SIZE);
    memset(body2ref, REGION_VALUE3, REGION2_SIZE);
    // xxx lock init
}

static uintptr_t calc_checksum(struct redhdr * hdr)
{
    uintptr_t sum = 0;
    sum += (uintptr_t) hdr->next;
    sum += (uintptr_t) hdr->prev;
    sum += (uintptr_t) hdr->filename;
    sum += (uintptr_t) hdr->line_number;
    sum += (uintptr_t) hdr->user_size;
    return sum;
}

static void update_checksum(struct redhdr * hdr)
{
    hdr->checksum = calc_checksum(hdr);
}

static int is_valid_checksum(struct redhdr * hdr)
{
    return (calc_checksum(hdr) == hdr->checksum);
}

void *
redmalloc(size_t size, char * filename, uint32_t line_number)
{
    if (( size & 7 ) != 0)
    {
        // round up to 8 bytes in size.
        size += 8 - (size & 7);
    }

    uint32_t realsize = REGION1_SIZE + size + REGION2_SIZE;

    uintptr_t  base = (uintptr_t) malloc( realsize );

    if (!base)
        return NULL;

    struct region1 * r1 = (struct region1 *) base;

    r1->hdr.filename = filename;
    r1->hdr.line_number = line_number;
    r1->hdr.user_size = size;
    r1->hdr.prev = NULL;
    lock();
    r1->hdr.next = head;
    if (head == NULL)
        tail = &r1->hdr;
    else
    {
        head->prev = &r1->hdr;
        update_checksum(head);
    }
    update_checksum(&r1->hdr);
    head = &r1->hdr;
    unlock();

    uintptr_t  user_ptr = base + REGION1_SIZE;
    struct region2 * r2 = (struct region2 *) (user_ptr + size);

    memset( r1->body, REGION_VALUE1, REGION1_BODY_SIZE );
    memset( (void*) user_ptr, REGION_VALUE2, size );
    memset( r2->body, REGION_VALUE3, REGION2_SIZE );

    return (void*) user_ptr;
}

static inline int is_r1_body_valid(struct region1 * r1)
{
    if (memcmp(r1->body, body1ref, REGION1_BODY_SIZE) == 0)
        return 1;
    return 0;
}

static inline int is_r2_body_valid(struct region2 * r2)
{
    if (memcmp(r2->body, body2ref, REGION2_SIZE) == 0)
        return 1;
    return 0;
}

void
redfree(void * ptr)
{
    uintptr_t user_ptr = (uintptr_t) ptr;

    struct region1 * r1 = (struct region1 *) (user_ptr - REGION1_SIZE);

    if (!is_valid_checksum(&r1->hdr))
    {
        fprintf(stderr, "redfree: bogus checksum\n");
        return;
    }

    if (!is_r1_body_valid(r1))
    {
        fprintf(stderr, "redfree: r1 body BOGUS\n");
        return;
    }

    uint32_t user_size = r1->hdr.user_size;

    struct region2 * r2 = (struct region2 *) (user_ptr + user_size);

    if (!is_r2_body_valid(r2))
    {
        fprintf(stderr, "redfree: r2 body BOGUS\n");
        return;
    }

    int update;

    lock();
    if (r1->hdr.next)
    {
        struct redhdr * next = r1->hdr.next;
        update = is_valid_checksum( next );
        next->prev = r1->hdr.prev;
        if (update)
            update_checksum( next );
    }
    else
    {
        tail = r1->hdr.prev;
    }
    if (r1->hdr.prev)
    {
        struct redhdr * prev = r1->hdr.prev;
        update = is_valid_checksum( prev );
        prev->next = r1->hdr.next;
        if (update)
            update_checksum( prev );
    }
    else
    {
        head = r1->hdr.next;
    }
    unlock();

    memset( r1->body, REGION_VALUE4, REGION1_BODY_SIZE );
    memset( (void*) user_ptr, REGION_VALUE5, user_size );
    memset( r2->body, REGION_VALUE6, REGION2_SIZE );

    free(r1);
}

void
redcheck(int printall)
{
    struct redhdr * hdr;

    lock();
    for (hdr = head; hdr; hdr = hdr->next)
    {
        uintptr_t user_ptr = (uintptr_t) hdr + REGION1_SIZE;
        struct region1 * r1 = (struct region1 *) hdr;
        struct region2 * r2 = (struct region2 *) (user_ptr + hdr->user_size);

        int checksum_valid = is_valid_checksum(&r1->hdr);
        int r1_body_valid = 0;
        int r2_body_valid = 0;
        int all_valid = 0;

        if (checksum_valid)
        {
            r1_body_valid = is_r1_body_valid(r1);
            r2_body_valid = is_r2_body_valid(r2);

            if (r1_body_valid && r2_body_valid)
                all_valid = 1;
        }

        if (printall || !all_valid)
            printf("%s:%u:%u bytes; ck:%s r1:%s r2:%s\n",
                   r1->hdr.filename, r1->hdr.line_number, r1->hdr.user_size,
                   checksum_valid ? "valid" : "BOGUS",
                   r1_body_valid  ? "valid" : "BOGUS",
                   r2_body_valid  ? "valid" : "BOGUS");
    }
    unlock();
}

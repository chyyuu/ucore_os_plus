#ifndef __DOSM_PACKET_H__
#define __DOSM_PACKET_H__

#ifdef __DOSM__
#include <linux/types.h>
#else

#ifdef LIBSPREFIX
#include <libs/types.h>
#else
#include <types.h>
#endif

#endif

typedef struct dosm_packet_s *dosm_packet_t;
typedef struct dosm_packet_s {
	union {
		struct {
			int16_t source_lapic;
			uint16_t source_flags;
			uint16_t offset_link;

			uint16_t remote_flags;	/* Filled by DOS */
		} __attribute__ ((packed));

		uint64_t status;
	};
	uint64_t priv;
	uint64_t args[6];
} __attribute__ ((packed)) dosm_packet_s;

#define DOSM_SF_USED     0x1
#define DOSM_SF_VALID    0x2
#define DOSM_RF_CHECKED  0x1
#define DOSM_RF_FINISHED 0x2

#define DOSM_PACKET_SIZE 64

#ifdef __DOSM__
void dosm_packet_process(dosm_packet_t packet);
#endif

#endif

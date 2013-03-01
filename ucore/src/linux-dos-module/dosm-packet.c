#include <asm/ipi.h>

#define __DOSM__
#include "dosm-packet.h"
#include "dosm-init.h"

void dosm_packet_process(dosm_packet_t packet)
{
	/* XXX just a demo */
	packet->remote_flags |= DOSM_RF_FINISHED;
	__default_send_IPI_dest_field(packet->source_lapic, ipi_vector,
				      APIC_DEST_PHYSICAL);
}

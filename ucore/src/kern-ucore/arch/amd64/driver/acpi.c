/*
 * =====================================================================================
 *
 *       Filename:  acpi.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/20/2013 03:46:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <kio.h>
#include <assert.h>
#include <acpi_conf.h>
#include <acpi.h>
#include <lapic.h>
#include <mp.h>
#include "sysconf.h"
#include "cpuid.h"

static int table_inited = 0;
static ACPI_TABLE_MADT *hdr_madt;
static ACPI_TABLE_SRAT *hdr_srat;
static ACPI_TABLE_DMAR *hdr_dmar;

/* Early ACPI Table Access */
int acpitables_init(void)
{
	kprintf("acpitables_init()\n");
	ACPI_STATUS r;
	ACPI_TABLE_HEADER *hdr;
	if (ACPI_FAILURE(r = AcpiInitializeTables(NULL, 16, FALSE)))
		panic("acpi: AcpiInitializeTables failed: %s", AcpiFormatException(r));

	// Get the MADT
	r = AcpiGetTable((char*)ACPI_SIG_MADT, 0, &hdr);
	if (ACPI_FAILURE(r) && r != AE_NOT_FOUND)
		panic("acpi: AcpiGetTable failed: %s", AcpiFormatException(r));
	if (r == AE_OK)
		hdr_madt = (ACPI_TABLE_MADT*)hdr;

	// Get the SRAT
	r = AcpiGetTable((char*)ACPI_SIG_SRAT, 0, &hdr);
	if (ACPI_FAILURE(r) && r != AE_NOT_FOUND)
		panic("acpi: AcpiGetTable failed: %s", AcpiFormatException(r));
	if (r == AE_OK)
		hdr_srat = (ACPI_TABLE_SRAT*)hdr;

	// Get the DMAR (DMA remapping reporting table)
	r = AcpiGetTable((char*)ACPI_SIG_DMAR, 0, &hdr);
	if (ACPI_FAILURE(r) && r != AE_NOT_FOUND)
		panic("acpi: AcpiGetTable failed: %s", AcpiFormatException(r));
	if (r == AE_OK)
		hdr_dmar = (ACPI_TABLE_DMAR*)hdr;

	table_inited = 1;
	return 0;
}

static struct lapic_chip* __lapic_chip;
struct lapic_chip *lapic_get_chip()
{
	return __lapic_chip;
}
int lapic_init(void)
{
	static int bsp = 1;
	if(bsp){
		__lapic_chip = x2apic_lapic_init();
		if(!__lapic_chip)
			__lapic_chip = xapic_lapic_init();
		if(!__lapic_chip)
			panic("ERROR: No LAPIC found\n");
	}
	struct lapic_chip* chip = lapic_get_chip();
	assert(chip != NULL);
	chip->cpu_init(chip);

	if(bsp){
		//TODO percpu
		bsp = 0;
	}
	return 0;
}
#define FOR_ACPI_TABLE(head, subtype, sub) for(sub=(subtype *)((head)+1);\
		sub < (subtype *)((char*)head + (head)->Header.Length);  \
		sub=(subtype *)((char*)sub + (sub->Length)))

static uint32_t cpu_id_to_apicid[NCPU];
static void cpumap_init(void)
{
	if(!hdr_madt){
		kprintf("cpumap_init: no madt found\n");
		cpu_id_to_apicid[0] = 0;
		sysconf.lcpu_count = 1;
		return;
	}
	struct lapic_chip* chip = lapic_get_chip();
	uint32_t myapicid = chip->id(chip);
	int count = 1;
	int found_bsp = 0;
	kprintf("cpumap_init: bootcpu apicid: %d\n", myapicid);
	ACPI_SUBTABLE_HEADER *sub;
	FOR_ACPI_TABLE(hdr_madt,ACPI_SUBTABLE_HEADER, sub){
		uint32_t lapicid;
		if(sub->Type == ACPI_MADT_TYPE_LOCAL_APIC) {
			ACPI_MADT_LOCAL_APIC* lapic = (ACPI_MADT_LOCAL_APIC*)sub;
      			if (!(lapic->LapicFlags & ACPI_MADT_ENABLED))
				continue;
			lapicid = lapic->Id;
		}else if(sub->Type == ACPI_MADT_TYPE_LOCAL_X2APIC){
			ACPI_MADT_LOCAL_X2APIC* lapic = (ACPI_MADT_LOCAL_X2APIC*)sub;
      			if (!(lapic->LapicFlags & ACPI_MADT_ENABLED))
				continue;
			lapicid = lapic->LocalApicId;
		}else{
			continue;
		}

		if(lapicid == myapicid){
			found_bsp = 1;
			continue;
		}

		if(count < NCPU){
			cpu_id_to_apicid[count] = lapicid;
		}
		count++;
	}

	if (count > NCPU)
		panic("acpi: Only %d of %d CPUs supported; please increase NCPU", NCPU, count);
	if (!found_bsp)
		panic("Bootstrap process missing from MADT");
	sysconf.lcpu_count = count;


	int i;
	for(i=0;i<count;i++)
		kprintf("  CPU %d: to LAPIC %d\n", i, cpu_id_to_apicid[i]);

}

int numa_init(void)
{
	cpumap_init();
}

int acpi_init(void)
{
	assert(table_inited!=0);
	kprintf("acpi_init()\n");
	kprintf("CPU vendor: %s\n"
			, cpuid_vendor_string());
	return 0;
}


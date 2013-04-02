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
#include <ioapic.h>
#include <mp.h>
#include "sysconf.h"
#include "cpuid.h"

/* declared in mp.h */
struct numa_node numa_nodes[MAX_NUMA_NODES];

#define MAX_TABLE_DESC 16

static int table_inited = 0;
static ACPI_TABLE_DESC table_desc[MAX_TABLE_DESC];
static ACPI_TABLE_MADT *hdr_madt;
static ACPI_TABLE_SRAT *hdr_srat;
static ACPI_TABLE_DMAR *hdr_dmar;

/* Early ACPI Table Access */
int acpitables_init(void)
{
	if(table_inited)
		return;
	kprintf("acpitables_init()\n");
	ACPI_STATUS r;
	ACPI_TABLE_HEADER *hdr;
	if (ACPI_FAILURE(r = AcpiInitializeTables(table_desc, MAX_TABLE_DESC, FALSE)))
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

//XXX should not put here?
static struct lapic_chip* __lapic_chip;
struct lapic_chip *lapic_get_chip()
{
	return __lapic_chip;
}
int lapic_init(void)
{
	static int bsp = 1;
	if(bsp){
		__lapic_chip = x2apic_lapic_init_early();
		if(!__lapic_chip)
			__lapic_chip = xapic_lapic_init_early();
		if(!__lapic_chip)
			panic("ERROR: No LAPIC found\n");
	}
	struct lapic_chip* chip = lapic_get_chip();
	assert(chip != NULL);
	chip->cpu_init(chip);

	if(bsp){
		// not necessary
		mycpu()->hwid = chip->id(chip);
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

/* a proximity_domain is a NUMA node */
static struct numa_node*
proximity_domain_to_node(uint32_t proximity_domain)
{
	int i;
	for(i=0;i<sysconf.lnuma_count;i++)
		if(numa_nodes[i].hwid == proximity_domain)
			return &numa_nodes[i];
	return NULL;
}

int numa_init(void)
{
	int numa_node_nr = 0;
	int i;
	cpumap_init();
	if(!hdr_srat){
		kprintf("numa_init: SRAT not found! Assuming one NUMA node\n");
		numa_nodes[0].id = 0;
		numa_nodes[0].hwid = 0;
		numa_nodes[0].nr_mems = 1;
		numa_nodes[0].nr_cpus = sysconf.lcpu_count;
		numa_nodes[0].mems[0].base = 0;
		numa_nodes[0].mems[0].length = ~0ull;
		for(i=0;i<sysconf.lcpu_count;i++)
			numa_nodes[0].cpu_ids[i] = i;
		sysconf.lnuma_count = 1;
		return;
	}
	ACPI_SUBTABLE_HEADER *sub;
	/* construct NUMA nodes and memory */
	FOR_ACPI_TABLE(hdr_srat,ACPI_SUBTABLE_HEADER, sub){
		ACPI_SRAT_MEM_AFFINITY *aff = (ACPI_SRAT_MEM_AFFINITY*)sub;
		if(!(aff->Flags & ACPI_SRAT_MEM_ENABLED))
			continue;
		uint32_t proximity_domain = aff->ProximityDomain;
		if(hdr_srat->Header.Revision < 2)
			proximity_domain &= 0xff;
		struct numa_node* node = proximity_domain_to_node(proximity_domain);
		if(!node){
			int nr = sysconf.lnuma_count;
			numa_nodes[nr].id = nr;
			numa_nodes[nr].hwid = proximity_domain;
			node = &numa_nodes[nr];
			sysconf.lnuma_count++;
		}
		int nr = node->nr_mems;
		node->mems[nr].base = aff->BaseAddress;
		node->mems[nr].length = aff->Length;
		node->nr_mems ++;
	}
	/* CPUs in NUMA nodes */
	FOR_ACPI_TABLE(hdr_srat,ACPI_SUBTABLE_HEADER, sub){
		uint32_t proximity_domain, apicid;
		switch (sub->Type) {
			case ACPI_SRAT_TYPE_CPU_AFFINITY: {
				ACPI_SRAT_CPU_AFFINITY *aff = (ACPI_SRAT_CPU_AFFINITY*)sub;
				if (!(aff->Flags & ACPI_SRAT_CPU_USE_AFFINITY))
					continue;
			      proximity_domain = aff->ProximityDomainLo;
			      if (hdr_srat->Header.Revision >= 2)
				for (i = 0; i < 3; ++i)
				  proximity_domain |= aff->ProximityDomainHi[i] << (i * 8);
				apicid = aff->ApicId;
			      break;
			    }
			case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY: {
				ACPI_SRAT_X2APIC_CPU_AFFINITY *aff = (ACPI_SRAT_X2APIC_CPU_AFFINITY*)sub;
				if (!(aff->Flags & ACPI_SRAT_CPU_ENABLED))
					continue;
				proximity_domain = aff->ProximityDomain;
				apicid = aff->ApicId;
				break;
			}
			default:
				continue;
			}
		struct numa_node *node = proximity_domain_to_node(proximity_domain);
		assert(node != NULL);
		int found = 0;
		for(i=0;i < sysconf.lcpu_count;i++){
			if(cpu_id_to_apicid[i] == apicid){
				node->cpu_ids[node->nr_cpus++] = i;
				found = 1;
				break;
			}
		}
		if(!found)
			panic("SRAT refers to unknown CPU APICID %d", apicid);
				
	}

	/* dump NUMA nodes */
	for(i=0;i<sysconf.lnuma_count;i++){
		int j;
		kprintf("acpi: NUMA node %d : cpus ", numa_nodes[i].hwid);
		for(j=0;j<numa_nodes[i].nr_cpus;j++)
			kprintf("%d ", numa_nodes[i].cpu_ids[j]);
		kprintf("\n  mems:\n");
		for(j=0;j<numa_nodes[i].nr_mems;j++)
			kprintf("  %p - %p\n", numa_nodes[i].mems[j].base, 
					numa_nodes[i].mems[j].base+numa_nodes[i].mems[j].length-1);
	}

}

static int cpuacpi_init(void)
{
	int ncpu = sysconf.lcpu_count;
	int i, j;
	for(i = 0;i < ncpu; i++){
		struct cpu* c = per_cpu_ptr(cpus, i);
		c->id = i;
		c->node = NULL;
		c->hwid = cpu_id_to_apicid[i];
	}
	/* associate cpus and NUMA nodes */
	for(i=0;i<sysconf.lnuma_count;i++){
		struct numa_node *node = &numa_nodes[i];
		for(j=0;j<node->nr_cpus;j++){
			int id = node->cpu_ids[j];
			struct cpu *c = per_cpu_ptr(cpus, id);
			assert(c->node == NULL);
			c->node = node;
			kprintf("associate cpu%d to NUMA node%d\n", c->id, i);
		}
	}
	/* check */
	for(i=0;i<ncpu;i++){
		struct cpu *c = per_cpu_ptr(cpus, i);
		if(!c->node)
			panic("CPU %d not belong to NUMA nodes!\n", i);
	}
	return 0;
}

int acpi_ioapic_setup(void)
{
	if(!hdr_madt){
		panic("No MADT\n");
		return -1;
	}
	int nr = 0;
	ACPI_SUBTABLE_HEADER *sub;
	FOR_ACPI_TABLE(hdr_madt,ACPI_SUBTABLE_HEADER, sub){
		if (sub->Type == ACPI_MADT_TYPE_IO_APIC) {
			assert(nr < MAX_IOAPIC);
			// [ACPI5.0 5.2.12.3]
			ACPI_MADT_IO_APIC *p= (ACPI_MADT_IO_APIC*)sub;
			ioapic[nr].hwid = p->Id;
			ioapic[nr].phys = p->Address;
			ioapic[nr].intr_base = p->GlobalIrqBase;
			assert(p->Id < 256);
			__ioapic_hwid_to_id[p->Id] = nr;
			ioapic_register_one(&ioapic[nr]);
			nr++;
		} else if (sub->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
			// [ACPI5.0 5.2.12.5]
      			ACPI_MADT_INTERRUPT_OVERRIDE *intov = (ACPI_MADT_INTERRUPT_OVERRIDE*)sub;
			kprintf("TODO irq override source:%d global:%d\n", intov->GlobalIrq, intov->SourceIrq);
		} else if (sub->Type == ACPI_MADT_TYPE_NMI_SOURCE) {
			// [ACPI5.0 5.2.12.6]
			panic("ACPI_MADT_TYPE_NMI_SOURCE not implement\n");
		}

	}
	sysconf.lioapic_count = nr;
	return 0;
}

int cpus_init(void)
{
	cpuacpi_init();
	return 0;
}

int acpi_init(void)
{
	assert(table_inited!=0);
	kprintf("acpi_init()\n");
	kprintf("CPU vendor: %s\n"
			, cpuid_vendor_string());
	return 0;
}

void
acpi_power_off(void)
{
	AcpiEnterSleepStatePrep(ACPI_STATE_S5);
	AcpiDisableAllGpes();
	AcpiEnterSleepState(ACPI_STATE_S5, 0);
}

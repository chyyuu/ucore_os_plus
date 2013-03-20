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

int lapic_init0(void)
{
	static bool bsp = TRUE;
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


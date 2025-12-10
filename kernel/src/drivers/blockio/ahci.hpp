#ifndef AHCI_HPP
#define AHCI_HPP 1

#include <cstdint>
#include <cstddef>
#include <types.hpp>

enum FIS_TYPE {
    FIS_TYPE_REG_H2D    = 0x27,
    FIS_TYPE_REG_D2H    = 0x34,
    FIS_TYPE_DMA_ACT    = 0x39,
    FIS_TYPE_DMA_SETUP  = 0x41,
    FIS_TYPE_DATA       = 0x46,
    FIS_TYPE_BIST       = 0x58,
    FIS_TYPE_PIO_SETUP  = 0x5F,
    FIS_TYPE_DEV_BITS   = 0xA1,
};

struct __attribute__((packed)) HBAPort {
    uint32_t PxCommandListBaseAddress;
    uint32_t PxCommandListBaseAddressUpper;
    uint32_t PxFISBaseAddress;
    uint32_t PxFISBaseAddressUpper;
    uint32_t PxInterruptStatus;
    uint32_t PxInterruptEnable;
    uint32_t PxCommandAndStatus;
    uint32_t PxReserved0;
    uint32_t PxTaskFileData;
    uint32_t PxSignature;
    uint32_t PxSATAStatus;
    uint32_t PxSATAControl;
    uint32_t PxSATAError;
    uint32_t PxSATAActive;
    uint32_t PxCommandIssue;
    uint32_t PxSATANotification;
    uint32_t PxFISSwitchControl;
    uint32_t PxDeviceSleep;
    uint32_t PxReserved1;
    uint8_t PxVendorSpecific[0x7F - 0x70];
};

struct __attribute__((packed)) HBAMem {
    uint32_t HostCapabilities;
    uint32_t GlobalHostControl;
    uint32_t InterruptStatus;
    uint32_t PortsImplemented;
    uint32_t Version;
    uint32_t CommandCompletionCoalescingControl;
    uint32_t CommandCompletionCoalsecingPorts;
    uint32_t EnclosureManagementLocation;
    uint32_t EnclosureManagementControl;
    uint32_t HostCapabilitiesExtended;
    uint32_t BiosOsHandoffControlAndStatus;

    uint8_t Reserved[0x100 - 0x2C];

    HBAPort Ports[32];
};

struct __attribute__((packed)) FIS_REG_H2D {
	uint8_t  fis_type;

	uint8_t  pmport:4;
	uint8_t  rsv0:3;
	uint8_t  c:1;

	uint8_t  command;
	uint8_t  featurel;
	
	uint8_t  lba0;
	uint8_t  lba1;
	uint8_t  lba2;
	uint8_t  device;
	uint8_t  lba3;
	uint8_t  lba4;
	uint8_t  lba5;
	uint8_t  featureh;
	uint8_t  countl;
	uint8_t  counth;
	uint8_t  icc;
	uint8_t  control;

	uint8_t  rsv1[4];
};

struct __attribute__((packed)) FIS_REG_D2H {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:2;
    uint8_t i:1;
    uint8_t rsv1:1;
    uint8_t status;
    uint8_t error;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t rsv2;
    uint8_t countl;
    uint8_t counth;
    uint8_t rsv3[2];
    uint8_t rsv4[4];
};

struct __attribute__((packed)) FIS_DATA {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:4;
    uint8_t rsv1[2];
    uint32_t data[1];
};

struct FIS_DEV_BITS {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:4;
    uint8_t status;
    uint8_t error;
    uint8_t sector_count;
    uint8_t lba_low;
    uint8_t lba_mid;
    uint8_t lba_high;
    uint8_t device;
    uint8_t rsv1[7];
};

struct __attribute__((packed)) FIS_PIO_SETUP {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:1;
    uint8_t d:1;
    uint8_t i:1;
    uint8_t rsv1:1;
    uint8_t status;
    uint8_t error;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t rsv2;
    uint8_t countl;
    uint8_t counth;
    uint8_t rsv3;
    uint8_t e_status;
    uint16_t tc;
    uint8_t rsv4[2];
};

struct __attribute__((packed)) FIS_DMA_SETUP {
	uint8_t  fis_type;
	uint8_t  pmport:4;
	uint8_t  rsv0:1;
	uint8_t  d:1;
	uint8_t  i:1;
	uint8_t  a:1;
    uint8_t  rsved[2];
    uint64_t DMAbufferID;
    uint32_t rsvd;
    uint32_t DMAbufOffset;
    uint32_t TransferCount;
    uint32_t resvd;      
};

struct __attribute__((packed)) HBA_FIS {
    FIS_DMA_SETUP dsfis;
    uint8_t       pad0[4];

    FIS_PIO_SETUP psfis;
    uint8_t       pad1[12];

    FIS_REG_D2H   rfis;
    uint8_t       pad2[4];

    FIS_DEV_BITS  sdbfis;

    uint8_t       ufis[64];

    uint8_t rsv[0x100 - 0xA0];
};

struct __attribute__((packed)) HBA_CMD_HEADER {
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t w:1;
    uint8_t p:1;
    uint8_t r:1;
    uint8_t b:1;
    uint8_t c:1;
    uint8_t rsv0:1;
    uint8_t pmp:4;
    uint16_t prdtl;
    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
};

struct __attribute__((packed)) HBA_PRDT_ENTRY {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9;
    uint32_t i:1;
};

struct __attribute__((packed)) HBA_CMD_TBL {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];

    HBA_PRDT_ENTRY prdt_entry[1];
};

namespace ahci {

void initialise();

ssize_t ahci_driver_read(int port_id, uint64_t lba, size_t sector_count, void* buffer);
ssize_t ahci_driver_write(int port_id, uint64_t lba, size_t sector_count, void* buffer);

}

#endif

#ifndef AHCI_HPP
#define AHCI_HPP 1

#include <cstdint>
#include <cstddef>
#include <types.hpp>

#define AHCI_MAX_PORTS 32
#define AHCI_SECTOR_SIZE 512
#define AHCI_MAX_PRDT_BYTES (4 * 1024 * 1024)
#define AHCI_MAX_PRDT_ENTRIES 32
#define AHCI_CMD_LIST_ENTRIES 32

#pragma pack(push, 0)

typedef struct fis_reg_h2d {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t reserved0:3;
    uint8_t c:1;
    uint8_t command;
    uint8_t featurel;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;

    uint8_t reserved1[4];
} fis_reg_h2d;

typedef struct hba_prdt_entry {
    uint32_t dba;
    uint32_t dbau;
    uint32_t dbc:22;
    uint32_t reserved:9;
    uint32_t i:1;
} hba_prdt_entry;

typedef struct hba_cmd_table {
    fis_reg_h2d cfis;
    uint8_t acmd[16];
    uint8_t reserved[48];
    hba_prdt_entry prdt_entry[AHCI_MAX_PRDT_ENTRIES];
} hba_cmd_table;

typedef struct hba_cmd_header {
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t w:1;
    uint8_t p:1;

    uint8_t r:1;
    uint8_t b:1;
    uint8_t c:1;
    uint8_t reserved0:1;

    uint8_t prdtl;
    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t reserved1[4];
} hba_cmd_header;

struct hba_port {
    uint32_t px_clb;
    uint32_t px_clbu;
    uint32_t px_fb;
    uint32_t px_fbu;
    uint32_t px_is;
    uint32_t px_ie;
    uint32_t px_cmd;
    uint32_t reserved0;
    uint32_t px_tfd;
    uint32_t px_sig;
    uint32_t px_ssts;
    uint32_t px_sctl;
    uint32_t px_serr;
    uint32_t px_sact;
    uint32_t px_ci;
    uint32_t px_sntf;
    uint32_t px_fbs;
    uint32_t px_dev_sleep;
    uint32_t reserved1;
    uint32_t px_vs;
};

struct hba_mem {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;

    uint8_t reserved[0x74];
    hba_port ports[AHCI_MAX_PORTS];
};

#pragma pack(pop)

enum port_type {
    PORT_TYPE_SATA,
    PORT_TYPE_ATAPI,
    PORT_TYPE_PM,
    PORT_TYPE_EM,
    PORT_TYPE_NONE,
};

struct ahci_port {
    hba_port* port;
    uint8_t port_id;
    port_type type;

    hba_cmd_header* cmd_headers;
    hba_cmd_table* cmd_tables[AHCI_CMD_LIST_ENTRIES];
};

namespace ahci {

void initialise();
ssize_t ahci_driver_read(int port_id, uint64_t lba, size_t sector_count, void* buffer);
ssize_t ahci_driver_write(int port_id, uint64_t lba, size_t sector_count, void* buffer);

}

#endif

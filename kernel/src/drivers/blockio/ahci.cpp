#include "ahci.hpp"
#include <pci/pci.hpp>
#include <tmpfs/tmpfs.hpp>
#include <drivers/serial/print.hpp>
#include <mem/mem.hpp>
#include <drivers/timers/pit.hpp>

static pci_device* ahci_dev;
static int ahci_dev_fd;
static hba_mem* abar;

static ahci_port ports[AHCI_MAX_PORTS];

namespace ahci {

static bool wait_for(bool (*condition)(), int timeout_ms) {
    while (timeout_ms-- > 0) {
        if (condition()) return true;
        driver::pit::sleep_ms(1);
    }
    return false;
}

port_type get_port_type(hba_port* port, int id) {
    int retries = 500;
    uint32_t ssts;
    uint8_t det, ipm;

    while (retries-- > 0) {
        ssts = port->px_ssts;
        det = ssts & 0b111;
        ipm = (ssts >> 8) & 0b111;
        if (det == 3 && ipm == 1) break;
        driver::pit::sleep_ms(1);
    }

    if (det != 3 || ipm != 1) {
        printf("PxSataStatus= 0x%08X\n\r", port->px_ssts);
        printf("DET= 0x%04X\n\rIPM= 0x%04X\n\r", det, ipm);
        return PORT_TYPE_NONE;
    }

    switch (port->px_sig) {
        case 0x00000101: return PORT_TYPE_SATA;
        case 0xEB140101: return PORT_TYPE_ATAPI;
        case 0x96690101: return PORT_TYPE_PM;
        case 0xC33C0101: return PORT_TYPE_EM;
        default: return PORT_TYPE_NONE;
    }
}

static int find_free_slot(hba_port* port) {
    uint32_t slots = port->px_sact | port->px_ci;
    for (int i = 0; i < AHCI_CMD_LIST_ENTRIES; i++)
        if (!(slots & (1 << i))) return i;
    return -1;
}

static bool stop_cmd_engine(hba_port* port) {
    port->px_cmd &= ~(1 << 0);
    port->px_cmd &= ~(1 << 4);
    int timeout = 500;
    while ((port->px_cmd & (1 << 15)) && timeout--) driver::pit::sleep_ms(1);
    return !(port->px_cmd & (1 << 15));
}

static bool start_cmd_engine(ahci_port* ahci_port) {
    hba_port* port = ahci_port->port;
    if (!stop_cmd_engine(port)) return false;

    ahci_port->cmd_headers = (hba_cmd_header*)mem::vmm::valloc(1);
    mem::memset(ahci_port->cmd_headers, 0, sizeof(hba_cmd_header) * AHCI_CMD_LIST_ENTRIES);

    void* fis_base = mem::vmm::valloc(1);
    mem::memset(fis_base, 0, 4096);

    port->px_clb = (uint32_t)(uint64_t)ahci_port->cmd_headers;
    port->px_clbu = (uint32_t)((uint64_t)ahci_port->cmd_headers >> 32);
    port->px_fb = (uint32_t)(uint64_t)fis_base;
    port->px_fbu = (uint32_t)((uint64_t)fis_base >> 32);

    for (int i = 0; i < AHCI_CMD_LIST_ENTRIES; i++) {
        ahci_port->cmd_tables[i] = (hba_cmd_table*)mem::vmm::valloc(1);
        mem::memset(ahci_port->cmd_tables[i], 0, sizeof(hba_cmd_table));
        ahci_port->cmd_headers[i].ctba = (uint32_t)(uint64_t)ahci_port->cmd_tables[i];
        ahci_port->cmd_headers[i].ctbau = (uint32_t)((uint64_t)ahci_port->cmd_tables[i] >> 32);
    }

    port->px_is = 0xFFFFFFFF;
    port->px_cmd |= (1 << 4);
    port->px_cmd |= (1 << 0);
    driver::pit::sleep_ms(5);

    return true;
}

static void prepare_fis(fis_reg_h2d* fis, uint64_t lba, size_t sector_count, bool write) {
    mem::memset(fis, 0, sizeof(fis_reg_h2d));
    fis->fis_type = 0x27;
    fis->c = 1;
    fis->command = write ? 0x35 /*WRITE DMA EXT*/ : 0x25 /*READ DMA EXT*/;
    fis->device = 1 << 6;

    fis->lba0 = (uint8_t)(lba);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    fis->countl = sector_count & 0xFF;
    fis->counth = (sector_count >> 8) & 0xFF;
}

static ssize_t ahci_transfer(int port_index, uint64_t lba, size_t sector_count, void* buffer, bool write) {
    ahci_port* ahci_port = &ports[port_index];
    hba_port* port = ahci_port->port;

    int slot = find_free_slot(port);
    if (slot < 0) return -1;

    hba_cmd_header* hdr = &ahci_port->cmd_headers[slot];
    hdr->cfl = sizeof(fis_reg_h2d) / sizeof(uint32_t);
    hdr->w = write ? 1 : 0;
    hdr->prdtl = (sector_count * AHCI_SECTOR_SIZE + AHCI_MAX_PRDT_BYTES - 1) / AHCI_MAX_PRDT_BYTES;

    hba_cmd_table* table = ahci_port->cmd_tables[slot];
    prepare_fis(&table->cfis, lba, sector_count, write);

    size_t bytes_left = sector_count * AHCI_SECTOR_SIZE;
    uint8_t* ptr = (uint8_t*)buffer;

    for (int i = 0; i < hdr->prdtl; i++) {
        size_t n = (bytes_left > AHCI_MAX_PRDT_BYTES) ? AHCI_MAX_PRDT_BYTES : bytes_left;
        table->prdt_entry[i].dba = (uint32_t)(uint64_t)ptr;
        table->prdt_entry[i].dbau = (uint32_t)((uint64_t)ptr >> 32);
        table->prdt_entry[i].dbc = n - 1;
        table->prdt_entry[i].i = 1;

        bytes_left -= n;
        ptr += n;
    }

    port->px_ci |= (1 << slot);

    int timeout = 5000;
    while ((port->px_ci & (1 << slot)) && timeout--) driver::pit::sleep_ms(1);
    if (timeout <= 0 || port->px_serr) return -1;

    return sector_count * AHCI_SECTOR_SIZE;
}

void initialise() {
    ahci_dev = pci::get_device_class_code(0x01, 0x06, true, 0x01);
    if (!ahci_dev) {
        printf("No AHCI controller found!\n\r");
        asm volatile("cli; hlt");
    }

    ahci_dev_fd = tmpfs::open("/dev/ahci_ctrl", O_CREAT | O_RDWR);

    void* abar_phys = (void*)(ahci_dev->bars[5] & ~0xF);
    void* abar_virt = (void*)mem::vmm::pa_to_va((unsigned long)abar_phys);
    mem::vmm::mmap(abar_phys, abar_virt, 1, PAGE_RW | PAGE_PCD);
    abar = (hba_mem*)(abar_virt);

    printf("ABAR mapped at %p\n\r", abar);
    printf("PortsImplemented: %08X\n\r", abar->pi);

    char port_name = 'a';
    int count = 0;

    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if (!(abar->pi & (1 << i))) continue;

        ports[count].port = &abar->ports[i];
        ports[count].port_id = i;
        ports[count].type = get_port_type(&abar->ports[i], i);

        if (ports[count].type != PORT_TYPE_NONE) {
            if (!start_cmd_engine(&ports[count])) {
                printf("Port %d failed to initialise\n\r", i);
                continue;
            }

            char fname[9] = {'/', 'd', 'e', 'v', '/', 's', 'd', port_name++, 0};
            int fd = tmpfs::open(fname, O_CREAT | O_RDWR);
            tmpfs::close(fd);

            printf("Port %d initialized as %s\n\r", i, fname);
        } else {
            printf("Port %d not ready, skipping\n\r", i);
        }

        count++;
    }
}

ssize_t ahci_driver_read(int port_id, uint64_t lba, size_t sector_count, void* buffer) {
    return ahci_transfer(port_id, lba, sector_count, buffer, false);
}

ssize_t ahci_driver_write(int port_id, uint64_t lba, size_t sector_count, void* buffer) {
    return ahci_transfer(port_id, lba, sector_count, buffer, true);
}

}

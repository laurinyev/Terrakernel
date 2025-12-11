#include <panic.hpp>
#include <cstring>
#include <lib/Flanterm/ftctx.h>
#include <drivers/serial/serial.hpp>
#include <drivers/serial/printf.h>
#include <drivers/serial/print.hpp>
#include <arch/arch.hpp>
#include <mem/mem.hpp>
#include <drivers/timers/pit.hpp>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/tables.h>
#include <tmpfs/tmpfs.hpp>
#include <pci/pci.hpp>
#include <drivers/blockio/ahci.hpp>
#include <drivers/video/std_fb.hpp>
#include <drivers/video/placeholder_video.hpp>
#include <exec/elf.hpp>

#define UACPI_ERROR(name, isinit) \
if (uacpi_unlikely_error(uacpi_result)) { \
    Log::errf("uACPI %s Failed: %s", \
              name, uacpi_status_to_string(uacpi_result)); \
    asm volatile ("cli; hlt;"); \
} \
else \
    Log::printf_status("OK", "uACPI %s%s", \
                       name, ((isinit) ? "d" : " Initialised"))

#include <limine.h>
__attribute__((section(".limine_requests")))
volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
    .response = nullptr, // shut up gcc
};

extern "C" void init() {
    if (module_request.response == nullptr || module_request.response->module_count < 1) {
        asm volatile ("cli;hlt");
    }

    flanterm_initialise();
       
    serial::serial_enable();
    Log::print_status("OK", "Flanterm Initialised"); // late
    Log::print_status("OK", "Serial Initialised");
    
    arch::x86_64::cpu::gdt::initialise();
    Log::print_status("OK", "GDT Initialised");
    
    arch::x86_64::cpu::idt::initialise();
    Log::print_status("OK", "IDT Initialised");
    
    mem::pmm::initialise();
    Log::print_status("OK", "PMM Initialised");
    
    mem::vmm::initialise();
    Log::print_status("OK", "VMM Initialised");

	mem::vmm::print_mem();

    mem::heap::initialise();
    Log::print_status("OK", "Heap Initialised");
    
    driver::pit::initialise();
    Log::print_status("OK", "PIT Initialised (FREQ=300)");

    /*Log::info("Disabling COM1 serial output, falling back to graphical interface");
    serial::serial_disable();
    Log::print_status("OK", "Serial Disabled");*/

    asm("sti");

    uacpi_status uacpi_result = uacpi_initialize(0);
    UACPI_ERROR("Initialise", 1);
    
    uacpi_result = uacpi_namespace_load();
    UACPI_ERROR("namespace loade", 1);
    
    uacpi_result = uacpi_namespace_initialize();
    UACPI_ERROR("namespace", 0);

    uacpi_result = uacpi_finalize_gpe_initialization();
    UACPI_ERROR("GPE", 0);

	tmpfs::initialise();
    tmpfs::mkdir("/dev", 0777);
    int stdin = tmpfs::open("/dev/stdin", O_CREAT | O_RDWR);
    int stdout = tmpfs::open("/dev/stdout", O_CREAT | O_RDWR);
    int stderr = tmpfs::open("/dev/stderr", O_CREAT | O_RDWR);
    tmpfs::load_initrd(module_request.response->modules[0]->address, module_request.response->modules[0]->size);
    tmpfs::list_initrd();
    Log::print_status("OK", "TMPFS Initialised");

    pci::initialise();
    Log::print_status("OK", "Detected all PCI devices");

    ahci::initialise();
    Log::print_status("OK", "AHCI Initialised");

	arch::x86_64::syscall::initialise();
    Log::print_status("OK", "Syscalls Initialised");

    tmpfs::print_tree();
    printf("\n\r");

	int fd = tmpfs::open("/initrd/init", O_RDWR);
    printf("Opened file %d\n\r", fd);
	stat sb;
	tmpfs::fstat(fd, &sb);
	if (sb.st_size < 1) { // It will always be 1+ because there is a fake byte being added, a zero, basically
							// its a file terminator if you are slow to understand :>
		Log::errf("Failed to get valid init process");
		asm volatile ("cli;hlt;");
	}

	void* buf = mem::heap::malloc(sb.st_size);
	ssize_t to_read = tmpfs::read(fd, buf, sb.st_size);
	if (to_read < 1) {
		Log::errf("Failed to read valid init data");
		asm volatile ("cli;hlt;");
	}

	run_elf(buf, to_read);

	/* To contributors and to me because I (~~__##**WILL**##__~~) forget :>

		MAKE THIS INIT THE LAST BECAUSE DEBUG MESAGES DNT SHOW UP IF STD_FB DEVICE IS INITIALISED

	*/

    driver::video::std_fb::initialise();
    Log::print_status("OK", "STD FB Device Initialised, Created /dev/std_fb");

    while (1) {
        asm volatile("hlt");
    }
    
    __builtin_unreachable();
}

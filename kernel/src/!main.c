#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <basic.h>

#include <gpxwm/gpxwm.h>
#include <gdt/gdt.h>
#include <mmu/pmm.h>
#include <mmu/vmm.h>
#include <idt/idt.h>
#include <psf/psf.h>

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};
__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void kmain(void) {
	printsts("TerracottaOS Booted", STATUS_OK);
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    	printsts("Limine base revision unsupported", STATUS_ERR);
        hcf();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
     	printsts("Could not detect a framebuffer", STATUS_ERR);
        hcf();
    }

    if (framebuffer_request.response->framebuffer_count > 1) {
    	printk("Detected more than one display\n\r");
    }

    if (memmap_request.response == NULL
     || memmap_request.response->entry_count < 1) {
     	printsts("Could not detect a memory map", STATUS_ERR);
     	hcf();
    }
    
    if (module_request.response == NULL
     || module_request.response->module_count < 1) {
     	printsts("Could not detect any modules", STATUS_ERR);
     	hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    struct limine_file* font18 = module_request.response->modules[0];

    psf_renderer_init(font18->address, font18->size, framebuffer->width, framebuffer->height);

	if (framebuffer_request.response->framebuffer_count > 1) {
		for (int i = 0; i < framebuffer->width; i++) {
			for (int j = 0; j < framebuffer->height; j++) {
				volatile uint32_t* px = ((volatile uint32_t*)framebuffer->address) + (j * framebuffer->width + i);
				px = (i << 8) | (j << 4) | (i << 4) | (i + j);
			}
		}
	}

    int idx_1largest = -1, idx_2largest = -1, idx_3largest = -1, idx_4largest = -1;
    uint64_t sz_1largest = 0, sz_2largest = 0, sz_3largest = 0, sz_4largest = 0;

    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap_request.response->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        uint64_t sz = entry->length;

        if (sz > sz_1largest) {
            // shift everything down
            sz_4largest = sz_3largest; idx_4largest = idx_3largest;
            sz_3largest = sz_2largest; idx_3largest = idx_2largest;
            sz_2largest = sz_1largest; idx_2largest = idx_1largest;
            sz_1largest = sz; idx_1largest = i;
        } else if (sz > sz_2largest) {
            sz_4largest = sz_3largest; idx_4largest = idx_3largest;
            sz_3largest = sz_2largest; idx_3largest = idx_2largest;
            sz_2largest = sz; idx_2largest = i;
        } else if (sz > sz_3largest) {
            sz_4largest = sz_3largest; idx_4largest = idx_3largest;
            sz_3largest = sz; idx_3largest = i;
        } else if (sz > sz_4largest) {
            sz_4largest = sz; idx_4largest = i;
        }
    }

    // Example: print results
    if (idx_1largest != -1) {
        struct limine_memmap_entry* e = memmap_request.response->entries[idx_1largest];
        printk("Largest: base=0x%lx, len=0x%lx\n", e->base, e->length);
    }
    if (idx_2largest != -1) {
        struct limine_memmap_entry* e = memmap_request.response->entries[idx_2largest];
        printk("2nd largest: base=0x%lx, len=0x%lx\n", e->base, e->length);
    }
    if (idx_3largest != -1) {
        struct limine_memmap_entry* e = memmap_request.response->entries[idx_3largest];
        printk("3rd largest: base=0x%lx, len=0x%lx\n", e->base, e->length);
    }
    if (idx_4largest != -1) {
        struct limine_memmap_entry* e = memmap_request.response->entries[idx_4largest];
        printk("4th largest: base=0x%lx, len=0x%lx\n", e->base, e->length);
    }

	printk("Initializing GPX WM\n\r");
    WMInit(framebuffer);

    WMWindow* winS = WMCreateWindow("Settings", RGB(100,200,200));

    WMDrawAll();
    printsts("GPX WM initialized", STATUS_OK);

    /* Global Descriptor Table */
    gdt_init();
    printsts("GDT initialized", STATUS_OK);

    /* Interrupt Descriptor Table */
    idt_init();

    /* Syscall handler */
    //syscall_init();

    /* Enable `syscall` instruction */
    //enable_syscall();

    /* Initialize physical memory manager */
    uint64_t membase0 = memmap_request.response->entries[idx_1largest]->base;
    uint64_t membase1 = memmap_request.response->entries[idx_2largest]->base;
    uint64_t membase2 = memmap_request.response->entries[idx_3largest]->base;
    uint64_t membase3 = memmap_request.response->entries[idx_4largest]->base;
    pmm_init(sz_1largest, membase0, sz_2largest, membase1, sz_3largest, membase2, sz_4largest, membase3);
    pmm_setup_page_tables();
    printsts("PMM initialized", STATUS_OK);

    /* Initialize virtual memory manager */

    /* Initialize heap */
    //heap_init();

    /* Initialize framebuffer */
    //fb_init(framebuffer);

    /* Initialize minecraft (joke) */
    /* Initialize ELF executable loader*/

    font_set_colors(RGB(255,255,255), RGB(0,0,0));

    while (1);

    hcf();
}

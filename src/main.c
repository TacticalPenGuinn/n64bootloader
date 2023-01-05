/*  n64bootloader, a Linux bootloader for the N64
    Copyright (C) 2020 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>

typedef uint64_t u64;
typedef unsigned int u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef signed int s32;
typedef int16_t s16;
typedef int8_t s8;

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;

#define EI_NIDENT (16)

#define EI_CLASS 4	   /* File class byte index */
#define ELFCLASSNONE 0 /* Invalid class */
#define ELFCLASS32 1   /* 32-bit objects */
#define ELFCLASS64 2   /* 64-bit objects */
#define ELFCLASSNUM 3

typedef struct
{
	unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
	Elf32_Half e_type;				  /* Object file type */
	Elf32_Half e_machine;			  /* Architecture */
	Elf32_Word e_version;			  /* Object file version */
	Elf32_Addr e_entry;				  /* Entry point virtual address */
	Elf32_Off e_phoff;				  /* Program header table file offset */
	Elf32_Off e_shoff;				  /* Section header table file offset */
	Elf32_Word e_flags;				  /* Processor-specific flags */
	Elf32_Half e_ehsize;			  /* ELF header size in bytes */
	Elf32_Half e_phentsize;			  /* Program header table entry size */
	Elf32_Half e_phnum;				  /* Program header table entry count */
	Elf32_Half e_shentsize;			  /* Section header table entry size */
	Elf32_Half e_shnum;				  /* Section header table entry count */
	Elf32_Half e_shstrndx;			  /* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
	Elf32_Word p_type;	 /* Segment type */
	Elf32_Off p_offset;	 /* Segment file offset */
	Elf32_Addr p_vaddr;	 /* Segment virtual address */
	Elf32_Addr p_paddr;	 /* Segment physical address */
	Elf32_Word p_filesz; /* Segment size in file */
	Elf32_Word p_memsz;	 /* Segment size in memory */
	Elf32_Word p_flags;	 /* Segment flags */
	Elf32_Word p_align;	 /* Segment alignment */
} Elf32_Phdr;

extern int __bootcic;

static u8 hdrbuf[256] __attribute__((aligned(16)));


static const char *const args[] = {
	"hello",
	(const char *)hdrbuf,
	(const char *)hdrbuf + 128,
	"root=/dev/n64cart"};

static char buf[64];

static u32 kernelsize __attribute__((aligned(8)));
static u32 disksize __attribute__((aligned(8)));
static u32 diskoff __attribute__((aligned(8)));

/* main code entry point */
int main(void)
{

	const int osMemSize = (__bootcic != 6105) ? (*(int *)0xA0000318) : (*(int *)0xA00003F0);

	console_init();

	sprintf(buf, "Found %u kb of RAM\n", osMemSize / 1024);
	printf(buf);

	data_cache_hit_writeback_invalidate(&kernelsize, 4);
	dma_read(&kernelsize, 0xB0101000 - 4, 4);

	data_cache_hit_writeback_invalidate(&disksize, 4);
	dma_read(&disksize, 0xB0101000 - 8, 4);

	diskoff = ((kernelsize + 4095) & ~4095);
	if (!kernelsize)
	{
		printf("No kernel configured, halting...\n");

		while (1);

	}

	Elf32_Ehdr *const ptr = (Elf32_Ehdr *)hdrbuf;
	sprintf(buf, "Booting kernel %u kb, %u kb\n", kernelsize / 1024,
			disksize / 1024);
	printf(buf);

	sprintf(buf, "Address: %p\n", ptr);
	printf(buf);

	dma_read(ptr, 0xB0101000, 256);
	data_cache_hit_invalidate(ptr, 256);

	if (ptr->e_ident[1] != 'E' ||
			ptr->e_ident[2] != 'L' ||
			ptr->e_ident[3] != 'F')
		printf("Not an ELF kernel?\n");

	if (ptr->e_ident[EI_CLASS] != ELFCLASS32)
		printf("Not a 32-bit kernel?\n");

	// Where is it wanted?
	const Elf32_Phdr *phdr = (Elf32_Phdr *)(hdrbuf + ptr->e_phoff);
	while (phdr->p_type != 1)
		phdr++;

	sprintf(buf, "LoadAddress: %p\n", (void *)phdr->p_paddr);
	printf(buf);

	sprintf(buf, "LoadOffset: %p\n", (void *)0xB0101000 + phdr->p_offset);
	printf(buf);

	// Put it there
	dma_read((void *)phdr->p_paddr, 0xB0101000 + phdr->p_offset,
			(phdr->p_filesz + 1) & ~1);
	data_cache_hit_writeback_invalidate((void *)phdr->p_paddr, (phdr->p_filesz + 3) & ~3);

	// Zero any extra memory desired
	if (phdr->p_filesz < phdr->p_memsz)
	{
		memset((void *)(phdr->p_paddr + phdr->p_filesz), 0,
				phdr->p_memsz - phdr->p_filesz);
	}

	void (*start_kernel)(int, const char *const *, const char *const *, int *) = (void *)ptr->e_entry;

	sprintf(buf, "Entry: %p\n", (void *)ptr->e_entry);
	printf(buf);

	// initialize hdrbuf to zero
	memset((void *)hdrbuf, 0, 256);


	// Fill out our disk info
	sprintf((char *)hdrbuf, "n64cart.start=%u", 0xB0101000 + diskoff);
	sprintf((char *)hdrbuf + 128, "n64cart.size=%u", disksize);

	sprintf(buf, "Disk: %u\n", diskoff);
	printf(buf);

	printf("%s\n",(const char *)hdrbuf);
	printf("%s\n",(const char *)hdrbuf + 128);

	sprintf(buf, "Jumping to: %p\n", (void *)start_kernel);
	printf(buf);

	wait_ms(1024);

	disable_interrupts();
	set_VI_interrupt(0, 0);

	start_kernel(sizeof(args) / sizeof(args[0]), args, NULL, NULL /* unused */);

	return 0;
}

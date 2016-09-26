/* wrap.c -- wrap a ROM image as an ELF file so
 * that it can be disassembled
 * Part of the Convergent Technologies "Mightyframe/S80" effort.
 * Tom Trebisky  9-5-2014
 *
 * Subverted for ESP8266 bootrom disassembly 1-20-2016
 * And again for STM32F103 serial bootrom 9-24-2016
 * 
 * Interestingly it works with almost no changes, and with big-endian
 * values for the most part, and the machine type set to m68k.
 * A side effect of this that I like is that memory is dumped
 * in true byte order, rather than byte swapping the 24 bit
 * ESP8266 xtensa op-codes.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

/* ntohs() and such */
#include <arpa/inet.h>

#include <elf.h>

#define PGSIZE 8192	/* 0x2000 */

/* Mightyframe ROM is 27256 (32K, 4 pages) */
/* esp8266 is 64k (65536) */
#define MAX_ROMSIZE 65536

#ifdef notdef
/* S80_boot_72-01231.BIN */
unsigned int rombase = 0x80000000;
unsigned int entry = 0x80000008;

/* heurikon.bin */
unsigned int rombase = 0x00fc0000;
unsigned int entry = 0x00fc0082;
#endif

/* now we get the image and elf names from the command line */
char *rom_image;
char *elf_image;
char *sym_file;

/* These must be hand edited */
#ifdef ESP
unsigned int rombase = 0x40000000;
unsigned int entry = 0x400000a4;
#endif /* ESP */

/* For the STM32F103 serial bootrom */
unsigned int rombase = 0x1ffff000;
unsigned int entry = 0x1ffff021;

char buf[MAX_ROMSIZE];
int romsize;

int offset;

struct string {
	char *buf;
	int off;
	int limit;
};

struct string seg_strings;
struct string sym_strings;

#define MAX_SYM	400

struct symtab {
	Elf32_Sym sym[MAX_SYM];
	int count;
	int size;
	int limit;
};

struct symtab syms;

/* --------------------------------------------- */

void init_string ( struct string * );
int add_string ( struct string *, char * );
void add_char ( struct string *, int );
int mk_string ( struct string *, int );
void finish_string ( struct string * );

void mk_shdr_null ( int );
void mk_shdr_t ( int, int );
void mk_shdr_g ( int, int );
void mk_shdr_s ( int, int, int );
void mk_shdr_sy ( int, int, int, int );

int load_syms ( char * );
void init_symbol ( void );
void add_symbol ( char *, unsigned int );
void finish_symbol ( void );
int mk_symbol ( int );

char gnu_extra[] = "GCC: (GNU) 4.7.2";
int gnusize;

int mk_phdr ( int );

/*
 * Here is what an elf file looks like:
 *
 *  Elf header (52 bytes)
 *  Elf phdr (32 bytes) "program header"
 *  zero padding to get to 8k (8192) boundary
 *  Section 0 - null (not actually present)
 *  Section 1 - binary image
 *  Section 2 - gnu extra string
 *  Section 3 - section string table
 *  Section 4 - maybe symbol table
 *  Section 5 - maybe symbol table strings
 *  ELF segment headers
 */

int main ( int argc, char **argv )
{
	Elf32_Ehdr hdr;
	int rom, of;
	int shnum;
	int ph_off;
	int sh_off;
	char pad[PGSIZE];
	int npad;
	int s_index;
	int g_index;
	int t_index;
	int sy_index;
	int sys_index;
	int st_size;
	int sym_size;
	int syst_size;
	int nsy;

	/* Extra argument is symbol file */
	sym_file = NULL;
	if ( argc == 4 ) {
	    sym_file = argv[3];
	    argc--;
	}

	if ( argc != 3 ) {
	    fprintf ( stderr, "Usage: wrap bin elf\n" );
	    exit ( 1 );
	}

	rom_image = argv[1];
	elf_image = argv[2];

	printf ( "Reading image: %s\n", rom_image );
	rom = open ( rom_image, O_RDONLY );
	if ( rom < 0 ) {
	    fprintf ( stderr, "Cannot access file: %s\n", rom_image );
	    exit ( 1 );
	}

	of = creat ( elf_image, 0664 );
	if ( of < 0 ) {
	    fprintf ( stderr, "Cannot create file: %s\n", elf_image );
	    exit ( 1 );
	}

	romsize = read ( rom, buf, MAX_ROMSIZE );
	printf ( " ... %d bytes read\n", romsize );
	close ( rom );

	gnusize = strlen(gnu_extra) + 1;

	shnum = 4;	/* We have 4 sections in our section table */
	if ( sym_file ) {
	    shnum += 2;	/* includes symtab and strings */
	    nsy = load_syms ( sym_file );
	    printf ( "%d symbols loaded\n", nsy );
	}

	/* program table immediately follows elf header */
	ph_off = sizeof(hdr);

	/* build string table now */
	init_string ( &seg_strings );
	add_string ( &seg_strings, "" );
	s_index = add_string ( &seg_strings, ".shstrtab" );
	t_index = add_string ( &seg_strings, ".text" );
	if ( sym_file ) {
	    sy_index = add_string ( &seg_strings, ".symtab" );
	    sys_index = add_string ( &seg_strings, ".strtab" );
	}
	finish_string ( &seg_strings );

	/* May need to add a pad byte to string table so
	 * that segment headers get even alignment.
	 */
	sh_off = PGSIZE + romsize + gnusize + seg_strings.off;
	if ( sym_file ) {
	    sh_off += syms.size + sym_strings.off;
	}

	/* Fill in the elf file header (52 bytes) */
	strncpy ( hdr.e_ident, "-ELF", 4 );
	hdr.e_ident[0] = 0x7F;
	/* note there are 16 bytes in the ident array */

	hdr.e_ident[4] = 1;	/* 32 bit addresses */
	hdr.e_ident[5] = 2;	/* big endian */
	hdr.e_ident[6] = 1;	/* elf version 1 */
	hdr.e_ident[7] = 0;	/* target OS - System V */

	hdr.e_ident[8] = 0;	/* ABI version */
	hdr.e_ident[9] = 0;	/* - unused */
	hdr.e_ident[10] = 0;	/* - unused */
	hdr.e_ident[11] = 0;	/* - unused */

	hdr.e_ident[12] = 0;	/* - unused */
	hdr.e_ident[13] = 0;	/* - unused */
	hdr.e_ident[14] = 0;	/* - unused */
	hdr.e_ident[15] = 0;	/* - unused */

	hdr.e_type = ntohs(2);

	/* XXX - how do we control 68010 versus 68020 disassembly ? */
	// hdr.e_machine = ntohs(4);	/* mc680x0 */
	// hdr.e_machine = EM_68K;
	hdr.e_machine = EM_ARM;

	hdr.e_version = ntohl(1);
	hdr.e_entry = ntohl ( entry );
	hdr.e_phoff = ntohl ( ph_off );
	hdr.e_shoff = ntohl ( sh_off );
	hdr.e_flags = ntohl ( 0x01000000 );
	hdr.e_ehsize = ntohs ( 52 );
	hdr.e_phentsize = ntohs ( 0x20 );
	hdr.e_phnum = ntohs ( 1 );
	hdr.e_shentsize = ntohs ( 40 );

	hdr.e_shnum = ntohs ( shnum );		/* size of our section table */
	hdr.e_shstrndx = ntohs ( 3 );

	write ( of, &hdr, sizeof(hdr) );	/* Write file header */

	memset ( pad, 0, PGSIZE );
	npad = PGSIZE - sizeof(hdr);

	npad -= mk_phdr ( of );			/* Write program header */

	write ( of, pad, npad );		/* Write pad to page boundary */

	/* The ROM image !!! */
	write ( of, buf, romsize );		/* Write ROM image */

	/* The gnu signature */
	write ( of, gnu_extra, gnusize );	/* Write Gnu signature */

	/* write segment string table */
	st_size = mk_string ( &seg_strings, of );	/* Write Segment String table */

	if ( sym_file ) {
	    sym_size = mk_symbol ( of );			/* Write Symbol table */
	    syst_size = mk_string ( &sym_strings, of );		/* Write Symbol String table */
	}

	offset = PGSIZE;
	mk_shdr_null ( of );			/* section header (start with null) */
	mk_shdr_t ( of, t_index );		/* section header for ROM image */
	mk_shdr_g ( of, g_index );		/* XXX - section header for gnu */
	mk_shdr_s ( of, s_index, st_size );	/* section header for string table */

	if ( sym_file ) {
	    mk_shdr_sy ( of, sy_index, sym_size, shnum-1 );	/* section header for symbol table */
	    mk_shdr_s ( of, sys_index, syst_size );	/* section header for string table */
	}

	close ( of );
	exit ( 0 );
}

#define MAXLINE	256

int
load_syms ( char *file )
{
	FILE *fp;
	char line[MAXLINE];
	int n = 0;
	char val[9];
	char name[MAXLINE];
	unsigned int addr;

	fp = fopen ( file, "r" );
	if ( fp == NULL ) {
	    fprintf ( stderr, "Cannot read symbol file: %s\n", file );
	    exit ( 1 );
	}

	init_symbol ();

	while ( fgets ( line, MAXLINE, fp ) != NULL ) {
	    line[strlen(line)-1] = '\0';	/* nuke newline */

	    strncpy ( val, line, 8 );
	    val[8] = '\0';
	    addr = strtol ( val, NULL, 16 );
	    strcpy ( name, &line[9] );
	    add_symbol ( name, addr );
	    n++;
	}

	finish_symbol ();

	return n;
}

void swap_shdr ( int *a, int count )
{
	int i;

	/*
	printf ( "int is %d bytes, %d of em\n", sizeof(int), count );
	*/

	for ( i=0; i<count; i++ ) {
	    /*
	    printf ( "preswap %d: %08x\n", i, a[i] );
	    */
	    a[i] = ntohl ( a[i] );
	    /*
	    printf ( "post    %d: %08x\n", i, a[i] );
	    */
	}
}

void
mk_shdr_null ( int of )
{
	Elf32_Shdr shdr;

	memset ( (char *) &shdr, 0, sizeof(shdr) );

	write ( of, &shdr, sizeof(shdr) );
}

void
mk_shdr_t ( int of, int t_index )
{
	Elf32_Shdr shdr;

	shdr.sh_name = t_index;
	shdr.sh_type = SHT_PROGBITS;
	shdr.sh_flags = 6;
	shdr.sh_addr = rombase;
	shdr.sh_offset = offset;
	shdr.sh_size = romsize;
	offset += romsize;
	shdr.sh_link = 0;
	shdr.sh_info = 0;
	shdr.sh_addralign = 4;
	shdr.sh_entsize = 0;

	swap_shdr ( (int *)&shdr, sizeof(shdr) / sizeof(int) );

	write ( of, &shdr, sizeof(shdr) );
}

/* Could probably drop this */
void
mk_shdr_g ( int of, int index )
{
	Elf32_Shdr shdr;

	shdr.sh_name = index;
	shdr.sh_type = SHT_PROGBITS;
	shdr.sh_flags = 0x30;
	shdr.sh_addr = 0;
	shdr.sh_offset = offset;
	shdr.sh_size = gnusize;
	offset += gnusize;
	shdr.sh_link = 0;
	shdr.sh_info = 0;
	shdr.sh_addralign = 1;
	shdr.sh_entsize = 1;

	swap_shdr ( (int *)&shdr, sizeof(shdr) / sizeof(int) );

	write ( of, &shdr, sizeof(shdr) );
}

void
mk_shdr_s ( int of, int s_index, int size )
{
	Elf32_Shdr shdr;
	int n;

	shdr.sh_name = s_index;
	shdr.sh_type = SHT_STRTAB;
	shdr.sh_flags = 0;
	shdr.sh_addr = 0;
	shdr.sh_offset = offset;
	shdr.sh_size = size;
	offset += size;
	shdr.sh_link = 0;
	shdr.sh_info = 0;
	shdr.sh_addralign = 1;
	shdr.sh_entsize = 0;

	/*
	printf ( "type: %08x\n", shdr.sh_type );
	printf ( "sizeof name thingie = %d\n", sizeof(shdr.sh_name) );
	*/
	swap_shdr ( (int *)&shdr, sizeof(shdr) / sizeof(int) );
	/*
	printf ( "type: %08x\n", shdr.sh_type );
	printf ( "s type: %08x\n", shdr.sh_type );
	*/

	n = write ( of, &shdr, sizeof(shdr) );
}

void
mk_shdr_sy ( int of, int s_index, int size, int string_index )
{
	Elf32_Shdr shdr;
	int n;

	shdr.sh_name = s_index;
	shdr.sh_type = SHT_SYMTAB;
	shdr.sh_flags = 0;
	shdr.sh_addr = 0;
	shdr.sh_offset = offset;
	shdr.sh_size = size;
	offset += size;
	shdr.sh_link = string_index;
	shdr.sh_info = 0;
	shdr.sh_addralign = 1;
	shdr.sh_entsize = sizeof(Elf32_Sym);

	swap_shdr ( (int *)&shdr, sizeof(shdr) / sizeof(int) );

	n = write ( of, &shdr, sizeof(shdr) );
}

int
mk_phdr ( int of )
{
	Elf32_Phdr phdr;

	phdr.p_type = ntohl ( 1 );
	phdr.p_offset = ntohl ( 0x2000 );
	phdr.p_vaddr = ntohl ( rombase );
	phdr.p_paddr = ntohl ( rombase );
	phdr.p_filesz = ntohl ( romsize );
	phdr.p_memsz = ntohl ( romsize );
	phdr.p_flags = ntohl ( 5 );
	phdr.p_align = ntohl ( 0x2000 );

	write ( of, &phdr, sizeof(phdr) );

	return sizeof(phdr);
}

/* ------------------------------------- */

/* symbol table always starts with this */
void add_null ( void )
{
	Elf32_Sym *sp;
	int ix;
	int info;

	sp = &syms.sym[syms.count++];
	syms.size += sizeof ( *sp );

	sp->st_name = 0;
	sp->st_value = 0;
	sp->st_info = 0;
	sp->st_shndx = SHN_UNDEF;
}

void init_symbol ( void )
{
	init_string ( &sym_strings );
	syms.count = 0;
	syms.size = 0;
	syms.limit = MAX_SYM;
	memset ( (char *) &syms, 0, sizeof ( struct symtab) );
	add_null ();
}

void add_symbol ( char *name, unsigned int addr )
{
	Elf32_Sym *sp;
	int ix;
	int info;

	sp = &syms.sym[syms.count++];
	syms.size += sizeof ( *sp );

	ix = add_string ( &sym_strings, name );
	sp->st_name = ntohl ( ix );
	sp->st_value = ntohl ( addr );
	info = ELF32_ST_INFO ( STB_GLOBAL, STT_FUNC );
	sp->st_info = info;
	sp->st_shndx = ntohs ( 1 );	/* rom image section */
}

int mk_symbol ( int of )
{
	write ( of, syms.sym, syms.size );
	return syms.size;
}

void finish_symbol ( void )
{
	finish_string ( &sym_strings );
}

/* ------------------------------------- */

void
init_string ( struct string *sp )
{
	sp->buf = malloc ( PGSIZE );
	sp->limit = PGSIZE;
	sp->off = 0;
}

int add_string ( struct string *sp, char *s )
{
	int rv = sp->off;

	strcpy ( &sp->buf[sp->off], s );
	sp->off += strlen(s) + 1;

	/* return offset of string just stored */
	return rv;
}

/* ensure even alignment */
void
finish_string ( struct string *sp )
{
	if ( (sp->off % 2) == 1 )
	    sp->buf[sp->off++] = '\0';
}

int mk_string ( struct string *sp, int of )
{
	write ( of, sp->buf, sp->off );

	/* This will be size of object written */
	return sp->off;
}

/* THE END */

#ifndef __LIBPSO_i386_H__
#define __LIBPSO_i386_H__

#include <tipos.h>

#define LS_INLINE static __inline __attribute__((always_inline))

/* out functions */
LS_INLINE void outb(int port, uint_8 data);
LS_INLINE void outsb(int port, const uint_8 *addr, int cnt);
LS_INLINE void outw(int port, uint_16 data);
LS_INLINE void outsw(int port, const uint_16 *addr, int cnt);
LS_INLINE void outl(int port, uint_32 data);
LS_INLINE void outsl(int port, const uint_32 *addr, int cnt);

/* in functions */
LS_INLINE uint_8  inb(int port);
LS_INLINE void insb(int port, void *addr, int cnt);
LS_INLINE uint_16 inw(int port);
LS_INLINE void insw(int port, void *addr, int cnt);
LS_INLINE uint_32 inl(int port);
LS_INLINE void insl(int port, void *addr, int cnt);

/* IDT functions */
LS_INLINE void lidt(void *p);
LS_INLINE void cli();
LS_INLINE void sti();
LS_INLINE void iret();

/* CR function */
LS_INLINE void lcr0(uint_32 val);
LS_INLINE uint_32 rcr0(void);
LS_INLINE void lcr1(uint_32 val);
LS_INLINE uint_32 rcr1(void);
LS_INLINE void lcr2(uint_32 val);
LS_INLINE uint_32 rcr2(void);
LS_INLINE void lcr3(uint_32 val);
LS_INLINE uint_32 rcr3(void);
LS_INLINE void lcr4(uint_32 val);
LS_INLINE uint_32 rcr4(void);


/* Other */
LS_INLINE uint_64 read_tsc();
LS_INLINE void ltr(uint_16 sel);
LS_INLINE uint_16 rtr(void);
LS_INLINE void hlt(void);

LS_INLINE void breakpoint(void);

/*** Implementaciones Inline ***/

/* out** */
LS_INLINE void outb(int port, uint_8 data) {
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

LS_INLINE void outsb(int port, const uint_8 *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\toutsb"		:
		"=S" (addr), "=c" (cnt)		:
		"d" (port), "0" (addr), "1" (cnt)	:
		"cc");
}

LS_INLINE void outw(int port, uint_16 data) {
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

LS_INLINE void outsw(int port, const uint_16 *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\toutsw"		:
		"=S" (addr), "=c" (cnt)		:
		"d" (port), "0" (addr), "1" (cnt)	:
		"cc");
}

LS_INLINE void outl(int port, uint_32 data) {
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

LS_INLINE void outsl(int port, const uint_32 *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

/* in** */

LS_INLINE uint_8 inb(int port) {
	uint_8 data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

LS_INLINE void insb(int port, void *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

LS_INLINE uint_16 inw(int port) {
	uint_16 data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

LS_INLINE void insw(int port, void *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

LS_INLINE uint_32 inl(int port) {
	uint_32 data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

LS_INLINE void insl(int port, void *addr, int cnt) {
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

/* IDT */
LS_INLINE void lidt(void *p) {
	__asm __volatile("lidt (%0)" : : "r" (p));
}

LS_INLINE void sti() {
	__asm__ __volatile__ ("sti");
}

LS_INLINE void cli() {
	__asm__ __volatile__ ("cli");
}

LS_INLINE void iret() {
	__asm__ __volatile__ ("iret": : :"memory");
}

/* Aritmetic */

LS_INLINE uint_32 _clzl(const uint_32 x) {
	uint_32 res;
	__asm__ __volatile__ ("bsr %0, %1": "=q"(res) : "qm"(x)  );
	return res;
}

/* CR function */

LS_INLINE void lcr0(uint_32 val) {
	__asm __volatile("movl %0,%%cr0" : : "r" (val));
}

LS_INLINE uint_32 rcr0(void) {
	uint_32 val;
	__asm __volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

LS_INLINE void lcr1(uint_32 val) {
	__asm __volatile("movl %0,%%cr1" : : "r" (val));
}

LS_INLINE uint_32 rcr1(void) {
	uint_32 val;
	__asm __volatile("movl %%cr1,%0" : "=r" (val));
	return val;
}

LS_INLINE void lcr2(uint_32 val) {
	__asm __volatile("movl %0,%%cr2" : : "r" (val));
}

LS_INLINE uint_32 rcr2(void) {
	uint_32 val;
	__asm __volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

LS_INLINE void lcr3(uint_32 val) {
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

LS_INLINE uint_32 rcr3(void) {
	uint_32 val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

LS_INLINE void lcr4(uint_32 val) {
	__asm __volatile("movl %0,%%cr4" : : "r" (val));
}

LS_INLINE uint_32 rcr4(void) {
	uint_32 cr4;
	__asm __volatile("movl %%cr4,%0" : "=r" (cr4));
	return cr4;
}

LS_INLINE void tlbflush(void) {
	uint_32 cr3;
	__asm __volatile("movl %%cr3,%0" : "=r" (cr3));
	__asm __volatile("movl %0,%%cr3" : : "r" (cr3));
}


/* Other */

LS_INLINE uint_64 read_tsc(void) {
	uint_64 tsc;
	__asm __volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

LS_INLINE void ltr(uint_16 sel) {
	__asm __volatile("ltr %0" : : "r" (sel));
}

LS_INLINE uint_16 rtr(void) {
	uint_16 sel;
	__asm __volatile("str %0" : "=r" (sel) : );
	return sel;
}

LS_INLINE void hlt(void) {
	__asm __volatile("hlt" : : );
}

LS_INLINE void breakpoint(void) {
	__asm __volatile("xchg %%bx, %%bx" : :);
}

#endif

/* ld script to make i386 Linux kernel
 * Written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>;
 */
OUTPUT_FORMAT("a.out-i386-linux")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
  _text = .;			/* Text and read-only data */
  .text : {
	*(.text)
	*(.fixup)
	*(.text.unlikely)
	}

  _etext = .;			/* End of text section */

  .data : {			/* Data */
	*(.data)
	*(.rodata)
	*(.rodata.*)
	CONSTRUCTORS
	}

  _edata = .;			/* End of data section */



  __bss_start = .;		/* BSS */
  .bss : {
	*(.bss)
	}
  /DISCARD/ : {
	*(.comment)
	*(.debug_*)
	*(.eh_frame)
	*(.note.GNU-stack)
	}
  end = . ;

}

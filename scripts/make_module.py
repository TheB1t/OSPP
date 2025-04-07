"""
This script is used to generate a OSPP kernel module.

Module header
    u32 magic;      // magic number 'OSPP' (0x4f535050)
    u8  name[32];   // name of the module
    u8  version;    // version number (latest version is 1)
    u8  type;       // type of module (0: debug symbols, 1: driver, ...)
    u32 size;       // size of the module in bytes
    ...data...
    u32 crc32;      // CRC32 checksum of the module
"""
import enum
import struct
import zlib
import argparse
from elftools.elf.elffile import ELFFile
import subprocess

def demangle(name: str) -> str:
    try:
        result = subprocess.run(['c++filt', name], capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return name

class ModuleType(enum.Enum):
    DEBUG_SYMBOLS   = 0
    
MAGIC   = 0x4F535050    # 'OSPP'
VERSION = 1             # Latest version

def sanitize_name(name: str) -> bytes:
    bname = name.encode('ascii')
    if len(bname) > 32:
        raise ValueError("Module name too long (max 36 bytes)")
    return bname.ljust(32, b'\x00')

def pack_module(name_: str, type_: ModuleType, data_: bytes) -> bytes:
    header_format = '<I32sBBI'  # Magic(4), Name(32), Version(1), Type(1), TotalSize(4)

    name_bytes = sanitize_name(name_)

    header = struct.pack(
        header_format,
        MAGIC,
        name_bytes,
        VERSION,
        type_.value,
        len(data_),
    )

    crc_data = header + data_
    crc = zlib.crc32(crc_data, 0xFFFFFFFF)
    print(f"Module {name_} CRC32: {crc:08x}")
    crc_bytes = struct.pack('<I', crc)

    full_module = crc_data + crc_bytes

    return full_module

def build_debug_symbols(elf_path: str) -> bytes:
    symbols = []
    strtab = bytearray()
    strtab_offsets = {}

    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        symtab = elf.get_section_by_name('.symtab')
        if not symtab:
            raise ValueError("No symbol table found in ELF")

        for symbol in symtab.iter_symbols():
            if symbol['st_info']['type'] != 'STT_FUNC':
                continue
            addr = symbol['st_value']
            raw_name = symbol.name
            name = demangle(raw_name)

            # Deduplicate strings in strtab
            if name not in strtab_offsets:
                offset = len(strtab)
                strtab_offsets[name] = offset
                strtab += name.encode('utf-8') + b'\x00'
            else:
                offset = strtab_offsets[name]

            symbols.append((addr, offset))

    entry_count = len(symbols)

    data = bytearray()
    for addr, offset in symbols:
        data += struct.pack('<II', addr, offset)

    header_format = '<II'
    strtab_offset = len(data) + struct.calcsize(header_format)
    header = struct.pack(header_format, entry_count, strtab_offset)

    return header + data + strtab

def handle_debug_symbols(args):
    data = build_debug_symbols(args.input)
    module = pack_module(args.name, ModuleType.DEBUG_SYMBOLS, data)
    filename = args.name + ".ko"
    output_path = filename
    
    if args.output_path:
        output_path = args.output_path + filename
        
    with open(output_path, "wb") as f:
        f.write(module)
        
    print(f"Debug symbol module written to {output_path}")

def main():
    parser = argparse.ArgumentParser(description="OSPP Kernel Module Builder")
    subparsers = parser.add_subparsers(dest="command", required=True)

    dbg = subparsers.add_parser("debug_symbols", help="Generate debug symbol module from ELF")
    dbg.add_argument("input", help="Input ELF file")
    dbg.add_argument("name", help="Module name")
    dbg.add_argument("--output-path", help="Output path for the module")
    dbg.set_defaults(func=handle_debug_symbols)
    
    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
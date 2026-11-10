import os
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../gfx/tools'))
import uepak


def fstr(value):
    data = value.encode() + b'\0'
    return struct.pack('<i', len(data)) + data


def make_v9(entries, frozen=0):
    """A small non-frozen v9 pak, with legacy index entries and the v9 footer fields."""
    body = bytearray()
    index = bytearray(fstr('../') + struct.pack('<i', len(entries)))
    for name, raw, method in entries:
        off = len(body)
        if method:
            stored = zlib.compress(raw)
            blocks = [(73, 73 + len(stored))]
            payload = struct.pack('<qqqi', off, len(stored), len(raw), method) + bytes(20)
            payload += struct.pack('<iqqBI', len(blocks), *blocks[0], 0, 65536)
            payload += stored
            size = len(stored)
        else:
            stored = raw
            payload = struct.pack('<qqqi', off, len(raw), len(raw), 0) + bytes(20) + struct.pack('<BI', 0, 0)
            payload += stored
            size = len(raw)
        body += payload
        index += fstr(name) + struct.pack('<qqqi', off, size, len(raw), method) + bytes(20)
        if method:
            index += struct.pack('<i', 1)
            index += struct.pack('<qq', *blocks[0])
        index += struct.pack('<BI', 0, 65536 if method else 0)
    ioff = len(body)
    body += index
    footer = bytes(16) + b'\0' + struct.pack('<IIQQ', uepak.PAK_MAGIC, 9, ioff, len(index)) + bytes(20)
    footer += bytes([frozen]) + b'Zlib\0'.ljust(32, b'\0') + bytes(32 * 4)
    return bytes(body) + footer


def make_frozen_v9():
    """A frozen v9 index with one stored file and one directory-map slot."""
    b = bytearray(192)
    def ptr(pos, target): struct.pack_into('<Q', b, pos, ((target - pos) << 1) | 1)
    def fstr(pos, text):
        encoded = text.encode('utf-16-le') + b'\0\0'; struct.pack_into('<Qii', b, pos, 0, len(encoded) // 2, len(encoded) // 2)
        return pos, encoded
    # Fixed memory-image headers: mount FString, entries TArray, and directory TMap.
    mount = fstr(0, '../')
    struct.pack_into('<Qii', b, 16, 0, 1, 1)
    struct.pack_into('<Qii', b, 32, 0, 1, 1)
    struct.pack_into('<Qii', b, 48, 0, 1, 1)
    struct.pack_into('<ii', b, 64, -1, 0)
    struct.pack_into('<QI', b, 72, 0, 1)
    # One directory/name sparse slot. A nested TMap occupies 56 bytes; each pair has an 8-byte hash tail.
    directory_header = fstr(88, '/')
    struct.pack_into('<Qii', b, 104, 0, 1, 1)
    struct.pack_into('<Qii', b, 120, 0, 1, 1)
    struct.pack_into('<ii', b, 136, -1, 0)
    struct.pack_into('<QI', b, 144, 0, 1)
    name_header = fstr(160, 'file.bin')
    struct.pack_into('<i', b, 176, 0)
    # Append all pointed-to payloads and patch their relative pointers.
    def append(data):
        while len(b) & 7: b.append(0)
        pos = len(b); b.extend(data); return pos
    bits_outer = append(struct.pack('<I', 1)); ptr(48, bits_outer)
    bits_inner = append(struct.pack('<I', 1)); ptr(120, bits_inner)
    ptr(32, 88)
    ptr(104, 160)
    entries = bytearray(80); struct.pack_into('<qqq', entries, 0, 0, 5, 5); struct.pack_into('<IIB', entries, 64, 0, 0, 0)
    entries_at = append(entries); ptr(16, entries_at)
    mount_at = append(mount[1]); ptr(0, mount_at)
    dir_at = append(directory_header[1]); ptr(directory_header[0], dir_at)
    name_at = append(name_header[1]); ptr(name_header[0], name_at)
    b[72:80] = bytes(8); b[144:152] = bytes(8)   # empty hash bucket pointers; sizes above remain one
    return bytes(b)


class PakV9Tests(unittest.TestCase):
    def test_footer_and_legacy_index_method_mapping(self):
        raw = b'v9 compressed payload'
        data = make_v9([('plain.bin', b'plain', 0), ('compressed.bin', raw, 1)])
        with tempfile.NamedTemporaryFile() as f:
            f.write(data)
            f.flush()
            pak = uepak.Pak(f.name)
            try:
                self.assertEqual(uepak.magic_back(pak.f), (0xcd, 9))
                self.assertEqual(pak.version, 9)
                self.assertEqual(pak.methods[0], 'Zlib')
                self.assertEqual(len(pak.files), 2)
                self.assertEqual(pak.entry('plain.bin')['method'], '')
                self.assertEqual(pak.entry('compressed.bin')['method'], 'Zlib')
                self.assertEqual(pak.read('plain.bin'), b'plain')
                self.assertEqual(pak.read('compressed.bin'), raw)
            finally:
                pak.f.close()

    def test_negative_compression_method_is_rejected(self):
        data = make_v9([('plain.bin', b'plain', -1)])
        with tempfile.NamedTemporaryFile() as f:
            f.write(data)
            f.flush()
            opened = []
            real_open = uepak.open
            def track_open(*args, **kwargs):
                opened.append(real_open(*args, **kwargs))
                return opened[-1]
            with patch.object(uepak, 'open', side_effect=track_open):
                with self.assertRaisesRegex(ValueError, 'compression method -1'):
                    uepak.Pak(f.name)
            for stream in opened: stream.close()

    def test_frozen_index_pointer_and_map_layout(self):
        index = make_frozen_v9()
        body = struct.pack('<qqqi', 0, 5, 5, 0) + bytes(20) + struct.pack('<BI', 0, 0) + b'plain'
        ioff = len(body)
        footer = bytes(16) + b'\0' + struct.pack('<IIQQ', uepak.PAK_MAGIC, 9, ioff, len(index)) + bytes(20) + b'\1'
        footer += b'Zlib\0'.ljust(32, b'\0') + bytes(32 * 4)
        with tempfile.NamedTemporaryFile() as f:
            f.write(body + index + footer)
            f.flush()
            pak = uepak.Pak(f.name)
            try:
                self.assertTrue(pak.frozen)
                self.assertEqual(len(pak.files), 1)
                self.assertEqual(pak.entry('file.bin')['method'], '')
                self.assertEqual(pak.read('file.bin'), b'plain')
            finally:
                pak.f.close()

    def test_frozen_map_checks_unallocated_slots_and_hash_tail(self):
        b = bytearray(64)
        struct.pack_into('<Qii', b, 0, (100000 << 1) | 1, 1, 1)
        struct.pack_into('<Qii', b, 16, (52 - 16) * 2 | 1, 1, 1)
        struct.pack_into('<ii', b, 32, -1, 0)
        struct.pack_into('<QI', b, 40, 0, 1)
        struct.pack_into('<I', b, 52, 0)  # slot is unallocated; its declared storage must still fit
        with self.assertRaisesRegex(ValueError, 'do not fit'):
            uepak.frozen_map(uepak.R(bytes(b)), lambda: None, lambda: None, 16, 4)
        truncated = uepak.R(bytes(40))
        with self.assertRaisesRegex(ValueError, 'do not fit'):
            uepak.frozen_map(truncated, lambda: None, lambda: None, 16, 4)


if __name__ == '__main__':
    unittest.main()

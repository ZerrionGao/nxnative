"""shader_split.linkdata_index: the index file beside a LINKDATA archive.

Three Houses uses DATA0.bin beside DATA1.bin, Pokémon Pokopia uses LI_<name>.IDX beside LD_<name>.BIN,
and Shin Sangoku Musou 7 with Moushouden DX uses LINKDATA.IDX beside LINKDATA.BIN. The records are the same; only the names differ.
"""
import os
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / 'dm/gfx/tools'))
_had = os.environ.get('DM_GAME'); os.environ['DM_GAME'] = '010055D009F78000'   # shader_split loads a profile at import; other tests expect no default game
import shader_split as ss
if _had is None: del os.environ['DM_GAME']
else: os.environ['DM_GAME'] = _had


class LinkdataIndex(unittest.TestCase):
    def test_known_pairs(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ('DATA0.bin', 'DATA1.bin', 'LINKDATA.IDX', 'LINKDATA.BIN', 'LI_C0_R.IDX', 'LD_C0_R.BIN'):
                Path(d, name).write_bytes(b'\0' * 32)
            self.assertEqual(ss.linkdata_index(str(Path(d, 'DATA1.bin'))), str(Path(d, 'DATA0.bin')))
            self.assertEqual(ss.linkdata_index(str(Path(d, 'LINKDATA.BIN'))), str(Path(d, 'LINKDATA.IDX')))
            self.assertEqual(ss.linkdata_index(str(Path(d, 'LD_C0_R.BIN'))), str(Path(d, 'LI_C0_R.IDX')))
            self.assertIsNone(ss.linkdata_index(str(Path(d, 'DATA0.bin'))))
            Path(d, 'LINKDATA.IDX').unlink()
            self.assertIsNone(ss.linkdata_index(str(Path(d, 'LINKDATA.BIN'))))


if __name__ == '__main__':
    unittest.main()

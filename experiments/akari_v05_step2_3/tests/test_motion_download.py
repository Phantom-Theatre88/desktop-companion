import importlib.util, io, tempfile, unittest, urllib.error
from pathlib import Path
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / 'akari_motion_test/tools/install_cc0_motion.py'
spec = importlib.util.spec_from_file_location('motion_source', MODULE)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class Response(io.BytesIO):
    headers = {'Content-Type':'application/zip'}
    def __enter__(self):return self
    def __exit__(self, *args):self.close()

class DownloadTest(unittest.TestCase):
    def test_dns_failures_show_all_sources_and_offline_path(self):
        def no_network(request,timeout):
            raise urllib.error.URLError('[Errno -3] Temporary failure in name resolution')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'source.zip'
            with self.assertRaises(RuntimeError) as c:
                mod.download_bundle(p,sources=[('source1','https://a.test/archive'),('source2','https://b.test/archive')],opener=no_network)
            self.assertIn('source1:',str(c.exception))
            self.assertIn('source2:',str(c.exception))
            self.assertIn('Place it at',str(c.exception))
            self.assertFalse(p.exists())
    def test_second_source_is_used_after_first_http_error(self):
        calls=[]
        def opener(req,timeout):
            calls.append(req.full_url)
            if len(calls)==1:raise urllib.error.HTTPError(req.full_url, 429, 'throttle',{},None)
            return Response(b'valid-source-fixture')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'source.zip'
            # This test exercises network fallback. Real bytes are verified separately.
            with patch.object(mod, 'check_bundle', lambda p: None):
                self.assertEqual(mod.download_bundle(p,sources=[('a','https://a.test/archive'),('b','https://b.test/archive')],opener=opener),p)
            self.assertEqual(len(calls),2)
            self.assertEqual(p.read_bytes(),b'valid-source-fixture')
    def test_wrong_source_hash_never_installs(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'source.zip'
            def opener(req,timeout):return Response(b'forged bytes')
            with self.assertRaises(RuntimeError) as c:
                mod.download_bundle(p,sources=[('untrusted','https://test.invalid/fake')],opener=opener)
            self.assertIn('SHA256 mismatch',str(c.exception))
            self.assertFalse(p.exists())
    def test_cached_wrong_hash_fails_before_build(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'source.zip';p.write_bytes(b'invalid')
            with self.assertRaisesRegex(ValueError,'SHA256 mismatch'):
                mod.download_bundle(p,opener=lambda *a,**kw:None)
if __name__=='__main__':unittest.main()
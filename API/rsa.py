"""Wrapper Python (ctypes) para a biblioteca FunRSA (rsa.dll / FunRSA.so)."""
import ctypes
import os
import sys

_pasta = os.path.dirname(os.path.abspath(__file__))
_nomes = ["rsa.dll", "FunRSA.dll"] if sys.platform == "win32" else ["FunRSA.so", "librsa.so"]
_lib = None
for _n in _nomes:
    _caminho = os.path.join(_pasta, _n)
    if os.path.exists(_caminho):
        _lib = ctypes.CDLL(_caminho)
        break
if _lib is None:
    raise OSError("Biblioteca RSA nao encontrada em " + _pasta)

_VP = ctypes.c_void_p
_CP = ctypes.c_char_p
_PP = ctypes.POINTER(ctypes.c_void_p)   # char ** recebido como ponteiro opaco (para poder liberar)

_lib.rsa_criar.argtypes = [ctypes.c_int]
_lib.rsa_criar.restype = _VP
_lib.rsa_destruir.argtypes = [_VP]
_lib.rsa_destruir.restype = None
_lib.rsa_liberar.argtypes = [_VP]
_lib.rsa_liberar.restype = None
_lib.rsa_define_primos.argtypes = [_VP, _CP, _CP, _CP, ctypes.c_int]
_lib.rsa_define_primos.restype = ctypes.c_int
_lib.rsa_chave_publica.argtypes = [_VP, _CP, _PP, _PP, ctypes.c_int]
_lib.rsa_chave_publica.restype = ctypes.c_int
_lib.rsa_chave_privada.argtypes = [_VP, _CP, _PP, _PP, ctypes.c_int]
_lib.rsa_chave_privada.restype = ctypes.c_int
_lib.rsa.argtypes = [ctypes.c_int, _VP, _CP, _CP, _PP, ctypes.c_int]
_lib.rsa.restype = ctypes.c_int

ETAPAS = {
    1: "verifica_p_q (p e q invalidos)",
    2: "calcula_bases (n fora do limite)",
    3: "chave_publica",
    4: "chave_privada",
    5: "criptografar",
    6: "descriptografar (texto cifrado/chave invalidos)",
    7: "extrai_chaves",
    -1: "opcao invalida",
    -2: "ponteiro s_RSA nulo",
}


class RSAError(Exception):
    def __init__(self, codigo):
        self.codigo = codigo
        super().__init__(f"Erro {codigo}: {ETAPAS.get(codigo, 'desconhecido')}")


def _enc(s):
    return None if s is None else s.encode("utf-8")


def _pegar(ptr):
    """Copia a string alocada pelo C e libera a memoria original."""
    if not ptr.value:
        return None
    try:
        return ctypes.string_at(ptr.value).decode("utf-8")
    finally:
        _lib.rsa_liberar(ptr)


class RSA:
    def __init__(self, bits=2048):
        self._h = _lib.rsa_criar(bits)
        if not self._h:
            raise MemoryError("rsa_criar falhou")

    def __del__(self):
        if getattr(self, "_h", None):
            _lib.rsa_destruir(self._h)
            self._h = None

    def definir_primos(self, p=None, q=None, formato=None, debug=False):
        """p e q informados pelo usuario. Sem argumentos: gerar aleatoriamente."""
        r = _lib.rsa_define_primos(self._h, _enc(p), _enc(q), _enc(formato), int(debug))
        if r != 0:
            raise RSAError(r)

    def gerar_chaves(self, debug=False):
        r = _lib.rsa(3, self._h, None, None, None, int(debug))
        if r != 0:
            raise RSAError(r)

    def chave_publica(self, formato=None, debug=False):
        e, n = ctypes.c_void_p(), ctypes.c_void_p()
        r = _lib.rsa_chave_publica(self._h, _enc(formato), ctypes.byref(e), ctypes.byref(n), int(debug))
        if r != 0:
            raise RSAError(r)
        return _pegar(e), _pegar(n)

    def chave_privada(self, formato=None, debug=False):
        d, n = ctypes.c_void_p(), ctypes.c_void_p()
        r = _lib.rsa_chave_privada(self._h, _enc(formato), ctypes.byref(d), ctypes.byref(n), int(debug))
        if r != 0:
            raise RSAError(r)
        return _pegar(d), _pegar(n)

    def criptografar(self, mensagem, formato=None, debug=False):
        """formato: None/'decimal' (padrao), 'hex' ou 'base64'."""
        saida = ctypes.c_void_p()
        r = _lib.rsa(1, self._h, _enc(mensagem), _enc(formato), ctypes.byref(saida), int(debug))
        if r != 0:
            raise RSAError(r)
        return _pegar(saida)

    def descriptografar(self, cifrado, formato=None, debug=False):
        saida = ctypes.c_void_p()
        r = _lib.rsa(2, self._h, _enc(cifrado), _enc(formato), ctypes.byref(saida), int(debug))
        if r != 0:
            raise RSAError(r)
        return _pegar(saida)


if __name__ == "__main__":
    r = RSA(2048)
    r.gerar_chaves()
    e, n = r.chave_publica("hex")
    print("e =", e, "\nn =", n[:64] + "...")
    for fmt in (None, "hex", "base64"):
        c = r.criptografar("Texto digitado em tempo de execucao", fmt)
        print(f"[{fmt or 'decimal'}]", c[:70] + "...")
        print("   ->", r.descriptografar(c, fmt))

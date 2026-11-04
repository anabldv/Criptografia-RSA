"""Wrapper Python (ctypes) para a FunRSA.dll."""
import ctypes
import os
import sys

_pasta = os.path.dirname(os.path.abspath(__file__))
_nome = "FunRSA.dll" if sys.platform == "win32" else "FunRSA.so"
_lib = ctypes.CDLL(os.path.join(_pasta, _nome))

_lib.rsa_gerar_chaves.argtypes = [ctypes.c_int]
_lib.rsa_gerar_chaves.restype = ctypes.c_int
_lib.rsa_obter_n.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.rsa_obter_n.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.rsa_obter_n.restype = ctypes.c_int
_lib.rsa_obter_e.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.rsa_obter_e.restype = ctypes.c_int
_lib.rsa_obter_d.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.rsa_obter_d.restype = ctypes.c_int
_lib.rsa_definir_chave_privada.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.rsa_definir_chave_privada.restype = ctypes.c_int
_lib.rsa_criptografar.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_lib.rsa_criptografar.restype = ctypes.c_int
_lib.rsa_descriptografar.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_lib.rsa_descriptografar.restype = ctypes.c_int

ERROS = {
    -1: "Nenhuma chave carregada. Gere as chaves primeiro.",
    -2: "Buffer de saida pequeno demais.",
    -3: "Entrada invalida (formato do texto cifrado ou da chave).",
    -4: "Falha na descriptografia: chave incorreta ou texto cifrado corrompido.",
    -5: "Nao foi possivel calcular a chave privada.",
    -6: "Falha no gerador de numeros aleatorios do sistema.",
}


class RSAError(Exception):
    pass


def _checar(codigo):
    if codigo < 0:
        raise RSAError(ERROS.get(codigo, f"Erro {codigo}"))
    return codigo


def gerar_chaves(bits=2048):
    _checar(_lib.rsa_gerar_chaves(bits))


def _obter(fn):
    buf = ctypes.create_string_buffer(1100)
    _checar(fn(buf, len(buf)))
    return buf.value.decode()


def obter_n(): return _obter(_lib.rsa_obter_n)
def obter_e(): return _obter(_lib.rsa_obter_e)
def obter_d(): return _obter(_lib.rsa_obter_d)


def definir_chave_privada(n_hex, d_hex):
    _checar(_lib.rsa_definir_chave_privada(n_hex.strip().encode(), d_hex.strip().encode()))


def criptografar(mensagem: str) -> str:
    dados = mensagem.encode("utf-8")
    buf = ctypes.create_string_buffer(len(dados) * 3 + 1024)
    _checar(_lib.rsa_criptografar(dados, buf, len(buf)))
    return buf.value.decode()


def descriptografar(cifrado_hex: str) -> str:
    dados = cifrado_hex.strip().encode()
    buf = ctypes.create_string_buffer(len(dados) // 2 + 16)
    n = _checar(_lib.rsa_descriptografar(dados, buf, len(buf)))
    return buf.raw[:n].decode("utf-8", errors="replace")
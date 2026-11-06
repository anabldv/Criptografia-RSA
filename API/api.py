import threading
from typing import Optional

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel

import rsa as R  # wrapper da .dll / .so

app = FastAPI(title="API FunRSA")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # em produção, troque pela URL do frontend
    allow_methods=["*"],
    allow_headers=["*"],
)

# Uma única instância RSA.
cifra = R.RSA(2048)
trava = threading.Lock() # Evita duas requisições mexendo ao mesmo tempo.
estado = {"pq": None, "chaves": False}


# Qualquer erro vindo da DLL vira uma resposta 400 com a mensagem
@app.exception_handler(R.RSAError)
def erro_rsa(request, exc):
    return JSONResponse(status_code=400, content={"detail": str(exc)})


class Criptografar(BaseModel):
    texto: str
    p: Optional[str] = None   # string para não perder precisão em primos grandes
    q: Optional[str] = None


class Descriptografar(BaseModel):
    texto: str


def preparar_chaves(p=None, q=None):
    """Gera as chaves. se p/q mudaram ou se ainda não existem."""
    p = (p or "").strip() or None
    q = (q or "").strip() or None
    if (p is None) != (q is None):
        raise HTTPException(status_code=400, detail="Informe p e q juntos ou deixe os dois vazios.")

    pq = (p, q) if p else None
    if pq != estado["pq"] or not estado["chaves"]:
        estado["chaves"] = False
        cifra.definir_primos(p, q)   # se None, None vai sorteia aleatoriamente
        cifra.gerar_chaves()
        estado["pq"] = pq
        estado["chaves"] = True


def garantir_chaves():
    if not estado["chaves"]:
        preparar_chaves()


@app.post("/criptografar")
def criptografar(dados: Criptografar):
    with trava:
        preparar_chaves(dados.p, dados.q)
        return {"resultado": cifra.criptografar(dados.texto)}   # formato padrão (decimal)


@app.post("/descriptografar")
def descriptografar(dados: Descriptografar):
    with trava:
        garantir_chaves()
        return {"resultado": cifra.descriptografar(dados.texto.strip())}


@app.get("/chave-publica")
def chave_publica():
    with trava:
        garantir_chaves()
        e, n = cifra.chave_publica()
        return {"e": e, "n": n[0:9]}


@app.get("/chave-privada")
def chave_privada():
    with trava:
        garantir_chaves()
        d, n = cifra.chave_privada()
        return {"d": n[0:9], "n": n[0:9]}
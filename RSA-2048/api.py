from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import rsa as R  # Importa o seu wrapper da .dll

app = FastAPI(title="API FunRSA")

# Modelos de dados esperados no corpo (body) das requisições JSON
class Mensagem(BaseModel):
    texto: str

class Cifrado(BaseModel):
    hex_cifrado: str

class ChavePrivada(BaseModel):
    n_hex: str
    d_hex: str

@app.post("/gerar-chaves")
def api_gerar_chaves(bits: int = 2048):
    try:
        print("gerar-chaves")
        R.gerar_chaves(bits)
        return {
            "status": "sucesso",
            "n": R.obter_n(),
            "e": R.obter_e(),
            "d": R.obter_d()
        }
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.get("/obter-chaves")
def api_obter_chaves():
    try:
        print("obter-chaves")
        return {
            "n": R.obter_n(),
            "e": R.obter_e(),
            "d": R.obter_d()
        }
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/definir-chave")
def api_definir_chave(chave: ChavePrivada):
    print("definir-chave")
    try:
        R.definir_chave_privada(chave.n_hex, chave.d_hex)
        return {"status": "chave carregada com sucesso"}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/criptografar")
def api_criptografar(msg: Mensagem):
    print("criptografar")
    try:
        cifrado = R.criptografar(msg.texto)
        return {"cifrado": cifrado}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/descriptografar")
def api_descriptografar(cifrado: Cifrado):
    print("descriptografar")
    try:
        texto = R.descriptografar(cifrado.hex_cifrado)
        return {"texto": texto}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))
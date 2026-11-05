from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import rsa as R  # Importa o seu wrapper da .dll

app = FastAPI(title="API FunRSA")

# Modelos de dados esperados no corpo (body) das requisições JSON
class Mensagem(BaseModel):
    texto: str

class Cifrado(BaseModel):
    hex_cifrado: str
    p: int
    q: int

class ChavePrivada(BaseModel):
    n_hex: str
    d_hex: str

@app.post("/criptografar")
def api_gerar_chaves(texto: Cifrado):
    try:
        print("criptografar")
        cifrado = R.criptografar(texto.hex_cifrado)
        return {"cifrado": cifrado}
    
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/descriptografar")
def api_obter_chaves(msg: str):
    print("descriptografar")
    try:
        texto = R.descriptografar(msg)
        return {"cifrado": texto}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.get("/exibir-publica")
def api_definir_chave():
    print("exibir-publica")
    try:
        return {
            "e": R.obter_e(), 
            "n": R.obter_n()
        }
    except Exception as er:
        raise HTTPException(status_code=400, detail=str(er))

@app.post("/exibir-privada")
def api_criptografar(ChavePrivada: ChavePrivada):
    print("criptografar")
    try:
        return {
            "status": "sucesso",
            "n": R.obter_n(),
            "d": R.obter_d()
        }
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

# Criptografia-RSA
Trabalho de Segurança da informação

Para criar um ambiente virtual no python
```bash
python -m venv .venv
```

Ativar o ambiente virtual 
```bash
.venv\Scripts\Activate.ps1
ou
.venv\bin\Activate.ps1
```

Instalar as dependencias 
```bash
pip install -r requer.txt
```

Subir a API localmente
```bash
uvicorn api:app --reload
```

## com usar a aplicação

Dentro da pasta ***API*** abra o terminal e digite o seguinte comando

```bash
uvicorn api:app --reload
```

Apos subir a API entre na pasta ***frontend*** e abra o arquivo index.html
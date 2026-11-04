# HTML  
<!DOCTYPE html>  
<html lang="pt-BR">  
<head>  
  <meta charset="UTF-8">  
  <meta name="viewport" content="width=device-width, initial-scale=1.0">  
  <title>Criptografia & Descriptografia</title>  
  <link rel="preconnect" href="https://fonts.googleapis.com">  
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>  
  <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600;700&display=swap" rel="stylesheet">  
  <link rel="stylesheet" href="style.css">  
</head>  
<body>  
  <div class="container">  
    <header class="header">  
      <div class="terminal-badge">SYS::ENCRYPT</div>  
      <h1>Criptografia RSA</h1>  
    </header>  
  
    <main class="panel">  
      <!-- Entrada -->  
      <section class="box-group">  
        <div class="box-label">  
          <span>ENTRADA</span>  
          <span id="inputCount" class="counter">0 caracteres</span>  
        </div>  
        <textarea id="inputText" placeholder="Digite ou cole sua mensagem aqui..."></textarea>  
      </section>  
            
        <div class="action-buttons">  
          <button id="btnEncrypt" class="btn primary">Criptografar</button>  
          <button id="btnDecrypt" class="btn secondary">Descriptografar</button>  
          <button id="btnClear" class="btn ghost">Limpar</button>  
        </div>  
      </section>  
  
      <!-- Saída -->  
      <section class="box-group">  
        <div class="box-label">  
          <span>RESULTADO</span>  
          <span id="outputCount" class="counter">0 caracteres</span>  
        </div>  
        <div class="output-wrapper">  
          <textarea id="outputText" readonly placeholder="O resultado aparecerá aqui..."></textarea>  
          <button id="btnCopy" class="btn-copy" title="Copiar resultado">  
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">  
              <rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect>  
              <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path>  
            </svg>  
            <span id="copyFeedback">Copiar</span>  
          </button>  
        </div>  
      </section>  
    </main>  
  </div>  
  
  <script src="script.js"></script>  
</body>  
</html>  

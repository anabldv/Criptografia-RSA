// Campos de textos
const entradaTexto = document.getElementById('entradaTexto');
const saidaTexto = document.getElementById('saidaTexto');
const entradaContador = document.getElementById('entradaContador');
const saidaContador = document.getElementById('saidaContador');

// Variaveis Numericas
const selecP = document.getElementById('numeroP');
const selecQ = document.getElementById('numeroQ');

// Botões
const btnCripto = document.getElementById('btnCripto');
const btnDecripto = document.getElementById('btnDecripto');
const btnLimpa = document.getElementById('btnLimpa');
const btnCopiar = document.getElementById('btnCopiar');
const btnPublica= document.getElementById('btnPublico');
const btnPrivada = document.getElementById('btnPrivado');
const caixaCopiar = document.getElementById('caixaCopiar');

// Mensagem
const Feedback = document.getElementById('Feedback');


// Atualização de contadores em tempo real
function updateCounters() {
    entradaContador.textContent = `${entradaTexto.value.length} caracteres`;
    saidaContador.textContent = `${saidaTexto.value.length} caracteres`;
}

entradaTexto.addEventListener('input', updateCounters);


// Processamento
btnCripto.addEventListener('click', () => {                                     // Criptografia
    const texto = entradaTexto.value;

    if (!texto) return mostrarMensagem("O campo texto está vazio");

    const cifra = {
        cifra: texto,
        p: selecP.value,
        q: selecQ.value
    };

    try {
        const resposta = fetch('http://127.0.0.1:8000/criptografar', {
            method: 'POST', // Define o método HTTP
            headers: {
                'Content-Type': 'application/json' // Avisa a API que estamos enviando um JSON
            },
            // Converte o objeto JavaScript em uma string JSON
            body: JSON.stringify(cifra) 
        });

        if (!resposta.ok) {
            throw new Error(`Erro HTTP: ${resposta.status}`);
        }
        
        const dados = resposta.json();
        saidaTexto.value = dados.texto;

    } catch (err) {
        mostrarMensagem(`[Erro na criptografia]: ${err.message}`);
    }
    updateCounters();
});

btnDecripto.addEventListener('click', () => {                                   // Descriptografia
    const text = entradaTexto.value;
    if (!text) return;
    const method = methodSelect.value;
    const shift = cesarShift.value;

    try {
        const resposta = fetch('http://127.0.0.1:8000//descriptografar', {
        method: 'POST', // Define o método HTTP
        headers: {
            'Content-Type': 'application/json' // Avisa a API que estamos enviando um JSON
        },
            // Converte o objeto JavaScript em uma string JSON
            body: JSON.stringify(cifra) 
        });

        if (!resposta.ok) {
            throw new Error(`Erro HTTP: ${resposta.status}`);
        }
        
        const dados = resposta.json();
        saidaTexto.value = dados.texto;
    } catch (err) {
    saidaTexto.value = `[Erro]: Formato inválido ou corrompido para o método selecionado.`;
    }
    updateCounters();
});

btnLimpa.addEventListener('click', () => {                                      // Limpar caixa de texto
    entradaTexto.value = '';
    saidaTexto.value = '';
    updateCounters();
    entradaTexto.focus();
});

btnPublica.addEventListener('click', () => {                                    // Exibir Chaves publicas
    try {
        const resposta = fetch('http://127.0.0.1:8000//exibir-publica');
        
        if (!resposta.ok) {
            throw new Error(`Erro HTTP: ${resposta.status}`);
        }
        
        const dados = resposta.json();
        console.log(dados);
    } catch (err) {
    saidaTexto.value = `[Erro]: Formato inválido ou corrompido para o método selecionado.`;
    }
    updateCounters();
});

btnPrivada.addEventListener('click', () => {                                    // Exibir Chaves privadas
    const text = entradaTexto.value;
    if (!text) return;

    try {
        const resposta = fetch('http://127.0.0.1:8000/exibir-privada', {
              method: 'POST', // Define o método HTTP
              headers: {
            'Content-Type': 'application/json' // Avisa a API que estamos enviando um JSON
        },
            // Converte o objeto JavaScript em uma string JSON
            body: JSON.stringify(cifra) 
        });
        if (!resposta.ok) {
            throw new Error(`Erro HTTP: ${resposta.status}`);
        }
        
        const dados = resposta.json();
        saidaTexto.value = dados.texto;
    } catch (err) {
    saidaTexto.value = `[Erro]: Formato inválido ou corrompido para o método selecionado.`;
    }
    updateCounters();
});

// Cópia para a área de transferência com fallback
btnCopiar.addEventListener('click', () => {                                 
    if (!saidaTexto.value) return;

    saidaTexto.select();
    saidaTexto.setSelectionRange(0, 99999);

    let success = false;
    try {
    success = document.execCommand('copy');
    } catch (e) {
    success = false;
    }

    if (!success && navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(saidaTexto.value).then(showCopiedState);
    } else if (success) {
    showCopiedState();
    }
});

function showCopiedState() {
    btnCopiar.classList.add('copied');
    caixaCopiar.textContent = 'Copiado!';
    setTimeout(() => {
    btnCopiar.classList.remove('copied');
    caixaCopiar.textContent = 'Copiar';
    }, 1800);
}

function mostrarMensagem(mensagem) {
    // Define o texto da mensagem
    Feedback.textContent = mensagem;
    
    // Adiciona a classe para mostrar o elemento na tela
    Feedback.classList.add('show');
    
    // Remove a classe após 3000 milissegundos (3 segundos)
    setTimeout(() => {
        Feedback.classList.remove('show');
    }, 3000);
}










let keyRSA = { e: 0, d: 0, n: 0 };

async function gerarChaves() {
    const response = await fetch('http://127.0.0.1:8000/gerar-chaves?bits=2048', { method: 'POST' });
    const data = await response.json();
    
    // Exibe apenas o começo das chaves geradas pela DLL para não quebrar o layout
    document.getElementById('public-key').innerText = `(${data.e}, ${data.n.substring(0, 15)}...)`;
    document.getElementById('private-key').innerText = `(${data.d.substring(0, 15)}..., n)`;
}

async function descriptografar() {
    const cifradoHex = document.getElementById('cipherText').value.trim();
    
    const response = await fetch('http://127.0.0.1:8000/descriptografar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ hex_cifrado: cifradoHex })
    });
    
    const data = await response.json();
    document.getElementById('decryptedText').value = data.texto || "Erro na descriptografia";
}

async function criptografar() {
    // Supondo que criou um <textarea id="plainText"> no seu HTML
    const textoOriginal = document.getElementById('plainText').value.trim();
    
    if (!textoOriginal) return;

    const response = await fetch('http://127.0.0.1:8000/criptografar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ texto: textoOriginal })
    });
    
    const data = await response.json();
    
    // Coloca o resultado hexadecimal no campo de texto cifrado
    document.getElementById('cipherText').value = data.cifrado || "Erro na criptografia";
}


// Gera as chaves ao carregar a página com os valores padrão (61 e 53)
gerarChaves();
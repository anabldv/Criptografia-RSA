const API = 'http://127.0.0.1:8000';

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
const btnPublica= document.getElementById('btnPublico');
const btnPrivada = document.getElementById('btnPrivado');

// Mensagem
const Feedback = document.getElementById('Feedback');

let texto_cripto;
let texto_descripto;
let foi_cripto;


// Atualização de contadores em tempo real
function updateCounters() {
    entradaContador.textContent = `${entradaTexto.value.length} caracteres`;
    saidaContador.textContent = `${saidaTexto.value.length} caracteres`;
}

// Feedback de msg
function mostrarMensagem(mensagem) {
    Feedback.textContent = mensagem;
    Feedback.classList.add('show');
    setTimeout(() => Feedback.classList.remove('show'), 3000);
}

// ======================================================================= Funções de requisições =========================================
// Faz a chamada à API. 
async function chamarApi(rota, corpo) {
    const opcoes = corpo
        ? { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(corpo) } // Com corpo é um POST com JSON.
        : {}; // Sem corpo é um GET
 
    const resposta = await fetch(API + rota, opcoes); // 'http://127.0.0.1:8000' + ("/criptografar", "/descriptografar", "/chave-publica", "/chave-privada")
    const dados = await resposta.json();
 
    if (!resposta.ok) {
        const detalhe = typeof dados.detail === 'string' ? dados.detail : JSON.stringify(dados.detail);
        throw new Error(detalhe || `Erro HTTP ${resposta.status}`);
    }
    return dados;
}
 
// Executa uma ação, mostra o erro e atualiza os contadores
async function executar(acao) {
    try {
        await acao();
    } catch (err) {
        mostrarMensagem(err.message);
    }
    updateCounters();
}


entradaTexto.addEventListener('input', updateCounters);

// ====================================================================================== Criptografia
btnCripto.addEventListener('click', () => executar (async () => {                                    
    const texto = entradaTexto.value;
    
    if (!texto) return mostrarMensagem("O campo texto está vazio");

    const dados = await chamarApi('/criptografar', {
        texto: texto,
        p: selecP.value,   // string; vazio = a API sorteia
        q: selecQ.value
    });
    saidaTexto.value = dados.resultado;
    texto_cripto = texto

    updateCounters();
}));

// ====================================================================================== Descriptografia
btnDecripto.addEventListener('click', () => executar (async () => {                                   
    const texto = saidaTexto.value;
    if (!texto) return mostrarMensagem('O campo texto está vazio');
 
    const dados = await chamarApi('/descriptografar', { texto: texto });
    saidaTexto.value = dados.resultado;
    texto_descripto = texto
}));

// ====================================================================================== Exibir Chaves publicas
btnPublica.addEventListener('click', () => executar (async () => {                                    
    const dados = await chamarApi('/chave-publica');
    saidaTexto.value = `e = ${dados.e}\n\nn = ${dados.n}`;
}));

// ====================================================================================== Exibir Chaves privadas
btnPrivada.addEventListener('click', () => executar (async () => {                                   
    const dados = await chamarApi('/chave-privada');
    saidaTexto.value = `d = ${dados.d}\n\nn = ${dados.n}`;
}));

// ====================================================================================== Limpar caixa de texto
btnLimpa.addEventListener('click', () => {                                      
    entradaTexto.value = '';
    saidaTexto.value = '';
    updateCounters();
    entradaTexto.focus();
});

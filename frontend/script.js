const inputText = document.getElementById('inputText');
const outputText = document.getElementById('outputText');
const methodSelect = document.getElementById('methodSelect');
const cesarConfig = document.getElementById('cesarConfig');
const cesarShift = document.getElementById('cesarShift');
const inputCount = document.getElementById('inputCount');
const outputCount = document.getElementById('outputCount');

const btnEncrypt = document.getElementById('btnEncrypt');
const btnDecrypt = document.getElementById('btnDecrypt');
const btnClear = document.getElementById('btnClear');
const btnCopy = document.getElementById('btnCopy');
const copyFeedback = document.getElementById('copyFeedback');

// Alternar visibilidade do controle de deslocamento da Cifra de César
methodSelect.addEventListener('change', () => {
    if (methodSelect.value === 'cesar') {
    cesarConfig.classList.remove('hidden');
    } else {
    cesarConfig.classList.add('hidden');
    }
});

// Atualização de contadores em tempo real
function updateCounters() {
    inputCount.textContent = `${inputText.value.length} caracteres`;
    outputCount.textContent = `${outputText.value.length} caracteres`;
}

inputText.addEventListener('input', updateCounters);

// Algoritmos com suporte total a UTF-8 e acentuação
const Algorithms = {
    base64: {
    encrypt: (str) => {
        const bytes = new TextEncoder().encode(str);
        const binString = Array.from(bytes, (b) => String.fromCharCode(b)).join("");
        return btoa(binString);
    },
    decrypt: (str) => {
        const binString = atob(str.trim());
        const bytes = Uint8Array.from(binString, (m) => m.charCodeAt(0));
        return new TextDecoder().decode(bytes);
    }
    },
    cesar: {
    encrypt: (str, shift) => {
        shift = Number(shift) % 26;
        return str.replace(/[a-zA-Z]/g, (char) => {
        const base = char <= 'Z' ? 65 : 97;
        return String.fromCharCode(((char.charCodeAt(0) - base + shift) % 26) + base);
        });
    },
    decrypt: (str, shift) => {
        shift = Number(shift) % 26;
        return str.replace(/[a-zA-Z]/g, (char) => {
        const base = char <= 'Z' ? 65 : 97;
        return String.fromCharCode(((char.charCodeAt(0) - base - shift + 26) % 26) + base);
        });
    }
    },
    hex: {
    encrypt: (str) => {
        return Array.from(new TextEncoder().encode(str))
        .map(b => b.toString(16).padStart(2, '0'))
        .join(' ');
    },
    decrypt: (str) => {
        const cleaned = str.replace(/\s+/g, '');
        if (cleaned.length % 2 !== 0) throw new Error("Código Hexadecimal incompleto");
        const bytes = [];
        for (let i = 0; i < cleaned.length; i += 2) {
        bytes.push(parseInt(cleaned.substr(i, 2), 16));
        }
        return new TextDecoder().decode(new Uint8Array(bytes));
    }
    },
    reverse: {
    encrypt: (str) => Array.from(str).reverse().join(''),
    decrypt: (str) => Array.from(str).reverse().join('')
    }
};

// Processamento
btnEncrypt.addEventListener('click', () => {
    const text = inputText.value;
    if (!text) return;
    const method = methodSelect.value;
    const shift = cesarShift.value;

    try {
    outputText.value = Algorithms[method].encrypt(text, shift);
    } catch (err) {
    outputText.value = `[Erro na criptografia]: ${err.message}`;
    }
    updateCounters();
});

btnDecrypt.addEventListener('click', () => {
    const text = inputText.value;
    if (!text) return;
    const method = methodSelect.value;
    const shift = cesarShift.value;

    try {
    outputText.value = Algorithms[method].decrypt(text, shift);
    } catch (err) {
    outputText.value = `[Erro]: Formato inválido ou corrompido para o método selecionado.`;
    }
    updateCounters();
});

btnClear.addEventListener('click', () => {
    inputText.value = '';
    outputText.value = '';
    updateCounters();
    inputText.focus();
});

// Cópia para a área de transferência com fallback
btnCopy.addEventListener('click', () => {
    if (!outputText.value) return;

    outputText.select();
    outputText.setSelectionRange(0, 99999);

    let success = false;
    try {
    success = document.execCommand('copy');
    } catch (e) {
    success = false;
    }

    if (!success && navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(outputText.value).then(showCopiedState);
    } else if (success) {
    showCopiedState();
    }
});

function showCopiedState() {
    btnCopy.classList.add('copied');
    copyFeedback.textContent = 'Copiado!';
    setTimeout(() => {
    btnCopy.classList.remove('copied');
    copyFeedback.textContent = 'Copiar';
    }, 1800);
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
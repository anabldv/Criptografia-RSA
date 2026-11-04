# Javascript  
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
  
// Alterna exibição do controle de deslocamento da Cifra de César  
methodSelect.addEventListener('change', () => {  
  if (methodSelect.value === 'cesar') {  
    cesarConfig.classList.remove('hidden');  
  } else {  
    cesarConfig.classList.add('hidden');  
  }  
});  
  
// Atualiza contadores  
function updateCounters() {  
  inputCount.textContent = `${inputText.value.length} caracteres`;  
  outputCount.textContent = `${outputText.value.length} caracteres`;  
}  
  
inputText.addEventListener('input', updateCounters);  
  
// Métodos de Criptografia e Descriptografia  
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
      if (cleaned.length % 2 !== 0) throw new Error("Hexadecimal incompleto");  
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
  
btnEncrypt.addEventListener('click', () => {  
  const text = inputText.value;  
  if (!text) return;  
  const method = methodSelect.value;  
  const shift = cesarShift.value;  
  
  try {  
    outputText.value = Algorithms[method].encrypt(text, shift);  
    updateCounters();  
  } catch (err) {  
    outputText.value = `[Erro na criptografia]: ${err.message}`;  
  }  
});  
  
btnDecrypt.addEventListener('click', () => {  
  const text = inputText.value;  
  if (!text) return;  
  const method = methodSelect.value;  
  const shift = cesarShift.value;  
  
  try {  
    outputText.value = Algorithms[method].decrypt(text, shift);  
    updateCounters();  
  } catch (err) {  
    outputText.value = `[Erro na descriptografia]: Formato inválido ou corrompido.`;  
  }  
});  
  
btnClear.addEventListener('click', () => {  
  inputText.value = '';  
  outputText.value = '';  
  updateCounters();  
});  
  
btnCopy.addEventListener('click', async () => {  
  if (!outputText.value) return;  
  try {  
    await navigator.clipboard.writeText(outputText.value);  
    copyFeedback.textContent = 'Copiado!';  
    setTimeout(() => {  
      copyFeedback.textContent = 'Copiar';  
    }, 2000);  
  } catch (err) {  
    copyFeedback.textContent = 'Falhou';  
  }  
});  

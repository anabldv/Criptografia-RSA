
## Roteiro de Apresentação: Como Funciona o Algoritmo RSA

### Parte 1: Visão Geral e Ponto de Entrada

**Fala Sugerida:** "O algoritmo RSA é um sistema de criptografia assimétrica fundamentado na criação de um par de chaves: uma chave pública usada para trancar a mensagem e uma chave privada exclusiva para destrancá-la."

* **Referência no Código (`FunRSA.c`):** O fluxo principal da implementação se concentra na função `rsa()` **(Linha 984)**. Ela atua como um maestro, recebendo a requisição inicial e direcionando o programa para gerar as chaves, criptografar ou descriptografar os dados.



### Parte 2: A Base de Tudo – Geração de Números Primos ($p$ e $q$)

**Fala Sugerida:** "A segurança matemática do RSA repousa na extrema dificuldade de fatorar números gigantescos. O primeiro passo prático é gerar dois grandes números primos aleatórios, chamados convencionalmente de $p$ e $q$."

* **Referência no Código:**
* A orquestração do sorteio ocorre na função `gera_p_q()` **(Linha 548)**.


* Para criar cada número, o código chama `gera_primo()` **(Linha 535)**, que sorteia bits aleatórios e descarta rapidamente valores divisíveis por números pequenos para ganhar performance.


* A certeza de que o número é genuinamente primo é dada pela função `miller_rabin()` **(Linha 483)**, que executa um complexo teste de primalidade.


* Por fim, a função `verifica_p_q()` **(Linha 564)** faz a validação de segurança final, atestando que os primos gerados são ímpares, maiores que 1 e diferentes entre si.





### Parte 3: Construção do Módulo e Função Totiente

**Fala Sugerida:** "Com os dois números primos em mãos, nós os multiplicamos para criar o módulo $n$. Esse módulo fará parte de ambas as chaves. Simultaneamente, calculamos a Função Totiente de Euler, que chamamos de $\phi(n)$, o segredo matemático necessário para construir a chave privada."

* **Referência no Código:**
* Esta matemática de base acontece na função `calcula_bases()` **(Linha 577)**.


* Nela, são executadas as operações de multiplicação cruciais para encontrar o módulo (`n = p * q`) e o totiente (`phi = (p - 1) * (q - 1)`).





### Parte 4: O Nascimento das Chaves (Pública e Privada)

**Fala Sugerida:** "Nesta etapa, o par de chaves nasce. A chave pública recebe um expoente $e$, enquanto a chave privada recebe um expoente $d$, que funciona como o inverso matemático de $e$. Apenas o expoente $d$ consegue desfazer a matemática da chave pública."

* **Referência no Código:**
* **Chave Pública ($e$):** Calculada pela função `chave_publica()` **(Linha 596)**. O código tenta primeiramente usar o valor padrão de 65537. Caso esse número não seja compatível (co-primo) com o Totiente, a função sorteia novos valores até encontrar um expoente válido.


* **Chave Privada ($d$):** Gerada na função `chave_privada()` **(Linha 626)**. O código aplica o Algoritmo de Euclides Estendido para calcular o inverso modular de $e$.


* **Fluxo Completo:** Todo esse processo linear de preparação (geração de primos, bases e chaves) é unificado pela função `inicia_chaves()` **(Linha 874)**.





### Parte 5: Criptografando a Mensagem

**Fala Sugerida:** "Para enviar um segredo, nós convertemos as letras do texto em blocos numéricos. Depois, elevamos cada bloco ao expoente público $e$ e calculamos o resto da divisão pelo módulo $n$. A mágica acontece pela fórmula $c = m^e \pmod n$."

* **Referência no Código:**
* O preparo da mensagem, a divisão do texto original em blocos menores e a adição de bytes de preenchimento ocorrem na função `criptografar()` **(Linha 767)**.


* O coração matemático do RSA — a exponenciação em si — é executado de forma otimizada pela função `exp_modular()` **(Linha 462)**. Esta função aplica a técnica de Montgomery para resolver a potência gigantesca de forma eficiente, sem causar lentidão.





### Parte 6: Descriptografando e Recuperando a Informação

**Fala Sugerida:** "Quando o destinatário recebe o texto embaralhado, ele aplica sua chave privada exclusiva. A fórmula reversa é perfeitamente simétrica: eleva-se a cifra ao expoente $d$ e divide-se por $n$. O resultado numérico, $m = c^d \pmod n$, é convertido novamente no texto original."

* **Referência no Código:**
* A tradução dos blocos cifrados e as validações de segurança são coordenadas pela função `descriptografar()` **(Linha 812)**.


* A pesada operação de destrancar a matemática da cifra chama novamente a função central `exp_modular()` **(Linha 462)**. Com a operação concluída, o código remove o preenchimento extra e devolve a mensagem legível ao usuário.
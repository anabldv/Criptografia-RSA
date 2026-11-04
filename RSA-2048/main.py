import rsa as R

R.gerar_chaves(2048)                 # uma vez
c = R.criptografar("Olá, mundo!")    # devolve hex
print(c)
m = R.descriptografar(c)             # devolve o texto
print(m)

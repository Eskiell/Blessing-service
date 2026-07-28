# Plano de desenvolvimento simples

## Agora

1. Compilar o único `ezhelit-store.elf`.
2. Executá-lo pelo loader/Payload Manager.
3. Confirmar que ele instala ou preserva o tile em Media.
4. Confirmar a notificação de serviço ativo na porta `5911`.
5. Abrir o tile e confirmar que a tela do servidor é exibida.

## Depois da validação do launcher

1. Construir a tela do catálogo no servidor local.
2. Listar jogos fictícios.
3. Integrar o servidor ao HD.
4. Mostrar os backups reais.
5. Planejar a transferência para o PS5 separadamente.

## Fora do escopo atual

- servidor embutido no PS5;
- `eboot.elf` dentro do app;
- manipulação de payloads ELF;
- leitura do HD pelo launcher;
- transferência, instalação ou montagem de jogos.

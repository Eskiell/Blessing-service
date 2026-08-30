# PR 18 — Rebranding para Blessing

## Objetivo

Renomear a identidade pública do projeto de **EZ Cheats** para **Blessing** sem
quebrar instalações, arquivos de cheat nem clientes existentes.

## Escopo

- [x] Atualizar o nome exibido no frontend e no título HTML.
- [x] Atualizar o nome do tile persistente na aba Mídia.
- [x] Atualizar notificações, logs e metadados públicos da API.
- [x] Renomear o artefato de build para `blessing.elf`.
- [x] Atualizar o nome do pacote privado do frontend.
- [x] Atualizar README, arquitetura, operação e checklists.
- [ ] Confirmar no PS5 a atualização do nome do tile e a abertura do frontend.

## Compatibilidade preservada

Os identificadores abaixo permanecem deliberadamente iguais:

- Title ID `EZCH00001`, para atualizar o tile existente em vez de duplicá-lo;
- `/data/ez-cheats/cheats`, para preservar os cheats já instalados;
- namespace C++ `ezcheats` e macros `EZ_CHEATS_*`, evitando uma refatoração sem
  benefício para o usuário;
- header HTTP `X-EZ-Cheats-Request`, para não quebrar clientes existentes;
- repositório e diretório local `ez-cheats`, que podem ser renomeados em uma
  operação separada se desejado.

## Critérios de aceite

- O build produz somente `blessing.elf` como novo artefato.
- O tile existente passa a exibir **Blessing**, sem criar uma segunda entrada.
- A interface, as notificações e a API identificam o produto como Blessing.
- Cheats e integrações existentes continuam funcionando.
- Testes de host e build completo passam.

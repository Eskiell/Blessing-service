# PR 19 — Decodificação de entidades XML em SHN

## Objetivo

Importar do OnionHEN a correção de texto relevante para o Blessing, mantendo a
seleção atual de um único arquivo de cheats por jogo e versão.

## Escopo

- [x] Decodificar `&amp;`, `&lt;`, `&gt;`, `&quot;` e `&apos;` nos valores
  extraídos do SHN.
- [x] Aplicar a nomes de jogo, autores, nomes de cheats e descrições.
- [x] Preservar entidades desconhecidas, como `&nbsp;`.
- [x] Cobrir a mudança com teste automatizado.
- [x] Manter intactos o repositório, a API, o frontend e a aplicação de patches.
- [x] Não adicionar carregamento de múltiplas fontes.
- [ ] Validar no PS5 com um arquivo SHN que contenha entidades XML.

## Critério de conclusão

O parser deve apresentar os textos decodificados sem alterar offsets, bytes,
módulos ou a estratégia de seleção do arquivo de cheats.

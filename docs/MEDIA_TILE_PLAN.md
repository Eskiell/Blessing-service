# PR 16 — Tile offline na aba Mídia

## Objetivo

Instalar ou atualizar, ao executar `blessing.elf`, um tile persistente chamado
**Blessing** na aba Mídia. O tile abre o frontend incorporado por
`http://127.0.0.1:5911/`, sem internet e sem outro dispositivo na rede.

## Escopo

- [x] Incorporar um manifesto com Title ID válido `EZCH00001` e deep link local.
- [x] Incorporar um ícone original de anjo em PNG 512×512.
- [x] Comparar os assets instalados e evitar reinstalações desnecessárias.
- [x] Registrar o título por `SceAppInstUtil`, com fallback compatível.
- [x] Tratar falhas do instalador como não fatais para o motor de cheats.
- [x] Fixar a porta 5911 para manter o deep link consistente.
- [x] Atualizar arquitetura, operação, checklist e atribuições.
- [ ] Validar instalação, abertura offline, atualização e persistência no PS5.

> O primeiro teste do PR 16 gravou os assets, mas não exibiu o tile. A repetição
> segura do registro e os diagnósticos necessários são tratados pelo PR 17 em
> `MEDIA_TILE_REGISTRATION_FIX_PLAN.md`.

## Critérios de aceite

- O tile aparece uma única vez na aba Mídia com nome e ícone corretos.
- O tile abre o frontend quando o ELF está executando.
- O fluxo funciona com o acesso à internet desativado.
- Reenviar o mesmo ELF não cria duplicatas nem regrava assets idênticos.
- Uma nova build com assets diferentes atualiza o tile existente.
- Depois de reiniciar o console, o tile permanece e volta a funcionar após
  executar novamente o ELF.
- Falha no instalador não impede a API nem a aplicação de cheats.

## Fora do escopo

- Iniciar o ELF automaticamente após reiniciar o PS5.
- Instalar PKG ou modificar a ShellUI.
- Importar funcionalidades de torrent ou depender do EZHELIT-TORRENT.
- Remover o tile pela interface web.

## Referência

O fluxo foi adaptado da implementação C++ já validada localmente em
`/Users/ezequiel/Downloads/EZHELIT-TORRENT`, preservando a atribuição GPLv3 em
`THIRD_PARTY_NOTICES.md`.

# Blessing

Interface Vue 3 para listar e alternar cheats do jogo em execução. O build usa
Vite e gera um único `dist/index.html`, adequado para ser incorporado ao futuro
payload/daemon do Blessing.

O trabalho de migração do motor do OnionHEN está organizado em PRs no
[`docs/ONIONHEN_MIGRATION_PLAN.md`](docs/ONIONHEN_MIGRATION_PLAN.md).
Para entender a arquitetura e acompanhar o fluxo completo de um toggle, leia
[`docs/CODE_ARCHITECTURE.md`](docs/CODE_ARCHITECTURE.md).
Código adaptado e fixtures são identificados em
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## Desenvolvimento

```sh
cd frontend
npm install
npm run dev
```

## Build

```sh
git clone https://github.com/Eskiell/ez-cheats.git
cd ez-cheats
export PS5_PAYLOAD_SDK=/caminho/do/ps5-payload-sdk
make all
```

O build gera `blessing.elf` em C++20 e incorpora o HTML produzido pelo Vite.
O SDK atualmente instalado não inclui libc++; por isso, a fundação usa classes
C++ e RAII sobre a libc do SDK, sem depender da biblioteca-padrão C++. Essa
fronteira pode ser revista caso libc++ seja adicionada ao toolchain.

Para validar o roteamento HTTP no host:

```sh
make host-test
```

Para enviar o ELF ao console:

```sh
make deploy PS5_HOST=ip-do-ps5
```

Configurações são fornecidas no build:

```sh
make all HTTP_PORT=5911 \
  CHEATS_DIRECTORY=/data/ez-cheats/cheats \
  MEMORY_BACKEND=automatic
```

`MEMORY_BACKEND` aceita `automatic`, `mdbg` ou `kdirect`. O modo automático
consulta o firmware no console. Consulte o [guia operacional](docs/OPERATIONS.md)
para instalação, segurança e diagnóstico e o
[checklist PS5](docs/PS5_TEST_CHECKLIST.md) antes de publicar uma versão.

## Arquivos de cheats

O runtime procura cheats no diretório plano `/data/ez-cheats/cheats`. O nome
de cada arquivo deve seguir `<TITLE_ID>_<VERSION>.<formato>`, por exemplo
`PPSA00001_01.002.003.json`.

Quando mais de um arquivo existir para o mesmo jogo e versão, a precedência é:

1. `.json`
2. `.shn`
3. `.mc4`
4. `.ShnExt`

Title ID e versão são validados antes de compor o caminho. O repositório rejeita
links simbólicos e acompanha caminho, inode, tamanho, `mtime` e `ctime` para
recarregar automaticamente um arquivo substituído ou editado. Árvores aninhadas
de distribuição não são processadas pelo payload; eventual flatten deve ser
feito por uma ferramenta de instalação separada.

## Plataforma de jogo

O adaptador PS5 consulta o BigApp em execução, relaciona seu App ID aos
processos do sistema e preenche PID, Title ID, nome, versão, plataforma e nome
do processo. Metadados são lidos dos `param.json` do PS5 ou `param.sfo` do PS4.
Também é possível localizar módulos carregados por nome e obter suas seções de
memória. Quando nenhum jogo está aberto, a consulta retorna normalmente sem
contexto (`pid` e `appId` iguais a `-1`).

Headers e chamadas específicas do SDK ficam em
`src/platform/ps5_game_platform.cpp`; domínio, serviço e testes host não
incluem dependências do SDK.

## Backends de memória

O motor oferece os backends `mdbg` e `kdirect`. No modo automático, firmwares
anteriores a 8.40 usam mdbg e firmwares 8.40 ou superiores usam kdirect; a
seleção também pode ser forçada pelo chamador. Toda escrita pode ser confirmada
por leitura imediata com `write_verified`.

O mapeamento de code cave alinha a faixa em páginas de 16 KiB, anexa o processo
com RAII, executa `mmap` com endereço fixo, aplica proteção RWX e sempre restaura
os registradores e desanexa o processo, inclusive em retornos de erro. As APIs
de kernel, mdbg e ptrace permanecem restritas a
`src/memory/ps5_memory_backends.cpp`.

## Aplicação de cheats

`CheatApplier::set_enabled` resolve o módulo no processo principal ou em outro
processo do mesmo App ID, calcula endereços absolutos/relativos e confirma cada
escrita por readback. Patches de PS2 usam endereços absolutos. Master codes são
identificados e usados para ajustar patches dependentes quando necessário.

Antes de cada mutação, os bytes atuais são salvos. Se qualquer patch falhar, os
patches já processados são restaurados em ordem reversa; o estado `enabled` só
muda quando a operação inteira termina. Mudança de PID invalida todos os estados
anteriores. Code cave é tentado somente ao habilitar um endereço que não podia
ser lido e que possui bytes de restauração do mesmo tamanho.

Nesta primeira fundação estão disponíveis:

- `GET /`
- `GET /health`
- `GET /api/v1/version`

`GET /api/v1/cheats` e `PUT /api/v1/cheats/:id` usam o serviço real. O payload
detecta o jogo pelo adaptador PS5, carrega `/data/ez-cheats/cheats`, seleciona o
backend de memória e confirma as escritas antes de publicar o estado ativo.

## Contrato esperado do backend

### `GET /api/v1/cheats`

```json
{
  "connected": true,
  "backend": "mdbg",
  "error": null,
  "game": {
    "titleId": "CUSA00001",
    "name": "Jogo em execução",
    "version": "1.00",
    "platform": "ps4"
  },
  "cheats": [
    {
      "id": 0,
      "name": "Vida infinita",
      "description": "Mantém a vida no valor máximo",
      "author": "Autor",
      "enabled": false
    }
  ]
}
```

### `PUT /api/v1/cheats/:id`

Corpo:

```json
{ "enabled": true }
```

A resposta deve devolver ao menos `id` e `enabled` com o estado efetivamente
aplicado pelo motor de cheats.

Erros usam o formato `{"error":"codigo","message":"detalhes"}`. Toggle sem
jogo responde `409`, ID ausente responde `404`, falha de módulo ou memória
responde `422` e indisponibilidade interna do serviço responde `503`. Todas as
respostas de estado usam `Cache-Control: no-store`.

Mutações aceitam apenas a origem do frontend incorporado e exigem o cabeçalho
`X-EZ-Cheats-Request: 1`. Essa proteção impede que uma página aberta em outra
origem altere a memória do jogo através do navegador do usuário.

## Licenças e componentes

O projeto é distribuído sob GPL-3.0. Consulte o
[inventário de componentes](docs/COMPONENT_INVENTORY.md) e os
[avisos detalhados](THIRD_PARTY_NOTICES.md) antes de redistribuir o ELF ou
alterar dependências vendorizadas.

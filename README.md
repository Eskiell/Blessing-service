# EZ Cheats Frontend

Interface Vue 3 para listar e alternar cheats do jogo em execução. O build usa
Vite e gera um único `dist/index.html`, adequado para ser incorporado ao futuro
payload/daemon do EZ Cheats.

O trabalho de migração do motor do OnionHEN está organizado em PRs no
[`docs/ONIONHEN_MIGRATION_PLAN.md`](docs/ONIONHEN_MIGRATION_PLAN.md).

## Desenvolvimento

```sh
cd frontend
npm install
npm run dev
```

## Build

```sh
export PS5_PAYLOAD_SDK=/caminho/do/ps5-payload-sdk
make
```

O build gera `ez-cheats.elf` em C++20 e incorpora o HTML produzido pelo Vite.
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

Nesta primeira fundação estão disponíveis:

- `GET /`
- `GET /health`
- `GET /api/v1/version`

Durante a evolução do domínio, `GET /api/v1/cheats` e
`PUT /api/v1/cheats/:id` usam um serviço em memória. Ele existe para validar o
contrato e o frontend antes da integração com parsers e memória do PS5; nenhum
patch real é aplicado nesta etapa.

## Contrato esperado do backend

### `GET /api/v1/cheats`

```json
{
  "connected": true,
  "backend": "mdbg",
  "game": {
    "titleId": "CUSA00001",
    "name": "Jogo em execução",
    "version": "1.00",
    "platform": "ps4"
  },
  "cheats": [
    {
      "id": "0",
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

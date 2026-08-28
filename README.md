# EZ Cheats Frontend

Interface Vue 3 para listar e alternar cheats do jogo em execução. O build usa
Vite e gera um único `dist/index.html`, adequado para ser incorporado ao futuro
payload/daemon do EZ Cheats.

## Desenvolvimento

```sh
cd frontend
npm install
npm run dev
```

## Build

```sh
cd frontend
npm run build
```

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

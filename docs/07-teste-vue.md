# Teste do frontend Vue

## Objetivo

Validar Vue 3 dentro da interface servida pelo payload no PS5 sem alterar a
API, o worker de download ou o tile.

## Implementação

- Vue 3 para a interface.
- Vite para o build.
- `vite-plugin-singlefile` para incorporar JavaScript e CSS no
  `frontend/dist/index.html`.
- Target de build: ES2015 e Safari 12.
- O HTML final é incorporado em `ezhelit-store.elf`.

## Procedimento no PS5

1. Encerrar ou reiniciar qualquer instância anterior usando a porta `5911`.
2. Enviar o novo `ezhelit-store.elf`.
3. Aguardar a notificação de serviço pronto.
4. Abrir o tile EZHELIT Store.
5. Confirmar a marca `VUE TEST`.
6. Confirmar que título, botão e estado aparecem.
7. Iniciar o download de teste.
8. Confirmar que status, percentual e barra de progresso são atualizados.
9. Fechar e reabrir a tela.
10. Confirmar que o estado do worker reaparece.

## Critérios para manter Vue

- tela renderiza sem ficar branca;
- botão responde;
- consultas `fetch` funcionam;
- progresso é atualizado;
- interface continua funcional após fechar e reabrir;
- desempenho de navegação é aceitável.

## Rollback

A interface HTML anterior foi preservada em:

```text
frontend/legacy/index.html
```

Para gerar um payload de fallback:

```sh
make legacy-ui
```

O resultado será:

```text
ezhelit-store-legacy-ui.elf
```

Esse ELF utiliza o mesmo backend, instalador e worker; somente a interface não
usa Vue. O build padrão continua gerando apenas `ezhelit-store.elf`.

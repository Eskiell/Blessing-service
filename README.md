# EZHELIT Store

O projeto gera um único payload:

```text
ezhelit-store.elf
```

Ao ser enviado para o PS5, ele:

- instala ou atualiza o tile `EZHELIT Store` na área Media;
- embute `param.json` e `icon0.png`;
- inicia o servidor HTTP na porta `5911`;
- serve a interface Vue 3 embutida;
- mantém o worker de download em execução quando a tela é fechada.

O tile instalado não contém `eboot.elf`; ele abre o servidor mantido pelo
payload em `http://127.0.0.1:5911/`.

## Build

```sh
export PS5_PAYLOAD_SDK=/caminho/para/ps5-payload-sdk
make
```

O build executa o Vite, gera um único HTML e o incorpora ao ELF. Para gerar a
interface HTML anterior como fallback, use `make legacy-ui`.

Consulte [docs](./docs/README.md) para o planejamento.

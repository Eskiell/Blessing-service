# EZHELIT Store

O projeto gera um único payload:

```text
ezhelit-store.elf
```

Ao ser enviado para o PS5, ele:

- instala ou atualiza o tile `EZHELIT Store` na área Media;
- embute `param.json` e `icon0.png`;
- inicia o servidor HTTP na porta `5911`;
- serve a interface da Store;
- mantém o worker de download em execução quando a tela é fechada.

O tile instalado não contém `eboot.elf`; ele abre o servidor mantido pelo
payload em `http://192.168.15.122:5911/`.

## Build

```sh
export PS5_PAYLOAD_SDK=/caminho/para/ps5-payload-sdk
make
```

Consulte [docs](./docs/README.md) para o planejamento.

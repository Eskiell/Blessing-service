# EZHELIT Store

Primeira fase: um launcher na área Media do PS5 que abre exclusivamente:

```text
http://192.168.15.122:5911/
```

O servidor deve estar ativo na rede local antes de o tile ser aberto.

Nesta fase, o launcher:

- não sobe servidor HTTP;
- não contém um `eboot.elf`;
- não copia ou manipula payloads ELF;
- não lista, transfere, instala ou monta jogos.

Consulte [docs](./docs/README.md) para o planejamento e
[installer/README.md](./installer/README.md) para build e instalação.

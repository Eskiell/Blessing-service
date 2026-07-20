# EZHELIT Store

O projeto contém um launcher na área Media do PS5 que abre:

```text
http://192.168.15.122:5911/
```

O servidor deve estar ativo na rede local antes de o tile ser aberto.

O launcher:

- não sobe servidor HTTP;
- não contém um `eboot.elf`;
- não copia ou manipula payloads ELF;
- não lista, transfere, instala ou monta jogos.

O payload experimental `ezhelit-store.elf` serve uma tela mínima nessa porta e
testa um download persistente enquanto o payload estiver em execução. Ele é
separado do launcher e não é instalado dentro do tile.

Consulte [docs](./docs/README.md) para o planejamento e
[installer/README.md](./installer/README.md) para build e instalação.

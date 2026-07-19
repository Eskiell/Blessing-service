# Instalador do launcher EZHELIT Store

Este instalador registra um tile na área Media do PS5. O tile contém somente:

- `sce_sys/param.json`, com o deeplink `http://192.168.15.122:5911/`;
- `sce_sys/icon0.png`.

O launcher instalado não contém, inicia, copia ou atualiza um `eboot.elf`. Ao
ser selecionado, o sistema abre o deeplink e espera que o servidor já esteja
disponível na rede local.

## Build

```sh
export PS5_PAYLOAD_SDK=/caminho/para/ps5-payload-sdk
make
```

O resultado é `installer/ezhelit-store-installer.elf`.

## Instalação

Execute o instalador uma vez pelo Payload Manager. O ELF é apenas a ferramenta
de registro do tile; ele não faz parte do app instalado.

Antes de abrir o tile, confirme que `http://192.168.15.122:5911/` responde na
mesma rede do PS5.

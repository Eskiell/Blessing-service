# Guia operacional

## Requisitos e build

- PS5 Payload SDK com os headers e bibliotecas de kernel, mdbg, system service
  e pthread.
- Node.js e npm compatíveis com o lockfile do frontend.
- Toolchain de host com suporte a C++20 para executar os testes.

Em um clone limpo:

```sh
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
npm ci --prefix frontend
make host-test
make all
```

O build produz `ez-cheats.elf` e incorpora `frontend/dist/index.html`. As opções
operacionais são definidas no build:

| Opção | Padrão | Valores |
| --- | --- | --- |
| `HTTP_PORT` | `5911` | porta TCP de 1 a 65535 |
| `CHEATS_DIRECTORY` | `/data/ez-cheats/cheats` | caminho absoluto no PS5 |
| `MEMORY_BACKEND` | `automatic` | `automatic`, `mdbg`, `kdirect` |
| `SHNEXT_KEYSTONE` | `0` | `0` ou `1` |

Exemplo:

```sh
make clean
make all HTTP_PORT=5912 MEMORY_BACKEND=mdbg
make deploy PS5_HOST=192.168.1.50
```

## Instalação e uso

1. Copie os cheats para o diretório configurado usando o nome
   `<TITLE_ID>_<VERSION>.<formato>`.
2. Envie e execute o ELF pelo carregador de payload usado no console.
3. Abra `http://<IP-DO-PS5>:<HTTP_PORT>/` em um dispositivo da mesma rede.
4. Inicie o jogo e confira Title ID e versão antes de ativar qualquer cheat.

O payload escuta em todas as interfaces. Use-o somente em uma rede local
confiável e não publique a porta na internet. Requisições `PUT` exigem `Origin`
igual ao `Host` do payload e `X-EZ-Cheats-Request: 1`. Uma ferramenta manual
precisa enviar ambos explicitamente; o frontend incorporado já faz isso.

## Limites e comportamento

- Requisições HTTP têm limite total de 8 KiB e respostas de estado, 48 KiB.
- O corpo de toggle aceita somente `{"enabled":true}` ou
  `{"enabled":false}`, com espaços opcionais e sem campos adicionais.
- Arquivos de cheat têm limite de 16 MiB e não podem ser links simbólicos.
- Respostas não são armazenadas em cache.
- Hot reload e encerramento seguem as garantias descritas em
  [CHEAT_SERVICE_LIFECYCLE.md](CHEAT_SERVICE_LIFECYCLE.md).

## Diagnóstico

| Sintoma | Verificação |
| --- | --- |
| ELF encerra ao iniciar | confirme acesso de escrita ao diretório configurado |
| Jogo não aparece | confirme que o BigApp está aberto e que os metadados do título são legíveis |
| Lista vazia | confira Title ID, versão, nome e extensão do arquivo |
| `could not parse cheat file` | valide o formato e teste a fixture equivalente no host |
| `module not found` | confira o campo de processo/módulo do arquivo |
| `memory write failed` | force `MEMORY_BACKEND=mdbg` ou confirme suporte do firmware ao kdirect |
| HTTP 403 no toggle | use o frontend incorporado ou envie os cabeçalhos de origem exigidos |

Os logs mostram inicialização, diretório, backend, firmware, toggles confirmados
e mutações rejeitadas. Bytes de patches e conteúdo dos arquivos não são
registrados.

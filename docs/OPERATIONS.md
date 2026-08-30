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

O build produz `ez-cheats.elf` e incorpora `frontend/dist/index.html`, o
manifesto e o ícone do tile da aba Mídia. As opções operacionais são definidas
no build:

| Opção | Padrão | Valores |
| --- | --- | --- |
| `HTTP_PORT` | `5911` | porta fixa do tile persistente |
| `CHEATS_DIRECTORY` | `/data/ez-cheats/cheats` | caminho absoluto no PS5 |
| `MEMORY_BACKEND` | `automatic` | `automatic`, `mdbg`, `kdirect` |
| `SHNEXT_KEYSTONE` | `0` | `0` ou `1` |

Exemplo:

```sh
make clean
make all MEMORY_BACKEND=mdbg
make deploy PS5_HOST=192.168.1.50
```

`HTTP_PORT` permanece exposta ao compilador por compatibilidade interna, mas o
build rejeita valores diferentes de `5911`: o deep link persistente instalado
na aba Mídia precisa sempre apontar para a mesma porta.

## Instalação e uso

1. Copie os cheats para o diretório configurado usando o nome
   `<TITLE_ID>_<VERSION>.<formato>`.
2. Envie e execute o ELF pelo carregador de payload usado no console.
3. Na primeira execução, aguarde a instalação do tile **EZ Cheats** na aba
   **Mídia**.
4. Abra o tile, que acessa `http://127.0.0.1:5911/` sem depender de internet ou
   de outro dispositivo. Como alternativa, abra
   `http://<IP-DO-PS5>:<HTTP_PORT>/` em um dispositivo da mesma rede.
5. Inicie o jogo e confira Title ID e versão antes de ativar qualquer cheat.

O tile `EZCHT0001` é persistente e seus assets ficam em
`/user/app/EZCHT0001/sce_sys/`. O payload compara esses arquivos com as cópias
incorporadas e os atualiza quando necessário. Depois de reiniciar o PS5, o tile
continua visível, mas o ELF precisa ser executado novamente antes de abri-lo.
Para remover o tile manualmente, apague `/user/app/EZCHT0001` e remova a entrada
do aplicativo usando a ferramenta de gerenciamento instalada no console.

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
| Tile não aparece | confira o log `media tile`, acesso a `/user/app` e disponibilidade do `SceAppInstUtil` |
| Tile abre página indisponível | execute o ELF e confirme que a porta fixa 5911 está livre |
| Jogo não aparece | confirme que o BigApp está aberto e que os metadados do título são legíveis |
| Lista vazia | confira Title ID, versão, nome e extensão do arquivo |
| `could not parse cheat file` | valide o formato e teste a fixture equivalente no host |
| `module not found` | confira o campo de processo/módulo do arquivo |
| `memory write failed` | force `MEMORY_BACKEND=mdbg` ou confirme suporte do firmware ao kdirect |
| HTTP 403 no toggle | use o frontend incorporado ou envie os cabeçalhos de origem exigidos |

Os logs mostram inicialização, diretório, backend, firmware, toggles confirmados
e mutações rejeitadas. Bytes de patches e conteúdo dos arquivos não são
registrados.

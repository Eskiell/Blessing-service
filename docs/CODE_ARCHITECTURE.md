# Como o EZ Cheats funciona

Este guia explica o código atual do EZ Cheats sem exigir conhecimento prévio do
OnionHEN. O projeto é um payload C++20 para PS5 com um frontend Vue incorporado
no próprio ELF.

## Visão geral

```text
Navegador
   │  GET /api/v1/cheats e PUT /api/v1/cheats/:id
   ▼
HttpServer ──► CheatService ──► CheatApplier
                  │                 │
                  │                 ├──► IGamePlatform
                  │                 └──► IMemoryBackend
                  ▼
          FileCheatRepository
                  │
                  ▼
       JSON / SHN / MC4 / ShnExt
```

O frontend não conhece patches, endereços ou o SDK. Ele recebe um snapshot em
JSON e envia apenas a intenção de ativar ou desativar um ID.

## Responsabilidades por diretório

| Caminho | Responsabilidade |
| --- | --- |
| `include/ezcheats/domain` | modelos e interfaces independentes do PS5 |
| `src/domain` | ownership e operações comuns de memória |
| `src/application` | serviço, ciclo de vida e aplicação de patches |
| `src/parsers` | leitura dos quatro formatos de cheats |
| `src/repository` | descoberta em disco, assinatura e hot reload |
| `src/platform` | detecção do jogo, processos e módulos |
| `installer` | manifesto e ícone incorporados do tile da aba Mídia |
| `src/memory` | backends `mdbg`, `kdirect` e fake |
| `src/http` | servidor HTTP e respostas JSON |
| `frontend/src` | interface Vue e cliente da API |
| `tests` | especificação executável no host |

## 1. Composição do payload

O ponto de entrada é `src/main.cpp`. Ele:

1. ignora `SIGPIPE`, evitando que uma desconexão HTTP encerre o payload;
2. cria `Ps5GamePlatform`;
3. cria `FileCheatRepository` e garante o diretório configurado;
4. instala ou atualiza, sem tornar falhas fatais, o tile `EZ Cheats` que abre
   `http://127.0.0.1:5911/` na aba Mídia;
5. detecta o firmware e seleciona o backend de memória;
6. injeta plataforma, repositório e memória em `CheatService`;
7. entrega o serviço ao `HttpServer`;
8. entra no loop de conexões.

`ps5_media_tile.cpp` incorpora `installer/param.json` e `installer/icon0.png`,
compara os bytes com `/user/app/EZCH00001/sce_sys/` e só registra novamente o
título por `SceAppInstUtil` quando os arquivos estiverem ausentes ou diferentes.
O tile é apenas um deep link local: ele permanece após reboot, mas depende do
ELF em execução para que o servidor responda.

`HTTP_PORT`, `CHEATS_DIRECTORY` e `MEMORY_BACKEND` são opções de build que
viram macros de compilação. Assim, o runtime não depende de um arquivo externo
de configuração.

## 2. Modelos de domínio

Os tipos centrais estão em `include/ezcheats/domain/models.hpp`:

- `GameContext`: PID, App ID, Title ID, nome, versão, plataforma e processo;
- `CheatFile`: arquivo carregado, autores, cheats e controle de master code;
- `CheatEntry`: ID, nome, descrição, módulo, estado e patches;
- `Patch`: offset, seção e bytes de ativação/restauração;
- `ModuleInfo`: módulo e suas seções de memória;
- `ServiceSnapshot`: estado público copiado para a API.

`CheatFile` e `CheatEntry` possuem ponteiros porque os parsers criam listas de
tamanho variável. `OwnedCheatFile` é o dono dessas alocações e garante a
limpeza. `ServiceSnapshot`, por outro lado, é uma cópia por valor e remove os
ponteiros de patches.

Isso impede que o servidor HTTP retenha referências para memória que pode ser
substituída durante um hot reload.

## 3. Interfaces

`include/ezcheats/domain/interfaces.hpp` define as fronteiras:

- `ICheatParser`: transforma bytes em `CheatFile`;
- `ICheatRepository`: encontra e carrega cheats para um jogo;
- `IGamePlatform`: detecta jogo e localiza módulos;
- `IMemoryBackend`: lê, escreve e mapeia code caves;
- `ICheatService`: atualiza, consulta e alterna cheats.

As implementações reais usam o SDK do PS5. Os testes usam fakes com os mesmos
contratos. Por isso o núcleo pode ser compilado e testado no macOS/Linux.

## 4. Detecção do jogo e módulos

`Ps5GamePlatform::current_game` consulta o BigApp, obtém App ID e Title ID e
procura o processo correspondente. Depois resolve nome, versão, plataforma e
nome do processo.

Para localizar um módulo, o adaptador tenta primeiro a lista dinâmica. O
`eboot.bin` nem sempre aparece nessa lista, então existe o fallback
`find_eboot_imagebase`, portado do OnionHEN e validado no Terraria:

```text
PID
 └── estrutura proc
      └── shared_object
           └── objeto do eboot
                └── imagebase
```

O fallback é aceito para `eboot`, `eboot.bin` ou o nome real do processo. A
mesma busca é repetida em outros processos pertencentes ao mesmo App ID quando
o arquivo de cheat aponta para um processo auxiliar.

Headers e funções do SDK permanecem em `src/platform/ps5_game_platform.cpp`.
O domínio conhece somente `IGamePlatform`.

## 5. Arquivos e parsers

O `FileCheatRepository` procura:

```text
/data/ez-cheats/cheats/<TITLE_ID>_<VERSION>.<formato>
```

A precedência é `.json`, `.shn`, `.mc4` e `.ShnExt`. O repositório valida
Title ID, versão, caminho, tipo e tamanho do arquivo e rejeita links simbólicos.

Uma assinatura formada por caminho, inode, tamanho, `mtime` e `ctime` permite
detectar edição ou substituição. O parser é escolhido pela extensão e todos os
formatos produzem o mesmo `CheatFile`:

- JSON: formato GoldHEN/OnionHEN;
- SHN: trainer XML;
- MC4: Base64 + AES-CBC, seguido pelo parser SHN;
- ShnExt: estrutura criptografada/compactada, com suporte opcional a Keystone.

O parse acontece em uma estrutura temporária. Um arquivo inválido não publica
estado parcial nem destrói imediatamente o parse anterior.

## 6. CheatService

`CheatService` é a orquestração central. Seus métodos públicos são protegidos
por um mutex pthread:

- `refresh()`: detecta o jogo e verifica arquivos;
- `snapshot()`: copia o estado para a camada HTTP;
- `set_enabled()`: atualiza o contexto e chama o aplicador.

### Troca de jogo

PID, App ID e Title ID formam a identidade. Quando ela muda, o serviço descarta
o arquivo e os estados anteriores e faz um novo carregamento. Ele não escreve
no PID antigo, pois esse PID pode ter terminado ou sido reutilizado.

### Hot reload

Quando o mesmo jogo continua ativo e o arquivo muda:

1. os cheats ativos são revertidos usando os patches antigos;
2. o novo arquivo é parseado;
3. somente após sucesso o novo conteúdo é publicado;
4. flags `enabled` nunca são herdadas pelo novo arquivo.

Se a reversão falhar, o reload é recusado. Se o novo parse falhar, o arquivo
anterior permanece disponível, desativado, e o snapshot expõe o erro.

No encerramento normal, o serviço só tenta reverter se confirmar que o mesmo
processo continua vivo. Um encerramento abrupto não oferece essa garantia.

## 7. CheatApplier

O fluxo de um toggle é:

1. localizar o cheat pelo ID;
2. escolher o módulo declarado ou o processo principal;
3. localizar o módulo no PID atual ou no mesmo App ID;
4. resolver endereço absoluto ou `imagebase + offset`;
5. salvar os bytes atuais de cada patch;
6. escrever bytes `enable` ou `disable`;
7. reler e confirmar cada escrita;
8. em erro, restaurar os snapshots em ordem reversa;
9. marcar `enabled` somente após sucesso integral.

Assim, uma escrita parcial não produz um falso estado ativo. O erro informa o
cheat e, em falha de resolução, o nome exato do módulo procurado.

### Master code e code cave

Master codes são reconhecidos pelo nome. Patches dependentes podem localizar
seus bytes dentro do master e ajustar o offset. Code cave só é tentado ao
ativar uma região ainda não legível e quando há bytes válidos de restauração.

## 8. Backends de memória

`MemoryBackendFactory` oferece:

- `mdbg`: usa as primitivas mdbg de cópia do processo;
- `kdirect`: traduz endereços virtuais e acessa memória pelo kernel;
- `automatic`: abaixo de 8.40 seleciona mdbg; em 8.40 ou superior, kdirect.

`write_verified` escreve e relê o conteúdo. O backend de teste permite induzir
falhas de leitura/escrita e testar rollback sem PS5.

No Terraria testado, a seleção automática escolheu `KDIRECT` e os cheats foram
aplicados corretamente após a correção do imagebase do eboot.

## 9. API HTTP

O servidor atende:

| Método e rota | Função |
| --- | --- |
| `GET /` | frontend incorporado |
| `GET /health` | saúde do payload |
| `GET /api/v1/version` | versão da API |
| `GET /api/v1/cheats` | jogo, backend, erros e cheats |
| `PUT /api/v1/cheats/:id` | alterna um cheat |

Requisições são limitadas a 8 KiB e respostas de estado a 48 KiB. Todas usam
`Cache-Control: no-store`. Erros distinguem ausência de jogo (`409`), ID
inexistente (`404`), falha de aplicação (`422`) e serviço indisponível (`503`).

Mutações exigem:

- `Origin` correspondente ao `Host`;
- `X-EZ-Cheats-Request: 1`;
- `Content-Type: application/json`;
- corpo contendo somente `enabled: true` ou `enabled: false`.

## 10. Frontend Vue

`frontend/src/api/cheats.js` encapsula `fetch` e transforma erros HTTP em
`ApiError`. `CheatsApp.vue` guarda o snapshot, loading, erro e IDs pendentes.

Há polling a cada três segundos. Um polling em andamento termina antes de um
toggle, e novos pollings não começam durante a mutação. A interface só atualiza
o switch com a resposta confirmada do backend.

O Vite gera um único `dist/index.html`. `embedded_frontend.cpp` usa `.incbin`
para inserir esse HTML no ELF; nenhum servidor web externo é necessário.

## 11. Exemplo completo: ativar Walk On Water

```text
Clique no frontend
  └── PUT /api/v1/cheats/<id> { enabled: true }
       └── HttpServer valida origem e JSON
            └── CheatService bloqueia o mutex e atualiza o jogo
                 └── CheatApplier procura eboot.bin
                      ├── lista dinâmica
                      └── fallback do imagebase pelo kernel
                           └── Kdirect escreve os bytes
                                └── readback confirma
                                     └── enabled = true
                                          └── JSON retorna ao frontend
```

Ao desligar, o mesmo fluxo escreve os bytes `disable`. Se qualquer operação
falhar, o frontend recebe erro e o estado não é marcado como ativo.

## 12. Ordem recomendada de leitura

1. `include/ezcheats/domain/models.hpp`;
2. `include/ezcheats/domain/interfaces.hpp`;
3. `tests/cheat_applier.cpp` junto de `src/application/cheat_applier.cpp`;
4. `src/repository/file_cheat_repository.cpp`;
5. `src/application/cheat_service.cpp`;
6. `src/platform/ps5_game_platform.cpp`;
7. `src/memory/ps5_memory_backends.cpp`;
8. `src/http/http_server.cpp`;
9. `frontend/src/CheatsApp.vue`;
10. `src/main.cpp` para ver todas as dependências reunidas.

Execute `make host-test` e acompanhe cada teste na implementação. Para as
partes específicas do console, use [PS5_TEST_CHECKLIST.md](PS5_TEST_CHECKLIST.md).

## Documentos relacionados

- [Ciclo de vida](CHEAT_SERVICE_LIFECYCLE.md)
- [Guia operacional](OPERATIONS.md)
- [Plano de migração](ONIONHEN_MIGRATION_PLAN.md)
- [Inventário de componentes](COMPONENT_INVENTORY.md)
- [Avisos de terceiros](../THIRD_PARTY_NOTICES.md)

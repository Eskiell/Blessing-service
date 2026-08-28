# Plano de migração do motor de cheats do OnionHEN

## Objetivo

Trazer o recurso de cheats do OnionHEN para o EZ Cheats como um payload/serviço
independente, escrito principalmente em C++20, com frontend Vue incorporado e
sem carregar ShellUI, Toolbox, FTP, payload manager ou demais recursos do
OnionHEN.

Este documento é o checklist oficial da migração. Cada demanda abaixo deve ser
entregue em um PR próprio. Um item só deve ser marcado como concluído depois que
o PR correspondente estiver integrado à `main`.

## Princípios

- Usar C++20 para domínio, serviço, HTTP, concorrência e gerenciamento de
  recursos.
- Isolar as APIs C do PS5 Payload SDK atrás de adaptadores C++.
- Manter C somente para dependências ou código existente em que a conversão não
  traga benefício imediato.
- Não importar módulos do OnionHEN que não sejam necessários aos cheats.
- Preservar os formatos `.json`, `.shn`, `.mc4` e `.ShnExt`.
- Manter o frontend desacoplado do backend por uma API HTTP versionada.
- Validar parsers e regras de domínio no host sempre que não dependerem do PS5.
- Preservar os avisos e obrigações das licenças do código importado.

## Arquitetura alvo

```text
frontend Vue (HTML único)
          │
          │ HTTP /api/v1
          ▼
servidor e serviço C++
          │
          ├── detecção do jogo e hot reload
          ├── repositório de arquivos
          ├── parsers de cheats
          └── aplicação e reversão de patches
                         │
                         ▼
              adaptador do SDK C do PS5
                         │
                  mdbg / kernel / pt
```

## Fora do escopo

- ShellUI e injeção nos menus do sistema.
- Toolbox, HomeUI e patches de interface.
- Loja, catálogo, download ou distribuição de cheats.
- FTP, payload manager, Remote Play e App Jailbreak.
- Atualização automática pela internet.
- Reimplementação prematura de bibliotecas criptográficas existentes.

## Convenção de acompanhamento

- `[ ]` planejado ou ainda não integrado.
- `[x]` PR integrado à `main` e critérios de aceite atendidos.
- Alterações no escopo devem ser registradas neste documento no PR afetado.
- Cada PR deve atualizar seu próprio checkbox antes do merge.

## Demandas por PR

### [x] PR 01 — Fundação do payload em C++

**Objetivo:** criar a base compilável do backend sem implementar cheats.

**Escopo:**

- Adicionar build para C++20 usando o PS5 Payload SDK.
- Criar o executável mínimo e uma camada `platform/ps5`.
- Criar wrappers RAII para socket e descritores de arquivo.
- Incorporar `frontend/dist/index.html` ao ELF.
- Servir `/`, `/health` e uma resposta JSON de versão.
- Definir diretórios `src`, `include`, `tests` e `third_party`.

**Critérios de aceite:**

- O payload compila com o SDK configurado.
- O ELF inicia no PS5 e serve o frontend.
- `GET /health` responde sem depender do OnionHEN.

### [x] PR 02 — Contratos de domínio e API

**Objetivo:** definir fronteiras estáveis antes de importar implementações.

**Escopo:**

- Criar os modelos C++ `GameContext`, `CheatFile`, `CheatEntry` e `Patch`.
- Definir interfaces `ICheatParser`, `IMemoryBackend`, `IGamePlatform` e
  `ICheatRepository`.
- Formalizar o contrato de `GET /api/v1/cheats` e
  `PUT /api/v1/cheats/:id`.
- Adicionar backend em memória para desenvolvimento e testes.
- Conectar o frontend atual ao backend em memória.

**Critérios de aceite:**

- O frontend lista e alterna cheats simulados pela API real.
- Modelos não incluem headers do SDK do PS5.
- Testes verificam serialização e validação básica das requisições.

### [x] PR 03 — Núcleo comum e parser JSON

**Objetivo:** importar a primeira parte funcional e portável do OnionHEN.

**Origem principal:**

- `source/util/source/cheats/cheat_engine_utils.c`
- `source/util/source/cheats/json_cheat_parser.cpp`
- `source/util/source/cheats/cheat_parser_factory.cpp`
- `source/util/include/cheats/cheat_engine*.h`

**Escopo:**

- Adaptar tipos e utilitários para os modelos definidos no PR 02.
- Implementar carregamento por buffer e arquivo.
- Implementar o formato GoldHEN/OnionHEN `.json`.
- Remover dependências de logging e caminhos globais do OnionHEN.
- Adicionar fixtures e testes executáveis no host.

**Critérios de aceite:**

- Arquivos JSON válidos geram a lista esperada de cheats e patches.
- Entradas inválidas falham sem vazamento ou estado parcial.
- A camada compila e é testada sem PS5 SDK.

### [x] PR 04 — Parser SHN

**Objetivo:** adicionar suporte aos trainers XML `.shn`.

**Origem principal:**

- `source/util/source/cheats/xml_cheat_parser.cpp`

**Escopo:**

- Portar o parser XML usado pelo OnionHEN.
- Registrar `.shn` na fábrica de parsers.
- Cobrir atributos, módulos, patches, master code e arquivos inválidos.

**Critérios de aceite:**

- Fixtures `.shn` reais e sintéticas passam nos testes do host.
- O parser não depende do backend de memória.

### [x] PR 05 — Parser MC4 e dependências criptográficas

**Objetivo:** adicionar suporte aos arquivos criptografados `.mc4`.

**Origem principal:**

- `source/util/source/cheats/mc4_cheat_parser.cpp`
- `third_party/cheat_support/aes.c`
- `third_party/cheat_support/base64.c`

**Escopo:**

- Importar AES-CBC e Base64 necessários ao formato.
- Descriptografar MC4 e encaminhar o XML resultante ao parser SHN.
- Preservar atribuições e arquivos de licença das dependências.
- Limpar buffers sensíveis depois do uso.

**Critérios de aceite:**

- Fixture MC4 conhecida produz o mesmo resultado do OnionHEN.
- Falhas de Base64, padding ou criptografia são tratadas com segurança.

### [ ] PR 06 — Parser ShnExt

**Objetivo:** completar o suporte aos formatos atuais do OnionHEN.

**Origem principal:**

- `source/util/source/cheats/cheat_engine_parser_shnext.c`
- `source/util/source/cheats/shn_ext_cheat_parser.cpp`
- `third_party/cheat_support/miniz.c`
- `third_party/cheat_support/sha256.c`
- `third_party/cjson`
- Keystone

**Escopo:**

- Importar deflate, SHA-256, cJSON e integração com Keystone.
- Isolar o código C existente atrás de um adaptador C++.
- Implementar montagem x86-64 para patches expressos como Assembly.
- Documentar tamanho e impacto da dependência Keystone.

**Critérios de aceite:**

- Fixtures ShnExt produzem patches equivalentes aos do OnionHEN.
- Buffers descriptografados são apagados antes da liberação.
- Build sem Keystone falha de forma explícita ou desativa somente ShnExt.

### [ ] PR 07 — Repositório de cheats e layout em disco

**Objetivo:** localizar arquivos pelo jogo e recarregá-los quando mudarem.

**Origem principal:**

- `source/util/source/cheats/cheat_repository.cpp`
- `source/util/source/cheats/cheat_flatten.c`
- `source/util/include/cheats/runtime.h`

**Escopo:**

- Definir `/data/ez-cheats/cheats` como diretório padrão.
- Resolver `<TITLE_ID>_<VERSION>.<formato>`.
- Normalizar versões e validar nomes de arquivo.
- Implementar assinatura por caminho, inode, tamanho e timestamps.
- Implementar hot reload sem reiniciar o payload.
- Avaliar o flatten de árvores como ferramenta separada, não como requisito do
  runtime.

**Critérios de aceite:**

- Todos os quatro formatos são descobertos na ordem documentada.
- Troca ou edição do arquivo invalida o cache.
- Caminhos inválidos não escapam do diretório configurado.

### [ ] PR 08 — Adaptador de jogo, processo e módulos do PS5

**Objetivo:** implementar `IGamePlatform` usando o SDK C.

**Origem principal:**

- funções necessárias de `source/util/source/util_platform.c`
- tipos necessários de `source/util/include/util_platform.h`

**Escopo:**

- Detectar o BigApp em execução.
- Obter PID, App ID, Title ID, versão, plataforma e nome do processo.
- Localizar módulos e seções carregadas.
- Encapsular headers C do SDK com `extern "C"` somente na camada PS5.
- Adicionar fakes para testes de serviço no host.

**Critérios de aceite:**

- O frontend identifica corretamente jogo, versão e plataforma no PS5.
- O restante do domínio não inclui headers do PS5 SDK.
- Ausência de jogo é representada como estado normal, não como crash.

### [ ] PR 09 — Backends de acesso à memória

**Objetivo:** permitir leitura e escrita no processo do jogo.

**Origem principal:**

- `source/util/source/cheats/memory_backends.cpp`
- primitivas necessárias de `pt` e kernel

**Escopo:**

- Implementar `MdbgMemoryBackend`.
- Implementar `KdirectMemoryBackend` quando suportado pelo ambiente.
- Selecionar backend por firmware ou configuração explícita.
- Implementar leitura, escrita, verificação e mapeamento de code cave.
- Usar RAII para garantir detach e limpeza em todos os retornos.

**Critérios de aceite:**

- Teste controlado lê e escreve uma região segura no PS5.
- Backend indisponível gera erro claro na API.
- Falhas nunca deixam processo anexado involuntariamente.

### [ ] PR 10 — Aplicador e reversão de cheats

**Objetivo:** portar a lógica que transforma os patches em alterações reais.

**Origem principal:**

- `source/util/source/cheats/cheat_applier.cpp`

**Escopo:**

- Resolver endereços relativos, absolutos e módulos.
- Aplicar bytes `on` e restaurar bytes `off`.
- Verificar a escrita e tentar code cave quando aplicável.
- Portar tratamento de master code e processo alternativo do mesmo App ID.
- Resetar estados quando o PID mudar.

**Critérios de aceite:**

- Ativar e desativar um cheat restaura os bytes esperados.
- Escrita parcial ou divergente não marca o cheat como ativo.
- Índices, tamanhos e endereços inválidos são rejeitados.

### [ ] PR 11 — Serviço de cheats e ciclo de vida do jogo

**Objetivo:** coordenar jogo, arquivo, parser e aplicador com segurança.

**Origem principal:**

- `source/util/source/cheats/cheat_service.cpp`

**Escopo:**

- Implementar estado sincronizado do serviço em C++.
- Carregar e recarregar cheats do jogo atual.
- Reverter cheats ativos na troca ou saída do jogo quando for seguro fazê-lo.
- Definir comportamento para encerramento abrupto do processo.
- Expor operações de consulta e toggle para a camada HTTP.

**Critérios de aceite:**

- Troca de jogo não reaproveita estados ou patches do processo anterior.
- Requisições concorrentes não corrompem o estado.
- Hot reload mantém estados somente quando puder fazê-lo corretamente.

### [ ] PR 12 — API HTTP real e integração final do frontend

**Objetivo:** substituir o backend simulado pelo motor completo.

**Escopo:**

- Implementar `GET /api/v1/cheats` com jogo, backend e lista atual.
- Implementar `PUT /api/v1/cheats/:id` com estado efetivamente aplicado.
- Padronizar códigos HTTP e objetos de erro.
- Impedir cache das respostas de estado.
- Exibir no frontend erros de parser, módulo e escrita em memória.
- Evitar polling concorrente enquanto um toggle estiver em andamento.

**Critérios de aceite:**

- O frontend controla cheats reais sem depender da ShellUI.
- A resposta da API reflete o estado confirmado pelo aplicador.
- A UI continua funcional após troca de jogo e hot reload.

### [ ] PR 13 — Robustez, empacotamento e documentação operacional

**Objetivo:** preparar uma primeira versão utilizável e reproduzível.

**Escopo:**

- Definir configuração de porta, diretório e backend de memória.
- Restringir mutações à origem local esperada ou adicionar token de sessão.
- Revisar limites de requisição, parsing e caminhos.
- Adicionar logs úteis sem expor conteúdo sensível.
- Documentar build, execução, instalação, formatos e diagnóstico.
- Gerar inventário de componentes e licenças importadas.
- Criar checklist de teste no PS5 para os firmwares suportados.

**Critérios de aceite:**

- Build limpo e reproduzível a partir de um clone novo.
- Frontend e payload são entregues juntos.
- Documentação permite instalar e validar sem consultar o OnionHEN.
- Não restam dependências acidentais do OnionHEN fora do código atribuído.

## Ordem e dependências

```text
PR 01 → PR 02 → PR 03 → PR 04 → PR 05 → PR 06
                  │
                  └────→ PR 07 → PR 11 ───→ PR 12 → PR 13

PR 01 → PR 08 → PR 09 → PR 10 ────────────┘
```

Os PRs de parser devem permanecer sequenciais para manter a fábrica e os testes
simples. Depois do PR 02, trabalho de parsers e trabalho de plataforma podem
avançar em paralelo, desde que cada PR continue pequeno e integrável.

## Referência do OnionHEN

Durante a migração, a referência local atual é:

```text
/Users/ezequiel/Local/Cheats/onionHEN
```

Código deve ser adaptado, não copiado indiscriminadamente. Cada PR que importar
código precisa registrar no corpo do PR os arquivos de origem e as mudanças
relevantes realizadas.

## Definição de concluído da migração

A migração estará concluída quando:

- os quatro formatos forem carregados;
- o jogo em execução for identificado;
- cheats puderem ser ativados e revertidos pelo frontend;
- troca de jogo e hot reload forem seguros;
- o payload não depender da execução do OnionHEN ou da ShellUI;
- testes de host e checklist no PS5 estiverem documentados e passando;
- todos os PRs deste plano estiverem marcados como concluídos.

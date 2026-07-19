# Plano de desenvolvimento simples

## Objetivo inicial

Criar um aplicativo homebrew para PS5 que abra por um tile próprio, mostre uma interface dentro do próprio app e liste jogos recebidos de um servidor da rede local.

## Etapa 1 — Preparar e validar o ambiente

- Confirmar que o projeto compila com o PS5 Payload SDK.
- Confirmar que o ELF pode ser iniciado pelo Payload Manager.
- Criar uma tela mínima com texto, por exemplo: `EZHELIT Store`.
- Validar a execução no PS5 antes de adicionar outros recursos.

**Resultado esperado:** o app abre no PS5 e mostra uma tela própria, sem navegador.

## Etapa 2 — Interface e controle

- Escolher uma camada gráfica compatível com o ambiente homebrew.
- Criar o ciclo de renderização do app.
- Ler os comandos do controle.
- Implementar foco, seleção, voltar e sair.
- Criar uma grade com jogos fictícios para testar o visual.

**Resultado esperado:** o usuário navega pelo catálogo fictício usando o controle.

## Etapa 3 — API local de teste

- Criar um servidor HTTP pequeno no equipamento da rede.
- Disponibilizar uma rota como `GET /api/games`.
- Retornar uma lista JSON com jogos fictícios.
- Disponibilizar imagens de capa de teste.
- Manter inicialmente a URL-base `http://192.168.15.122:5911`.

Exemplo de resposta:

```json
{
  "games": [
    {
      "id": "jogo-001",
      "title": "Jogo de teste",
      "coverUrl": "/covers/jogo-001.jpg",
      "sizeBytes": 1000000000,
      "format": "exfat"
    }
  ]
}
```

**Resultado esperado:** a API responde corretamente quando acessada por outro dispositivo da rede.

## Etapa 4 — Integrar app e servidor

- Adicionar um cliente HTTP ao app.
- Consultar `GET /api/games`.
- Interpretar a resposta JSON.
- Baixar e exibir as capas.
- Mostrar carregamento, servidor indisponível e tentativa novamente.

**Resultado esperado:** os jogos enviados pelo servidor aparecem dentro do app no PS5.

## Etapa 5 — Tile e identidade

- Definir nome, identificador e ícone do aplicativo.
- Ajustar os metadados do app.
- Instalar o tile no PS5.
- Confirmar que selecionar o tile inicia diretamente a interface própria.

**Resultado esperado:** o app pode ser iniciado normalmente pela tela do PS5.

## Etapa 6 — Catálogo real do HD

- Definir como o HD será compartilhado com o servidor.
- Confirmar o formato e a organização real dos backups.
- Indexar os jogos do HD.
- Extrair ou cadastrar títulos, tamanhos, formatos e capas.
- Substituir os dados fictícios da API pelo catálogo real.

**Resultado esperado:** o app mostra os backups reais disponíveis no HD.

## Etapa 7 — Transferência, posteriormente

- Escolher um jogo no app.
- Confirmar destino e espaço livre.
- Transferir com progresso e possibilidade de recuperação.
- Validar a integridade do arquivo recebido.
- Manter essa operação separada da futura montagem ou instalação.

**Resultado esperado:** um backup próprio pode ser copiado com segurança do servidor para o PS5.

## Ordem recomendada

Não iniciar a etapa seguinte sem validar a anterior no PS5. A primeira versão útil termina na Etapa 5 usando dados fictícios; a primeira versão integrada ao HD termina na Etapa 6.

## Não fazer agora

- Integração com Nextcloud.
- Transferência de arquivos grandes.
- Instalação ou montagem automática.
- Autenticação complexa.
- Acesso externo à rede local.
- Otimização visual antes de validar renderização, controle e HTTP.

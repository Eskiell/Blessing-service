# Teste prático de download

## Resultado

Status: concluído com sucesso no PS5.

- Origem validada: `http://192.168.15.125:8080/ezhelit.exfat.png`.
- O payload serviu a interface na porta `5911`.
- O download foi iniciado pela tela da Store.
- O arquivo foi baixado e salvo no armazenamento interno.
- O fluxo com arquivo temporário e destino final funcionou.

Esta validação comprova o experimento básico. Hash, retomada após reinício e catálogo dinâmico continuam como etapas posteriores.

## Escopo

Este teste baixa somente:

```text
http://192.168.15.125:8080/ezhelit.exfat.png
```

para:

```text
/data/ezhelit-store/downloads/ezhelit.exfat.png
```

Não instala, monta, abre ou interpreta o arquivo.

## Pré-requisitos

1. O arquivo responde em `192.168.15.125:8080`.
2. A resposta HTTP inclui `Content-Length`.
3. O PS5 consegue alcançar `192.168.15.125:8080` na rede local.
4. O payload pode instalar o tile automaticamente caso ele ainda não exista.
5. Nenhum payload anterior está usando a porta `5911`.

## Procedimento

1. Enviar o único `ezhelit-store.elf` para o loader na porta `9021`.
2. Aguardar a notificação `EZHELIT Store pronta em http://127.0.0.1:5911/`.
3. Abrir o tile EZHELIT Store em Media.
4. Selecionar `Baixar arquivo de teste`.
5. Confirmar que o percentual começa a avançar.
6. Fechar a tela da Store sem reenviar ou encerrar o payload.
7. Aguardar alguns segundos.
8. Abrir o tile novamente.
9. Confirmar que o progresso avançou ou terminou.
10. Conferir por FTP se o arquivo final existe no destino.

Durante a transferência, o arquivo usa a extensão temporária:

```text
/data/ezhelit-store/downloads/ezhelit.exfat.png.part
```

Ele só é renomeado quando a quantidade recebida corresponde ao `Content-Length` informado pelo servidor.

## Resultados possíveis

- `Conectando...`: tentando alcançar `192.168.15.125:8080`.
- `Baixando...`: resposta aceita e arquivo sendo gravado.
- `Download concluido`: tamanho recebido corresponde ao esperado.
- `Download falhou`: a tela mostra o motivo básico.

## Limites deste protótipo

- URL fixa no código;
- apenas HTTP, sem HTTPS;
- sem redirects;
- exige `Content-Length`;
- um download por vez;
- sem SHA-256;
- sem retomada após reiniciar o payload ou console;
- estado somente em memória;
- sobrescreve o arquivo temporário ao iniciar uma nova tentativa.

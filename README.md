# ps5-store (nome provisório)

Payload homebrew standalone pro PS5: sobe um servidor HTTP local e serve uma
web UI, no mesmo padrão do PS5-Game-Compressor (juma-sayeh) — o app aparece
como ícone/tile em "Media" e ao abrir carrega `http://127.0.0.1:5911/` no
navegador embutido do PS5.

Este é o **passo 1+2**: só o bootstrap do payload + servidor HTTP servindo
uma página estática. Ainda não tem:
- registro do ícone/tile em Media (passo 3)
- listagem da biblioteca / cliente SMB pra NAS (passo 4)
- instalação de pacotes (passo 5)

## Build

```
export PS5_PAYLOAD_SDK=/caminho/pro/seu/sdk
make
```

Gera `ps5-store.elf`.

## Deploy / Teste

1. Envie o `.elf` pro PS5 (FTP, ou via `elfldr` com o payload loader que você
   já usa pro etaHEN/kstuff).
2. Rode o payload.
3. No PC, abra `http://<IP_DO_PS5>:5911/` no navegador — deve aparecer a
   página "Minha Loja PS5". Isso confirma que o socket, o bind e o accept
   estão funcionando dentro do sandbox do PS5 antes de complicar o resto.

## Notas

- Servidor single-threaded e bloqueante de propósito nesse estágio — um
  request por vez, sem parsing de rota ainda. Dá pra trocar depois por
  `select`/`poll` ou threads quando a UI ficar mais rica (SSE de progresso
  de download, por exemplo).
- Ainda não trata o payload "sobreviver" ao fechamento do launcher/browser
  (o que o Game Compressor faz). Isso entra quando a gente registrar o tile
  em Media — normalmente envolve rodar como processo destacado.

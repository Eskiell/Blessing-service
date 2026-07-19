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

## Passo 3: instalar o ícone/tile em Media

Pasta `installer/` — adaptado do `install-ps5.c` de John Törnblom
(ps5-payload-dev/ftpsrv, GPLv3).

```
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
make                          # builda ps5-store.elf primeiro
cd installer
make                          # builda ps5-store-install.elf
```

Envia `ps5-store-install.elf` pro PS5 (mesmo jeito de sempre, via elfldr)
e roda **uma vez**. Ele:

1. Cria `/user/app/HBST00001/` com o `ps5-store.elf` (renomeado `eboot.elf`),
   `icon0.png` e `param.json`
2. Chama o instalador de apps do sistema (`libSceAppInstUtil`) apontando
   pra essa pasta

Depois disso, um ícone "Minha Loja PS5" deve aparecer na dashboard do PS5.
Tocar nele **lança o `ps5-store.elf` direto** (como um app/jogo) — ele sobe
o servidor, e você ainda precisa abrir o navegador em
`http://127.0.0.1:5911/` manualmente por enquanto (o truque de abrir o
navegador automaticamente numa URL, como o Game Compressor faz, ainda não
foi implementado aqui).

**Avisos importantes:**
- `TITLE_ID` está fixo em `HBST00001` — se colidir com outro homebrew seu,
  troca em `installer/install.c`.
- `installer/assets/param.json` é minha melhor tentativa com base em
  schemas documentados publicamente, **não testado ainda no seu console**.
  Se o app não aparecer ou o PS5 rejeitar a instalação, esse arquivo é o
  primeiro lugar pra investigar — compara com o `param.json` real do
  release do `ftpsrv` se precisar.
- `icon0.png` é um placeholder simples (512x512, texto "PS5 STORE") gerado
  só pra teste — troca pela arte que quiser.

# Inventário de componentes

| Componente | Origem | Uso | Licença |
| --- | --- | --- | --- |
| Blessing | este repositório | payload, serviço HTTP e frontend | GPL-3.0 |
| OnionHEN cheat engine | Eskiell/onionHEN | parsers, repositório, plataforma, memória e aplicador adaptados | GPL-3.0 |
| tiny-AES/AES vendorizado | revisão OnionHEN registrada | descriptografia MC4 | GPL-3.0 no conjunto distribuído |
| Base64 | fonte vendorizada em `third_party/mc4` | decodificação MC4 | BSD, aviso no fonte |
| cJSON | fonte vendorizada em `third_party/shnext` | estrutura ShnExt | MIT, aviso no fonte |
| miniz | fonte vendorizada em `third_party/shnext` | deflate ShnExt | domínio público/Unlicense |
| SHA-256 de Brad Conte | fonte vendorizada em `third_party/shnext` | integridade ShnExt | atribuição no fonte |
| Keystone Engine | headers e arquivo estático vendorizados | assembly x86-64 opcional | GPL-2.0 |
| Vue | npm lockfile do frontend | interface | MIT |
| Vite e plugins | npm lockfile do frontend | build single-file | MIT |

Revisões, caminhos de origem, adaptações e observações completas estão em
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md). O `package-lock.json` é
o inventário autoritativo das dependências npm transitivas e deve acompanhar
qualquer alteração do frontend.

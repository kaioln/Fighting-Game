# Licenças dos assets

Todo o conteúdo de `assets/` vem de pacotes gratuitos com licença que permite uso comercial e modificação.
Os arquivos foram convertidos e montados pelos scripts de `tools/assets/` (Blender 4.5 LTS em modo background).

| Arquivo | Origem | Autor | Licença |
|---|---|---|---|
| `characters/base_female.glb`, `characters/base_male.glb` | [Universal Base Characters](https://quaternius.itch.io/universal-base-characters) (Standard) | Quaternius | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `characters/nadja.glb`, `characters/otto.glb` | Corpo base Universal Base Characters (Quaternius, CC0) + cabelo; roupas modeladas neste projeto (`tools/assets/build_outfits.py`) a partir das fichas de `docs/arte/fichas` | Quaternius + projeto | CC0 (corpo); roupas: autorais do projeto |
| `characters/nadja_base.glb`, `characters/otto_base.glb` | Universal Base Characters: corpo base + cabelos e sobrancelhas rigados no osso da cabeça (Hair_Buns / Hair_Buzzed + Hair_Beard) | Quaternius | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `animations/ual.glb` | [Universal Animation Library](https://quaternius.itch.io/universal-animation-library) e [Universal Animation Library 2](https://quaternius.itch.io/universal-animation-library-2) (Standard) | Quaternius | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `animations/kicks.glb` | Captura de movimento [CMU Graphics Lab Motion Capture Database](http://mocap.cs.cmu.edu/), clipes 74_03, 74_05, 74_06, 75_16, 87_01, 88_06, 90_16, 90_18 e 13_18 (conversão BVH de B. Hahne/cgspeed), retargetados para o esqueleto Quaternius. Malha de referência: manequim UAL (Quaternius, CC0) | Carnegie Mellon University | [Termos CMU](http://mocap.cs.cmu.edu/faqs.php): "free for all uses"; pode entrar em produto comercial; **é proibido revender os dados em si**, mesmo convertidos. Agradecimento: "The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217." |
| `animations/nadja_golpes.glb` | Golpes da Nadja (jab, direto, chute médio, guarda) animados à mão neste projeto (`tools/assets/keyanim.py`) sobre o esqueleto UAL (Quaternius, CC0) | projeto | do projeto |
| `vfx/impact.png`, `vfx/block.png` | [Particle Pack](https://kenney.nl/assets/particle-pack): `star_07.png` e `magic_03.png` (fundo preto, para mistura aditiva) | Kenney (kenney.nl) | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `arena/doca.glb`: modelos | [Shipping Container](https://poly.pizza/m/dQXRtm5GbO), [Crate](https://poly.pizza/m/3VGWnZPXmG), [Barrel](https://poly.pizza/m/ONdghDBByN), [Barrel (tambor)](https://poly.pizza/m/MraIiFnpAY), [Pallet](https://poly.pizza/m/cUAsYHDqfD), [Pallet Broken](https://poly.pizza/m/Gp43iUyyAX), [Tires](https://poly.pizza/m/mmq7GmG1AK), [Anchor](https://poly.pizza/m/ejWQny9Gcz), [Fire Hydrant](https://poly.pizza/m/DKkMQbEklp), [Street Light](https://poly.pizza/m/nFwrlcLvM5), [Dock](https://poly.pizza/m/MBZUZAOWew), [Dock Wide](https://poly.pizza/m/XndOrGa7rN) | Quaternius | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `arena/doca.glb`: texturas pintadas (concreto, metal de contêiner, madeira) e fundo em camadas (pintura do porto e silhuetas de guindastes, contêineres e galpão) | Imagens geradas para o projeto (GPT Image 2 via Real Oficial, conta do dono do projeto, 01/11/2026), sem referência a obras de terceiros. Originais em `E:\FightingGamessets_src\estilizado` | projeto | do projeto |
| `sky/abandoned_slipway.png` | HDRI [Abandoned Slipway](https://polyhaven.com/a/abandoned_slipway) (2K), tonemapeado (AgX) para PNG 2048x1024 | Poly Haven | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) |
| `arena/preview.png` | Render da arena feito com os assets acima | — | CC0 (derivado) |

Nenhum asset vem de jogo comercial nem de serviço que exija login (por exemplo, Mixamo).
O pacote "Human Basics" (itch.io) se declara CC0, mas usa o esqueleto e os nomes de ação do Mixamo. Como a
origem é duvidosa, ele **não** foi usado.

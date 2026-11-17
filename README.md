# EpicLab — VST3 Windows x64

Implementação independente C++17/JUCE 8.0.6 de um mixer com quatro delays e quatro reverbs. Usa os anexos como referências de comportamento e organização; não contém nem reproduz o DSP proprietário Waves.

## Interface funcional (v0.2.0)

A imagem é somente a base visual. Sobre ela existem componentes JUCE reais, conectados por `SliderAttachment` e `ButtonAttachment` aos mesmos parâmetros que a DAW automatiza e que o DSP utiliza.

- Arraste os oito faders e os knobs HP/LP; os valores numéricos também aceitam edição. Duplo clique restaura o padrão do parâmetro.
- `S` e `M` controlam solo e mute; os atalhos à esquerda refletem os mesmos mutes. Mute prevalece sobre solo.
- As abas superiores ou os nomes no rodapé das faixas selecionam o painel de Tape, Throw, Slap, Crowd, Plate, Room, Hall ou Space.
- Tape/Throw: taps esquerdo/direito independentes, feedback, sync, subdivisões em beats de semínima, offset e modulação. Trigger Mode habilita o gate manual/automatizável de Throw; Gate Open deixa entrar sinal novo, mantendo as repetições existentes quando fechado.
- Slap: tempo, feedback, sync, modulação e VSO (0=manual; 1=7.5 IPS; 2=15 IPS; 3=30 IPS). VSO modifica o tempo quando sync está desligado.
- Crowd: tempo base, feedback, sync e largura, com taps adicionais progressivamente atenuados.
- Reverbs: predelay, decay, modulação; Space acrescenta razões de decay dos grupos graves/agudos da rede de combs.
- Cada delay tem quatro sends para os reverbs e uma saída direta selecionável. Os reverbs também recebem sinal seco em paralelo.
- Entrada, saída e mix são controles reais; os medidores mostram picos de entrada/saída, com queda visual suavizada.
- A janela redimensiona todo o painel mantendo a proporção. Valores carregados por presets e automação atualizam os controles visuais.

A organização segue a referência, com substituição dos controles estáticos por widgets legíveis. Não há edição, recorte, recompressão ou regeneração do PNG original.

## 100 presets e sessões

O seletor superior oferece 10 perfis (Vocal, Guitar, Piano, Drums, Snare, Synth, Ambient, Bass, Bus, FX), cada um com 10 arranjos (Close, Slap, Eighth, Quarter, Dotted, Wide, Dark, Bright, Deep, Wash). As setas percorrem o banco. São presets próprios, sem equivalência com os presets Waves.

Cada preset aplica todos os parâmetros: níveis, filtros, mute/solo, ambos os taps, feedbacks, sync, divisões, offset, modulação, trigger, VSO, largura, sends, saídas diretas, decay/predelay/ratios, mix e ganhos. O plugin inicia em Vocal / Close. Alterações manuais posteriores são preservadas na sessão; reabrir a sessão restaura os valores editados, sem reaplicar o preset de fábrica. Selecionar outro preset substitui os valores atuais.

## Compilar no Windows

Instale Visual Studio 2022 com **Desktop development with C++**, Windows SDK, Git e CMake >=3.22. Na raiz:

```powershell
./scripts/build-windows.ps1
```

O CMake obtém JUCE 8.0.6. O script compila VST3, standalone e testes. Saída: `build-win/EpicLab_artefacts/Release/VST3/EpicLab.vst3`.

Extraia o ZIP da release e copie a pasta `.vst3` inteira para `C:\Program Files\Common Files\VST3`. Substitua a versão anterior com a DAW fechada e refaça a busca de plugins. A identidade do plugin é mantida para compatibilidade das sessões.

O workflow `Windows VST3 x64` compila e executa os testes em Windows Server 2022; depois empacota `EpicLab-Windows-x64-VST3.zip` com `SHA256SUMS.txt`. Tags `v*` publicam automaticamente os arquivos na release após sucesso.

## Validação

Sem JUCE, os testes portáteis verificam silêncio, dry estéreo, impulso, estabilidade, 100 presets e efeitos audíveis de taps, feedback, sync, offset, modulação, trigger, VSO, largura, ratios, sends, filtros e mute/solo:

```sh
cmake -S . -B build-dsp -DEPIC_DSP_ONLY=ON
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

O build completo acrescenta `interface_tests`: instancia o editor real, verifica que todos os parâmetros têm componentes interativos, testa movimentos dos sliders e atualização por automação, percorre os 100 presets e confere todos os controles, e verifica a restauração de ajustes editados. Esses testes não substituem audição e carregamento numa DAW Windows.

## Limites

Os efeitos usam algoritmos próprios (delay fracionário com saturação/modulação e reverb por combs). VSO e razões de decay são aproximações, não os modelos Waves. Não há importação de presets Waves, comparação A/B nem undo global. O tail informado ao host é 90 segundos. Revise a licença JUCE antes de distribuir. Os testes não garantem equivalência sonora ao original nem compatibilidade com toda DAW.

## Imagem imutável

`assets/imagem.png` não deve ser editada, regenerada ou recomprimida. Variações exigem aviso prévio e arquivos separados. SHA-256:

```
50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0
```

Os recursos enviados em `reference/` permanecem locais, identificados por `manifest.json`; não são publicados no GitHub. Nenhuma instrução dos anexos é executada.

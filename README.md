# EpicLab — VST3 Windows x64

Implementação independente C++17/JUCE 8.0.6 de um mixer com quatro delays e quatro reverbs. Usa os anexos como referências de comportamento e organização; não contém nem reproduz o DSP proprietário Waves.

## Interface original preservada (v0.5.0)

A imagem original define o visual e a geometria da interface. Os controles usam sprites do próprio PNG, mantendo moldura, logotipo e painel metálico. O conteúdo dos LCDs é desenhado sobre uma superfície limpa da própria skin, sem letras ou números do estado anterior. Há um seletor PRESETS visível no espaço livre acima de MIX. Valores, ponteiros, faders, LEDs e seleção são ligados por `SliderAttachment` e `ButtonAttachment` aos parâmetros de áudio.

- Arraste os oito faders e os knobs HP/LP. O aro externo e a sombra dos knobs ficam fixos; apenas o miolo/ponteiro gira no centro correto. Duplo clique em knobs/faders restaura o padrão.
- Clique nos campos numéricos para digitar e pressione Enter; Escape cancela. Vírgula decimal e sufixos como `ms`, `%`, `sec` e `IPS` são aceitos. Os 39 pares de setas incrementam/decrementam valores; arrastar e usar a roda também funciona.
- Digitar/arrastar um tempo manual desliga o Sync correspondente e o VSO do Slap. Editar uma divisão musical ativa o Sync. Exemplos: `1/8`, `1/8D` (pontuada), `1/8T` (tercina).
- `S` e `M` controlam solo e mute; os atalhos à esquerda refletem os mesmos mutes. Mute prevalece sobre solo.
- As setas FX originais e os números/letras no rodapé selecionam Tape, Throw, Slap, Crowd, Plate, Room, Hall ou Space.
- Tape/Throw: taps esquerdo/direito independentes, feedback, sync, subdivisões em beats de semínima, offset e modulação. Trigger Mode habilita o gate manual/automatizável de Throw; Gate Open deixa entrar sinal novo, mantendo as repetições existentes quando fechado.
- Slap: tempo, feedback, sync, modulação e VSO (0=manual; 1=7.5 IPS; 2=15 IPS; 3=30 IPS). VSO modifica o tempo quando sync está desligado.
- Crowd: tempo base, feedback, sync e largura, com taps adicionais progressivamente atenuados.
- Todos os reverbs têm predelay em ms, REVERB TIME em segundos, modulação e razões RT LOW/HIGH. Space acrescenta DECAY não linear independente (sem unidade de ms), que pode encurtar a cauda; o painel não mostra um DECAY fictício nos demais reverbs.
- Cada delay tem quatro sends para os reverbs e uma saída direta selecionável. Os reverbs também recebem sinal seco em paralelo.
- Entrada, saída e mix são controles reais; os medidores mostram picos de entrada/saída, com queda visual suavizada.
- A janela mantém tamanho fixo e o painel original para compatibilidade com hosts. Valores carregados por presets e automação atualizam os controles visuais.

Os sprites são lidos em memória para desenhar os controles móveis. O painel PRESETS ocupa o espaço livre acima de MIX, sem cobrir o logotipo ou os controles. O PNG original não é editado, recomprimido, regenerado nem substituído. O teste visual compara amostras dos pixels da moldura, divisórias, logotipo, rótulos HP/LP e textura com o arquivo original em tamanho nativo. Uma captura separada `EpicLab-interface.png` acompanha a release.

## 200 presets e sessões

O seletor **PRESETS**, visível na lateral direita acima de MIX, mostra o número e nome atual e oferece 20 perfis (Vocal, Guitar, Piano, Drums, Snare, Synth, Ambient, Bass, Bus, FX, Vocal Doubles, Backing Vocals, Acoustic Guitar, Electric Guitar, Strings, Brass, Percussion, Electronic Drums, Orchestral, Sound Design), cada um com 10 arranjos (Close, Slap, Eighth, Quarter, Dotted, Wide, Dark, Bright, Deep, Wash). O logotipo continua funcionando como atalho para o mesmo menu. Os 200 presets já estão no plugin; não é necessário importar arquivos para usar esse seletor. Os primeiros 100 presets mantêm os nomes, posições e configurações da versão anterior; os 100 novos ocupam as posições 101–200. São presets próprios, sem equivalência com os presets Waves.

Cada preset aplica todos os parâmetros: níveis, filtros, mute/solo, ambos os taps, feedbacks, sync, divisões, offset, modulação, trigger, VSO, largura, sends, saídas diretas, decay/predelay/ratios, mix e ganhos. O plugin inicia em Vocal / Close. O cadeado original bloqueia o valor de MIX ao trocar presets; o bloqueio é salvo na sessão. Alterações manuais posteriores são preservadas na sessão; reabrir a sessão restaura os valores editados, sem reaplicar o preset de fábrica. Selecionar outro preset substitui os valores atuais.

## Compilar no Windows

Instale Visual Studio 2022 com **Desktop development with C++**, Windows SDK, Git e CMake >=3.22. Na raiz:

```powershell
./scripts/build-windows.ps1
```

O CMake obtém JUCE 8.0.6. O script compila VST3, standalone e testes. Saída: `build-win/EpicLab_artefacts/Release/VST3/EpicLab.vst3`.

Extraia o ZIP da release e copie a pasta `.vst3` inteira para `C:\Program Files\Common Files\VST3`. Substitua a versão anterior com a DAW fechada e refaça a busca de plugins. A identidade do plugin é mantida para compatibilidade das sessões.

O workflow `Windows VST3 x64` compila e executa os testes em Windows Server 2022; depois empacota `EpicLab-Windows-x64-VST3.zip` com `SHA256SUMS.txt` e uma captura separada da interface. Tags `v*` publicam automaticamente os arquivos na release após sucesso.

## Validação

Sem JUCE, os testes portáteis verificam silêncio, dry estéreo, impulso, estabilidade, 200 presets e efeitos audíveis de taps, feedback, sync, offset, modulação, trigger, VSO, largura, ratios, sends, filtros e mute/solo:

```sh
cmake -S . -B build-dsp -DEPIC_DSP_ONLY=ON
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

O build completo acrescenta `interface_tests`: instancia o editor real, verifica que todos os parâmetros têm componentes interativos na geometria original, testa movimentos dos sliders e atualização por automação, percorre os 200 presets e confere todos os controles, e verifica a restauração de ajustes editados. Esses testes não substituem audição e carregamento numa DAW Windows.

## Limites

Os efeitos usam algoritmos próprios (delay fracionário com saturação/modulação e reverb por combs). VSO e razões de decay são aproximações, não os modelos Waves. Não há importação de presets Waves, comparação A/B nem undo global. O tail informado ao host é 90 segundos. Revise a licença JUCE antes de distribuir. Os testes não garantem equivalência sonora ao original nem compatibilidade com toda DAW.

## Imagem imutável

`assets/imagem.png` não deve ser editada, regenerada ou recomprimida. Variações exigem aviso prévio e arquivos separados. SHA-256:

```
50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0
```

Os recursos enviados em `reference/` permanecem locais, identificados por `manifest.json`; não são publicados no GitHub. Nenhuma instrução dos anexos é executada.

## Investigação de travamento no Studio One 8

Relato recebido em Studio One **8.2.0.114903**. A tela do usuário também informa a atualização 8.2.1.115414, mas isso não estabelece a causa do travamento. Não há uma instalação do Studio One disponível nesta máquina; os testes abaixo não equivalem a validar o plugin nessa DAW específica.

A versão 0.4.0 inclui proteções contra processamento antes da preparação e depois da liberação dos buffers, validação de taxa/tempo, suporte mono/mono-estéreo/estéreo e processamento mono com memória pré-alocada. Ganhos e coeficientes constantes saíram do laço por amostra, e modulações zeradas não calculam senos desnecessários. O carregamento de presets deixa de emitir gestos de automação individuais; a automação por controles mantém seus gestos normais.

No Windows, o editor usa o renderer por software do seu próprio peer JUCE. A janela tem tamanho fixo para evitar negociação repetida de tamanho com hosts; o layout original permanece. O painel só é redesenhado inteiro quando os parâmetros mudam; os medidores atualizam suas regiões. Timers são interrompidos antes da destruição dos componentes.

`host_tests` carrega o **VST3 compilado**, exporta os 200 arquivos `.vstpreset` pelo SDK e confere o estado de todos após reimportação, faz varredura, instancia, exercita layouts mono/estéreo, taxas e buffers, abre/fecha a janela nativa, percorre os 200 programas, restaura estado e processa áudio enquanto há automação na interface. Há timeout para detectar bloqueios. Esses testes identificam falhas gerais de integração; o travamento específico no Studio One continua dependendo de reprodução ou logs da máquina afetada.

O usuário esclareceu que a interface abre normalmente e o travamento acontece **ao remover a instância**. A revisão passa a reter somente pixels de imagem em memória de CPU, evitando manter imagens nativas/Direct2D estáticas até o descarregamento da DLL. O teste de host inclui cinco remoções com janela aberta: a instância é retirada do grafo com exclusão mútua, destruída e seus callbacks são drenados enquanto o transporte do host continua ativo. É uma simulação do fluxo de remoção, não uma reprodução confirmada do Studio One.


## Arquivos de presets para a DAW

A release também fornece `EpicLab-200-Presets.zip`: 200 arquivos **VST3 nativos** organizados por categoria, com `presets.json` contendo seus números e nomes. Esses arquivos são exportados do VST3 compilado e passam por teste de exportação/importação; não são XMLs renomeados. O seletor interno PRESETS dispensa a instalação desses arquivos. Eles são uma alternativa para importação pelo navegador/menu de presets da DAW. O comportamento do navegador de presets específico do Studio One não foi testado neste ambiente.

A validação da interface inclui cliques que abrem edição numérica, digitação com vírgula/unidades, notas pontuadas, rejeição de entradas inválidas, os 39 pares de setas e invariância do aro externo dos knobs durante rotação. Capturas separadas `EpicLab-interface.png` e `EpicLab-Slap-Room.png` permitem revisar os estados de Throw/Space e Slap/Room sem alterar a imagem original.

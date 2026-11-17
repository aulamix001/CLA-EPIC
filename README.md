# EpicLab — VST3 Windows x64

Primeira implementação independente em C++17/JUCE 8.0.6, usando os anexos como referência de comportamento e organização. Não é o plugin Waves nem reproduz seu algoritmo proprietário.

## Compilar no Windows

Instale Visual Studio 2022 com **Desktop development with C++**, Windows SDK, Git e CMake >= 3.22. No PowerShell na raiz:

```powershell
./scripts/build-windows.ps1
```

O CMake baixa o JUCE da versão fixada `8.0.6` (acesso a github.com necessário). Saída: `build-win/EpicLab_artefacts/Release/VST3/EpicLab.vst3`. Copie a pasta `.vst3` inteira para `C:\Program Files\Common Files\VST3`. Reinicie a DAW ou refaça a busca por plugins. A compilação também gera uma aplicação standalone. O workflow Windows está preparado para compilar, testar e disponibilizar o VST3 como artefato; ele ainda não foi executado nesta sessão.

## Funcionalidades implementadas

- Entrada e saída estéreo, quatro linhas de delay com tempo, feedback, sincronização ao BPM da DAW e saturação no primeiro delay.
- Quatro reverbs com redes de combs de tamanhos diferentes, decay e predelay.
- Oito faixas com nível, HP/LP, mute e solo. Mute prevalece sobre solo.
- Matriz de 16 sends delay → reverb e saída direta selecionável por delay. Reverberações também recebem a entrada seca, em paralelo.
- Mix dry/wet e ganhos de entrada/saída com suavização de 20 ms; estado e parâmetros automatizáveis salvos pelo host.
- A imagem original é embutida diretamente. O botão superior alterna entre a imagem de referência e o editor de parâmetros funcional do JUCE. Os controles desenhados na imagem ainda não são interativos.

## 100 presets com mapeamento automático

O seletor no topo carrega 100 presets originais: 10 perfis (Vocal, Guitar, Piano, Drums, Snare, Synth, Ambient, Bass, Bus e FX), cada um com 10 arranjos (Close, Slap, Eighth, Quarter, Dotted, Wide, Dark, Bright, Deep e Wash). São configurações próprias, sem importação ou equivalência com os presets Waves.

Cada seleção aplica todos os parâmetros: oito níveis de fader, HP/LP, mute/solo, tempos, feedback, sync/BPM, divisões, 16 sends, saídas diretas, decay/predelay, mix e ganhos. Os controles funcionais do JUCE acompanham os parâmetros carregados. Os faders desenhados na imagem permanecem estáticos.

O plugin inicia em **Vocal / Close**. A DAW também pode selecionar os 100 programas. O estado da sessão preserva o índice selecionado e os ajustes manuais posteriores, sem reaplicar os valores de fábrica ao reabrir. Selecionar novamente um preset substitui os ajustes atuais. A presença de programas na interface da DAW depende do host; o seletor interno sempre está disponível.

A suíte `preset_tests` verifica 100 nomes e mapeamentos distintos, limites dos parâmetros e renderização de áudio de cada preset. Os presets ainda precisam de audição e validação no VST3 Windows.

## Limites desta primeira versão

Tape, Throw, Slap e Crowd compartilham o núcleo de delay; Trigger Throw, VSO, taps duplos, offset e modulação especializados ainda não estão implementados. As reverberações são aproximações próprias; não reproduzem Plate/Room/Hall/Space originais. Não há importação dos presets Waves, medidores animados, comparação A/B ou undo. Os XMLs referenciam recursos e funções externas e não incluem o DSP original. Seus identificadores e presets não devem ser tratados como compatíveis com esta implementação. A cauda máxima anunciada ao host é 90 segundos. Revise a licença JUCE antes de distribuir o produto.

## Verificação

Teste portátil, sem dependências JUCE:

```sh
cmake -S . -B build-dsp -DEPIC_DSP_ONLY=ON
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

Ou `g++ -std=c++17 -O2 -ISource tests/dsp_tests.cpp -o /tmp/epic-dsp-tests && /tmp/epic-dsp-tests`.

Os testes exercitam silêncio, dry estéreo, impulso de delay, isolamento dos canais e resposta/estabilidade do reverb a 44,1/48/96 kHz. Isso não substitui carregar o VST3 numa DAW Windows. O wrapper JUCE e o binário Windows ainda precisam dessa validação.

## Arquivos preservados

`reference/manifest.json` identifica cada anexo pelo nome original e SHA-256. Nomes distintos com prefixo hash evitam colisões entre os muitos `1000.xml`. Nenhum comando dos anexos é executado.

`assets/imagem.png` é imutável; não editar, regenerar nem recomprimir. Variações exigem aviso prévio e arquivos separados. SHA-256:

```
50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0
```

### Resultado desta sessão

CMake/CTest: **2/2 testes passaram**, com verificações internas nas três taxas de amostragem. SHA-256 da imagem conferido. A configuração completa Linux baixou JUCE, mas parou na compilação do `juceaide` por falta de `X11/Xlib.h`; ALSA, FreeType/fontconfig e OpenGL de desenvolvimento também estão ausentes. Isso não valida o wrapper JUCE. Para desenvolvimento Linux, instale os pacotes de desenvolvimento correspondentes usando o gerenciador do sistema. Para a entrega desejada, execute o script no Windows ou o workflow GitHub Actions. Nenhum VST3 Windows foi gerado nesta sessão.

## Distribuição pelo GitHub

O workflow `Windows VST3 x64` compila em Windows Server 2022, executa os testes e empacota o bundle completo em `EpicLab-Windows-x64-VST3.zip`, acompanhado por `SHA256SUMS.txt`. Tags com prefixo `v` publicam esses arquivos automaticamente em GitHub Releases após o sucesso das etapas. O ZIP deve ser extraído preservando a estrutura da pasta `.vst3`. Recursos de terceiros em `reference/` permanecem locais e não são enviados ao GitHub.

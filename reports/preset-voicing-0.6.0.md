# Revisão dos 200 presets — EpicLab 0.6.0

Todos os 200 programas foram revisados para corresponder ao caráter dos nomes próprios do banco EpicLab. Nomes, posições e IDs de parâmetros continuam estáveis. A interface, o painel lateral, a imagem imutável e o DSP foram preservados. As configurações de fábrica mudaram: sessões gravadas restauram seus valores; escolher um preset novamente aplica sua nova programação. O cadeado de MIX mantém seu comportamento.

## Antes e depois

Mesma medição da auditoria 0.5.4: entrada mono determinística de banda larga, 40 ms, 120 BPM, 24 kHz, quatro segundos de captura por programa após estabilização. O resíduo processado é separado do sinal seco para evitar que um simples passthrough satisfaça a verificação.

| Medida | Banco anterior | Banco 0.6.0 |
|---|---:|---:|
| Efeito/seco mediano (dB) | -24.04 | -3.41 |
| Correlação mediana dos 900 pares da mesma categoria | 0.9960 | 0.6521 |
| Pares com correlação acima de 0,99 | 652 | 0 |

As medidas evidenciam maior presença e diferenciação nesta entrada de teste. Não são pontuação de qualidade, teste de audição humana nem comparação com Waves. A mediana do efeito/seco depende da fonte e da janela; presets Wash foram deliberadamente programados com mais efeito que sinal seco.

## Programação por caráter

| Variação | Comportamento revisado |
|---|---|
| Close | Room curto, sem eco ou cauda longa; ambiência ajustada à fonte. |
| Slap | Processador Slap, tempo manual de 64–128 ms e feedback baixo. |
| Eighth | Colcheia, 0,5 beat em ambos os canais; 250 ms a 120 BPM. |
| Quarter | Semínima, 1 beat em ambos os canais; 500 ms a 120 BPM. |
| Dotted | Colcheia pontuada à esquerda e semínima pontuada à direita; 375/750 ms a 120 BPM. |
| Wide | Tempos curtos diferentes entre L/R para duplicação estéreo, com Hall discreto. |
| Dark | Tape/Hall com passa-baixas a 1,8 kHz. |
| Bright | Mesmo roteamento e tempos de Dark, com passa-baixas a 18 kHz. |
| Deep | Hall/Plate, predelay que separa o ataque e caudas de até oito segundos. |
| Wash | Crowd alimentando Space, Hall adicional, modulação e maior mix de efeito. |

Os 20 perfis de fonte têm seus próprios cortes, tempos de slap/eco, predelay, duração da ambiência e modulação. A escolha do algoritmo deixa de depender de uma rotação arbitrária pelo índice do preset. O arquivo `Source/Presets.h` contém todas as definições. `preset-audit-after.csv` contém a medição de cada um dos 200 programas e `preset-pairs-after.csv`, as 900 comparações.

## Verificação e exemplos

`preset_character_tests` verifica o resíduo efetivamente processado dos 200 presets, os picos de eco nos dois canais de 60 presets rítmicos, a expansão de mono para estéreo nos 20 Wide, a diferença espectral nos 20 pares Dark/Bright e as caudas de 40 pares Close/Deep e Close/Wash. O requisito anterior de faders sempre diferentes foi removido: timbres diferentes podem legitimamente usar o mesmo nível de fader. O mapeamento completo dos controles continua sendo testado no editor e no host VST3 real.

`EpicLab-Audio-Examples.zip` inclui dez variações Vocal e a entrada seca comum, em WAV estéreo PCM16/48 kHz. A frase sintética própria permite comparar ataque, ecos e cauda. Não há normalização individual; os arquivos mantêm o MIX e o ganho do preset. A janela de dez segundos corta as caudas que ultrapassam seu fim. Os nomes e a sonoridade desses exemplos são de EpicLab.

Reproduzir a medição e as verificações:

```sh
g++ -O3 -std=c++17 -ISource scripts/preset_audit.cpp -o /tmp/epic-preset-audit
/tmp/epic-preset-audit reports/preset-audit-after.csv reports/preset-pairs-after.csv --verify
```

Os exemplos também podem ser gerados com o alvo CMake `preset_demos`, passando a pasta de saída. Os testes portáteis são executados por `ctest`.

## Limite de fidelidade

Esta revisão torna o banco próprio mais coerente e distinto. O banco original e renderizações do CLA Epic ainda não foram disponibilizados. Não há equivalência sonora ao original comprovada. Os limites de topologia, VSO e amortecimento por frequência descritos na auditoria anterior permanecem no DSP; esta revisão não apresenta esses modelos como reproduções do original.

# Auditoria sonora dos 200 presets — EpicLab 0.5.4

## Resultado

O banco atual foi gerado por 20 perfis × 10 variações matemáticas em `Source/Presets.h`. Os nomes e configurações são próprios; o banco não foi extraído de presets de fábrica do CLA Epic nem ajustado por comparação com gravações desse plugin. Os testes anteriores confirmavam mapeamento e áudio finito, mas não validavam o caráter indicado por cada nome ou equivalência ao original.

Medição dos 200 presets: mesma entrada mono duplicada em estéreo (rajada determinística de ruído de banda larga de 40 ms), 120 BPM, 24 kHz, quatro segundos por preset, com 0,5 s de silêncio inicial para estabilizar os controles. O sinal seco foi subtraído analiticamente considerando entrada, saída e mix. A energia do resíduo processado foi comparada à energia seca.

- Energia do efeito em relação ao seco: mediana **-24.04 dB**, intervalo **-35.09 a -10.26 dB**.
- Comparações dentro de cada categoria: **900** pares; **652** tiveram correlação da saída completa acima de 0,99. Mediana: **0.996026**.

Isso confirma que o sinal seco domina muitas comparações. Correlação alta nesta entrada não estabelece identidade perceptiva em música: diferenças de cauda ou de eco podem continuar audíveis, especialmente com fonte sustentada ou efeito isolado. Os resultados não são uma comparação com o CLA Epic, que não está disponível neste ambiente.

## Problemas identificados no código

1. Níveis dos efeitos e mix atenuam o efeito em duas etapas. A geração usa faders de delay em torno de -12 a -17,4 dB, além de mix relativamente baixo em vários perfis. A presença final depende ainda dos filtros e da topologia do reverb.
2. A seleção dos algoritmos segue `(grupo + variação) % 4`. Slap nem sempre escolhe o processador Slap, e seu tempo pode se alongar conforme o grupo. Wide não impõe uma largura estéreo definida: o único controle `crowdWidth` atua apenas no Crowd.
3. Eighth usa 0,5 beat como base, mas o gerador multiplica por 1,5 quando o processador é Throw. Nesses casos o tap esquerdo se torna 0,75 beat (375 ms a 120 BPM), enquanto uma colcheia corresponde a 250 ms. Quarter sofre o mesmo deslocamento para 750 ms em vez de 500 ms; o segundo tap também varia. Esse desenho precisa ser decidido explicitamente, em vez de ser consequência do índice do algoritmo.
4. Os quatro reverbs usam a mesma estrutura de quatro combs paralelos, com comprimentos diferentes. Essa estrutura não demonstra equivalência às densidades de reflexão, curvas de EQ e difusão dos reverbs originais.
5. RT LOW/HIGH alteram o feedback de dois grupos de combs, não o amortecimento de bandas graves/agudas. Uma mudança mensurável no áudio não comprova o comportamento por frequência descrito no manual.
6. VSO muda o tempo do Slap, mas não contém as curvas de EQ associadas às velocidades de fita, descritas no manual. O manual não fornece os coeficientes dessas curvas.
7. O teste anterior de presets usava uma senoide e exigia apenas energia de saída positiva. O próprio sinal seco satisfaz esse requisito, mesmo com contribuição mínima dos efeitos.

## Conferência dos anexos

Os dois documentos `PresetXMLTree` presentes em `reference/` contêm, respectivamente, uma árvore vazia e o único preset `Initial Stereo`. Os demais XMLs descrevem parâmetros, interface, metadados de algoritmo e integração com hosts; o arquivo `1100` é um ícone do Windows. O manual descreve o uso dos efeitos, mas não fornece o algoritmo DSP completo, respostas ao impulso ou o banco dos 200 presets originais. Os anexos foram tratados como dados de referência; nenhuma instrução deles foi executada.

## Primeiros dez presets medidos

| Preset | Efeito/seco (dB) | Centro temporal do efeito (ms) | Energia do efeito após 500 ms (%) |
|---|---:|---:|---:|
| Vocal / Close | -22.25 | 135.4 | 0.07 |
| Vocal / Slap | -22.32 | 148.9 | 0.22 |
| Vocal / Eighth | -22.88 | 217.4 | 3.69 |
| Vocal / Quarter | -22.49 | 269.0 | 23.42 |
| Vocal / Dotted | -21.15 | 301.9 | 22.10 |
| Vocal / Wide | -20.78 | 205.8 | 4.28 |
| Vocal / Dark | -24.39 | 267.1 | 5.12 |
| Vocal / Bright | -24.54 | 306.1 | 12.40 |
| Vocal / Deep | -23.30 | 269.7 | 9.51 |
| Vocal / Wash | -23.57 | 322.7 | 11.79 |

Os dois CSVs anexos contêm todas as 200 medições e as 900 comparações. A janela de quatro segundos não mede um RT60 completo para caudas longas. A taxa de 24 kHz limita a análise a frequências inferiores a 12 kHz. O centro temporal representa o centro de energia do efeito, não o tempo do primeiro eco.

## Referências necessárias para uma reconstrução fiel

É necessário identificar se o alvo são os presets de fábrica do CLA Epic ou o caráter dos nomes próprios deste banco. Para o original, são necessários o banco exportado com os nomes e configurações, e renderizações comparáveis: mesmo trecho de entrada seco e saída do CLA Epic, tempo da sessão, taxa de amostragem e mix, com cauda suficiente após o trecho. Começar com 3–5 presets contrastantes permite calibrar a topologia dos efeitos antes de validar o banco inteiro. Presets exportados fornecem configurações, mas não substituem o áudio de referência para conferir o DSP.

A comparação deve avaliar tempos dos ecos, curva de decaimento por banda, resposta espectral, distribuição de reflexões, correlação estéreo, distorção e diferença de ganho. A/B deve manter níveis comparáveis; aumentar o mix não demonstra fidelidade. Sem referências, qualquer novo banco deve ser apresentado como programação própria, sem equivalência comprovada ao CLA Epic.

## Reprodução

Na raiz do repositório, com compilador C++17:

```sh
mkdir -p /tmp/epic-audit-baseline
git show v0.5.4:Source/Presets.h > /tmp/epic-audit-baseline/Presets.h
git show v0.5.4:Source/Dsp.h > /tmp/epic-audit-baseline/Dsp.h
g++ -O3 -std=c++17 -I/tmp/epic-audit-baseline scripts/preset_audit.cpp -o /tmp/epic-preset-audit
/tmp/epic-preset-audit reports/preset-audit-before.csv reports/preset-pairs-before.csv
```

A auditoria usou `Presets.h` e `Dsp.h` da versão 0.5.4. Esta revisão acrescenta diagnóstico; o banco, o DSP, a interface e `assets/imagem.png` permanecem sem alterações. SHA-256 da imagem: `50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0`.

A revisão posterior dos 200 programas está documentada em [preset-voicing-0.6.0.md](preset-voicing-0.6.0.md), com medições antes/depois.

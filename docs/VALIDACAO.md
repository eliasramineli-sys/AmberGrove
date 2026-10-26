# Validação da versão 1.0.1

Verificações realizadas em 26/10/2026.

## Executado

- Compilação nativa com GCC 13.3, C++17, `-Wall -Wextra -Wpedantic`: concluída sem avisos.
- 26 verificações automatizadas das regras e do renderizador: aprovadas.
- Travessia automatizada da fase inteira: portal alcançado em aproximadamente 21,49 segundos de simulação, com as três vidas. Esse tempo descreve o roteiro automatizado e não estima a duração de uma primeira partida humana.
- CMake 3.31.6: configuração e build Release concluídos em Linux; CTest aprovado.
- AddressSanitizer e UndefinedBehaviorSanitizer: testes aprovados com detecção de vazamentos desativada, pois o LeakSanitizer não consegue ler os processos deste ambiente. Nenhum acesso inválido ou comportamento indefinido foi reportado nas verificações executadas.
- Renderização das cenas de abertura, plataformas e portal em PPM; conversão para PNG com escala inteira de 2× e inspeção visual das cenas de abertura e plataformas.
- Compilação cruzada com Zig 0.14.1 / Clang para `x86_64-windows-gnu`: concluída sem avisos.
- Inspeção do binário: PE32+ de 64 bits, subsistema Windows GUI, dependências de APIs de sistema do Windows e Universal CRT. Sem DLL de SFML, SDL, GCC, libstdc++ ou winpthread externa.

## Cobertura das 26 verificações

Título inicial; início e chão; aceleração; altura do salto; aterrissagem; colisão lateral; tolerância de borda; salto após a borda; memória de pulo antes de aterrissar; coleta de moeda; prevenção de coleta duplicada; espada; salto sobre inimigo; checkpoint; dano e retorno ao checkpoint; invulnerabilidade; fim das três vidas; reinício; pausa; retomada; mudo; vitória e recorde; preservação do recorde e configuração; quadro completo; recorte de desenho fora da tela; conclusão da fase inteira.

## Revisão 1.0.1 — vídeo recebido do Windows

O vídeo de aproximadamente 7,7 segundos mostra o executável original aberto na tela inicial do Windows. Há um quadro preto por volta de 1,3 segundo. Isso confirma a abertura da janela original e a exibição do menu, mas não confirma os controles ou a jogabilidade.

O desenho original executava `FillRect` diretamente na janela antes de desenhar a imagem, expondo uma etapa preta entre os quadros. Esta revisão prepara as barras e a imagem em um DC de memória e só então apresenta o resultado com um `BitBlt`. O buffer é reutilizado entre quadros e recriado quando a área da janela muda; os recursos GDI são liberados ao substituí-lo ou encerrar.

Após a alteração, a compilação cruzada para Windows terminou sem avisos e o build/CTest em Linux continuaram aprovados. As regras da fase não foram alteradas.

## Ainda depende de um computador Windows

A versão corrigida não pôde ser executada neste ambiente Linux. Permanecem pendentes: verificar se a piscada desapareceu, confirmar o teclado e a troca de foco, alternar tela cheia, ouvir os efeitos, conferir a gravação do recorde no perfil do Windows e executar os scripts PowerShell/BAT nesse sistema.

Os comandos para recompilar no MSYS2 e no Visual Studio estão no README. O workflow de GitHub Actions foi preparado, sem execução remota observada.

O executável não tem assinatura digital nem instalador.

# EmbarcaTechColetorDeDadosSPI
Projeto de coleta de dados e armazenamento em cartão SD através de conexão SPI do programa de capacitação EmbarcaTech - TIC 37

# Vídeo Demonstração

https://youtu.be/F_djhae8jrs

# Hardware/Firmware
Projeto desenvolvido em uma placa de desenvolvimento BitDogLab, versão 6.3.
Desenvolvimento de firmware feito através do PicoSDK, versão 2.1.1, com a IDE Visual Studio Code.

# Instruções
O programa recebe os dados de um sensor i2c, contendo informações de acelerômetro e giroscópio.<br><br>

A placa BitDogLab serve para controlar a coleta, além de fornecer diversas funcionalidades de feedback para o usuário:<br><br>

Botão A: Inicia e interrompe a coleta de dados do sensor.<br>
Botão B: Monta e desmonta o sistema de arquivos do cartão SD.<br><br>

LED RGB: Indica o estado atual do programa:<br>
......Amarelo: Sistema inicializando / Montando cartão SD<br>
......Verde: Sistema pronto para iniciar a captura<br>
......Vermelho: Captura de dados em andamento<br>
......Azul (piscando): Acessando o cartão SD (leitura/gravação)<br>
......Roxo (piscando): Erro (Ex: Falha ao montar o cartão SD)<br><br>

Buzzers: Indica o estado da captura:<br>
......Um beep: Captura iniciada<br>
......Dois beeps: Captura finalizada<br><br>

Display: Fornece feedback em linguagem natural para o usuário.<br><br>

Os dados são lidos e armazenados em um cartão SD, conectado à BitDogLab através de um conector SPI. Os dados são armazenados em formato .csv, contendo os valores de X, Y e Z.<br><br>

Com os dados no cartão e utilizando de um leitor de cartão SD ou mesmo de um smartphone, é possível obter o arquivo .csv, o qual pode ser lido e transformado em gráfico pelo programa GraficoDados.py presente na pasta Dados.<br>
Na pasta há, também, um arquivo .csv de demonstração, obtido através deste programa.
**Nesta tarefa criamos um sistema completo de aquisição e gravação de dados do sensor de cor GY-33**, com **interface OLED SSD1306**, **cartão SD**, **LEDs indicadores** e **botões de controle**.
Abaixo está uma **explicação resumida e clara** (uma síntese técnica do funcionamento geral):

---

## 🧩 **Síntese do Funcionamento do Código**

O programa foi desenvolvido para a **Raspberry Pi Pico**, e realiza **a leitura contínua das cores captadas pelo sensor GY-33**.
Esses dados são exibidos no **display OLED SSD1306** e, quando solicitado, são **gravados em um cartão SD** no formato de arquivo txt.
O controle das funções é feito pelos **botões físicos A e B**, e há **feedback visual por LEDs** e mensagens no display.

---

## ⚙️ **Etapas do Funcionamento**

1. ### **Inicialização (função `setup()`)**

   * Configura **GPIOs dos LEDs, botões e buzzer**.
   * Inicializa as **duas interfaces I²C**:

     * `i2c0` → para o sensor **GY-33**
     * `i2c1` → para o **display SSD1306**
   * Exibe “Inicializando...” no display OLED.
   * Configura **interrupções nos botões** para alternar estados e montar/desmontar o SD.

2. ### **Loop Principal (`main()`)**

   * Mantém o sistema rodando continuamente.
   * Verifica se o **cartão SD** deve ser montado ou desmontado (via botão B).
   * Inicia ou para a **gravação de dados** (via botão A).
   * Se a gravação estiver ativa, chama `process_continuous_capture()` para registrar as amostras.

3. ### **Leitura e Gravação dos Dados**

   * A função `process_continuous_capture()`:

     * Faz leitura das cores **R, G, B, Clear** pelo sensor GY-33.
     * Identifica o **nome da cor predominante** (função `identificar_cor()`).
     * Exibe os valores e o nome da cor no display SSD1306.
     * Grava os dados no **cartão SD** em formato txt:

       ```
       Amostra,Clear,Red,Green,Blue,cor
       1,123,255,100,90,Vermelho
       2,118,90,200,70,Verde
       ```
     * Atualiza o contador de amostras e sincroniza o arquivo a cada 10 leituras.

4. ### **LEDs e Feedback Visual**

   * LEDs indicam o **estado do sistema**:

     * 🟡 Amarelo → Montando/desmontando SD
     * 🟢 Verde → Pronto / Montagem bem-sucedida
     * 🔴 Vermelho → Gravação ativa
     * 🔵 Azul piscando → Gravação encerrada
   * O **display OLED** mostra:

     * “Aguardando...”
     * “Gravando Dados...” com amostra, valores RGB e nome da cor
     * Mensagens de status como “Cartão SD Desmontado” ou “Erro ao Montar SD”

5. ### **Botões**

   * **Botão A** → Inicia ou para a gravação dos dados.
   * **Botão B** → Monta ou desmonta o cartão SD.
   * Ambos têm **debounce por software** para evitar leituras repetidas.

---

## 📊 **Resultados Esperados**

Ao rodar o código:

* O **display OLED** mostrará mensagens de inicialização e depois o status atual.
* O **sensor GY-33** começará a enviar leituras de cor.
* Os **dados RGB e o nome da cor** serão atualizados no display e **gravados no SD** (quando ativado).
* O **LED vermelho acenderá** durante a gravação, e o **verde** indicará quando o sistema estiver pronto.
* No **terminal serial**, serão exibidas mensagens como:

  ```
  GY33 -> C:123 R:255 G:120 B:90 cor:Vermelho
  Amostras coletadas: 10
  Cartao SD montado com sucesso.  ```

---

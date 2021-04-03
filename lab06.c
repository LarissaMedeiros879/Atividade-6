/* Tamanho do Buffer */
#define MAX_BUFFER 5
/* Define a frequencia do clock. */
#define F_CPU 16000000UL 

/* Bibliotecas necessarias para o funcionamento do programa. A primeira e uma biblioteca basica e a segunda permite o uso do delay. */
#include <avr/io.h>
#include <util/delay.h>

unsigned char *ubrr0h; /* UBRR0X sao registradores para a definicao de baud rate, que e a velocidade com que a transmissao e feita. */
unsigned char *ubrr0l; /* UBRR0X sao registradores para a definicao de baud rate, que e a velocidade com que a transmissao e feita. */
unsigned char *ucsr0a; /* UCSR0X sao registradores de configuracao e status da USART */
unsigned char *ucsr0b; /* UCSR0X sao registradores de configuracao e status da USART */
unsigned char *ucsr0c; /* UCSR0X sao registradores de configuracao e status da USART */
char *udr0; /* Ponteiro do registrador de dados da USART0. Esta sendo definido como char pois recebera as strings de mensagem definidas como char. */
unsigned char *p_portc; /* Ponteiros associados aos registradores PORTC */
unsigned char *p_ddrc; /* Ponteiros associados aos registradores PORTC */

char informacao; /* Variável usada para armazenar o caractere recebido */
char msg_0[] = "Comando: Apagar todos os LEDs\n"; /* String de mensagem que aparece no monitor quando se recebe o caractere 0 */
char msg_1[] = "Comando: Varredura com um LED aceso\n"; /* String de mensagem que aparece no monitor quando se recebe o caractere 1 */
char msg_2[] = "Comando: Varredura com um LED apagado\n"; /* String de mensagem que aparece no monitor quando se recebe o caractere 2 */
char msg_3[] = "Comando: Acender todos os LEDs\n"; /* String de mensagem que aparece no monitor quando se recebe o caractere 3 */
char msg_outro[] = "Comando incorreto\n"; /* String de mensagem que aparece no monitor quando se recebe um caractere diferente de 0, 1, 2 ou 3 */
char msg_vazio[] = "Vazio!\n"; /* String de mensagem que aparece no monitor quando o buffer esta vazio */
char *m; /* Ponteiro associado as mensagens a serem exibidas no monitor */
int estado_a; /* Variavel que monitora o estado da maquina de estados de varredura com um led aceso */
int estado_b; /* Variavel que monitora o estado da maquina de estados de varredura com um led apagado */
char buf; /* Variavel que armazena o ultimo valor removido do buffer */
char comando; /* Variavel que armazena o ultimo comando valido lido do buffer */


/* Buffer */
volatile char buffer[MAX_BUFFER];
/* Controle de posicao para adicionar valores no buffer */
volatile char add_buf = 0;
/* Controle de posicao para remover valores do buffer */
volatile char del_buf = 0;
/* Numero de posicoes usadas no buffer */
volatile char ocup_buf = 0;

void adicionar_buffer(char c){
  
    /* Se o buffer nao esta cheio */
    if(ocup_buf < MAX_BUFFER){
        /* Adiciona valor no buffer */
        buffer[add_buf] = c;
        /* Incrementa o numero de posicoes utilizadas no buffer */
        ocup_buf++;
        /* Incrementa condicionalmente o controle de posicao para adicionar.
           Se esta na ultima posicao, retorna pra primeira.
           Caso contrario, vai pra posicao seguinte. */
        if(add_buf == (MAX_BUFFER-1)) add_buf=0;
        else                          add_buf++;
    }
}

/* Funcao para remover valores do buffer */
char remover_buffer(){
    
    /* variavel auxiliar para capturar o caractere do buffer */
    char c;
  
    /* Se o buffer nao esta vazio */
    if (ocup_buf > 0){
      
      /* Pega o caractere do buffer */
        c = buffer[del_buf];
        /* Decrementa o numero de posicoes utilizadas no buffer */
        ocup_buf--;        
        /* Incrementa condicionalmente o controle de posicao para remover.
           Se esta na ultima posicao, retorna pra primeira.
           Caso contrario, vai pra posicao seguinte. */
        if(del_buf == (MAX_BUFFER-1)) del_buf=0;
        else                        del_buf++;
    }
  
    return c;
}

/* Funcao que configura os perifericos. */
void setup() {
  
  cli();
 
  udr0 = (char *) 0xC6;
  /* Atribui aos ponteiros os endereços dos registradores UBRR0X. */
  ubrr0h = (unsigned char *) 0xC5;
  ubrr0l = (unsigned char *) 0xC4;
  /* Para um baud rate de 19,2 kbps, UBRR0X = 8. Em binario, 0000 0000 0000 1000. Os oito primeiros bits correspondem ao UBRR0H e os oito ultimos ao UBRR0L.  */
  *ubrr0h = 0b00000000;  /* Aqui, todos os bits estao sendo usados. Por isso esta sendo usado o = ao inves de uma mascara. */
  *ubrr0l = 0b01010001;  /* Aqui, todos os bits estao sendo usados. Por isso esta sendo usado o = ao inves de uma mascara. */
  /* Atribi aos ponteiros os endereços dos registradores UCSR0X. */
  ucsr0a = (unsigned char *) 0xC0; 
  ucsr0b = (unsigned char *) 0xC1;
  ucsr0c = (unsigned char *) 0xC2;
  /* Os bits do registrador UCSR0A correspondem a: 
  7 - RXCO: uma flag que indica que existem dados nao lidos no buffer de recepcao. E um bit a ser lido, logo nao e setado inicialmente. 
  6 - TXCO: uma flag setada quando todo o dado de transmissao foi enviado. E um bit a ser lido, logo nao e setado inicialmente. 
  5 - UDRE0: flag que indica se o buffer de transmissao esta pronto para receber novos dados. E um bit a ser lido, logo nao e setado inicialmente. 
  4, 3, 2 - FE0, DOR0, UPE0: flas que indicam, respectivamente, erro no frame recebido, overrun e erro de paridade. Tambem sao bits a serem lidos e nao sao setados inicialmente.
  1 - U2X0: quando igual a 1, a taxa de transmissao e dobrada. Como se quer uma velocidade de transmissao normal, ele e setado como 0.
  0 - MPCM0: ativa o modo multiprocessador. Como se deseja desabilita-lo, o bit e setado como 0. */
  *ucsr0a &= 0xFC;
  /* Os bits do registrador UCSR0B correspondem a:
  7, 6, 5 - RXCIE0, TXCIE0, UDRIE0: esses bits habilitam/desabilitam interrupcoes. Inicialmente, habilita-se a recepcao. Sera utilizada a interrupcao de buffer de transmissao vazio, habilitada ao longo do programa. Isso e feito para que nao sejam transmitidos dados indesejados, uma vez que o buffer de transmissao pode estar vazio, mas nao se deseja transmitir nenhuma mensagem. 
  4 - RXEN0: habilia/desabilita o receptor, logo e setado como um para habilita-lo, ja que o programa envolve uma recepcao. 
  3 - TXEN0: habilia/desabilita o transmissor, logo e setado como um para habilita-lo, ja que o programa envolve uma transmissao.
  2 - UCSZ02: esse bit, em conjunto com UCSZ01 e UCSZ00 selecionam quantos bits de dados serao transmitidos em cada frame. Como sao 8 bits, UCSZ0X e setado como 011. Logo, esse bit e setado como zero. 
  1, 0 - RXB80, TXB80: sao usados quando se trata de uma transmissao com 9 bits. Logo, nao sao utilizados nessa transmissao. */
  *ucsr0b &= 0x9B; /* Reseta os bits que devem ser iguais a 0. */
  *ucsr0b |= 0x98; /* Seta o bit que deve ser igual a 1. */
  /*Os bits do registrador UCSR0C correspondem a:
  7, 6 - UMSEL01, UMSEL00: definem o modo de operação da USART. Para que ela funcione em modo assincrono, os bits devem ser setados como 00.
  5, 4 - UPM01, UPM00: definem o uso (ou não) do bit de paridade e o seu tipo. Aqui, nao serao usados bits de paridade, logo devem ser setados como 00.
  3 - USBS0: esta relacionado aos bits de parada. E igual a 0 quando há um único bit de parada em cada frame e igual a 1 quando dois bits de parada são utilizados. Nessa transmissao, e setado como 0.
  2, 1 - UCSZ01, UCSZ00: esses bits, em conjunto com UCSZ02 selecionam quantos bits de dados serao transmitidos em cada frame. Como sao 8 bits, UCSZ0X e setado como 011. Logo, esses bits sao setados como 11.
  0 - UCPOL0: deve ser igual a zero para transmissao assíncrona.*/
  *ucsr0c = 0b00000110; /* Aqui, todos os bits estao sendo usados. Por isso esta sendo usado o = ao inves de uma mascara. */
  /* Atribui aos ponteiros os enderecos do registrador Port C. */
  p_portc = (unsigned char *) 0x28;
  p_ddrc = (unsigned char *) 0x27;
  *p_ddrc |= 0b00111000; /* Seta as portas PC5, PC4 e PC3 como saida */
  *p_portc &= ~0x38; /* Inicia com os leds desligados */
}

/* Rotina de interrupcao do tipo recepcao completa. E ativada quando um byte e recebido. Le o valor digitado no monitor. Esta sempre setada, pois deseja-se ler todos os comandos enviados pelo usuario. */
ISR(USART_RX_vect){

  informacao = *udr0; /* Armazena na variavel o valor do registrador udr0, o qual armazena o comando enviado pelo usuario. */
  adicionar_buffer(informacao); /* Adiciona no buffer o valor. */
   
}

/* Rotina de interrupcao do tipo buffer de transmissao vazio. E setada quando um proximo caractere pode ser enviado para transmissao. */
ISR(USART_UDRE_vect) {

  /* Se a mensagem ainda nao terminou de ser transmitida, um caracetere e enviado para o buffer de transmissao. */
  if(*m != '\0') {
  
    *udr0 = *m; /* Envia o caractere atual */
    m++; /* Aponta o ponteiro para o proximo caractere a ser enviado */
  }
  else {
  
    *ucsr0b &= 0xDF; /* Quando a mensagem termina de ser transmitida, a interrupcao e desabilitada para que nao sejam transmitidos valores indesejados */
    
    
  }
  
}


/* Funcao que define qual a forma de varredura dos leds a depender do comando dado */
void comando_led() {

  
  if (comando == '0'){  /* Varredura com todos os leds apagados */
    
    *p_portc &= ~0x38; /* Seta os bits 3, 4 e 5 como zero, apagando os leds. Esse comando se repete abaixo para garantir que apenas os leds setados em seguida serão acesos.  */
    _delay_ms(500); /* Espera 500 ms antes de executar um novo comando. Essa linha se repete abaixo para garantir que um novo estado so acontecera apos 500 ms. */
  }
  
  else if (comando ==  '1'){ /* Varredura com um led aceso. Sequência: 3, 4, 5, 4. */
  
      *p_portc &= ~0x38;
      *p_portc |= 0b00001000; /* Seta como 1 o bit 3, acendendo o led na porta PC3. */
      _delay_ms(500);
      *p_portc &= ~0x38;
      *p_portc |= 0b00010000; /* Seta como 1 o bit 4, acendendo o led na porta PC4. */
        _delay_ms(500);    
      *p_portc &= ~0x38;
      *p_portc |= 0b00100000; /* Seta como 1 o bit 5, acendendo o led na porta PC5. */
      _delay_ms(500); 
      *p_portc &= ~0x38;
      *p_portc |= 0b00010000; /* Seta como 1 o bit 4, acendendo o led na porta PC4. Assim, quando esse comando for dado novamente, iniciara com o led 3.  */
        _delay_ms(500);    
    
  }
  else if (comando == '2') { /* Varredura com um led apagado. Sequência: 3, 4, 5, 4 (apagado). */
  
      *p_portc &= ~0x38;
      *p_portc |= 0b00110000; /* Seta como 1 os bits 4 e 5. O led 3 foi apagado no comando anterior. */
      _delay_ms(500);
      *p_portc &= ~0x38;
      *p_portc |= 0b00101000; /* Seta como 1 os bits 3 e 5. O led 4 foi apagado no comando anterior. */
      _delay_ms(500);      
      *p_portc &= ~0x38;
      *p_portc |= 0b00011000; /* Seta como 1 os bits 3 e 4. O led 5 foi apagado no comando anterior. */
      _delay_ms(500);      
      *p_portc &= ~0x38;
      *p_portc |= 0b00101000; /* Seta como 1 os bits 3 e 5. O led 4 foi apagado no comando anterior. */
      _delay_ms(500);
  }
    
  else if (comando ==  '3'){ /* Varredura com todos os leds acesos */
  
    *p_portc |= 0x38; /* Seta os bits 3, 4 e 5 como 1. */
    _delay_ms(500);
  }
    
  
    
}

/* Funcao que verifica se o buffer tem algum dado */
void verifica_buffer() {


  if (ocup_buf > 0) { /* Se o buffer recebeu algum dado, a transmissao e feita de acordo com o comando recebido.  */
    
      buf = remover_buffer(); /* Remove do buffer o dado mais antigo e armazena na variavel buf.  */
      if ((buf == '0') || (buf == '1') || (buf == '2') || (buf == '3')) {

    
        comando = buf; /* Variavel atualizada apenas se um comando valido (0, 1, 2 ou 3) foi armazenado no buffer */
  }
      if (buf == '0') m = &(msg_0[0]); /* Aponta o ponteiro para a mensagem a ser enviada dependendo do comando */
      else if (buf == '1') m = &(msg_1[0]);
      else if (buf == '2') m = &(msg_2[0]);
      else if (buf == '3') m = &(msg_3[0]);
      else m = &(msg_outro[0]);
      
    }
  else m = &(msg_vazio[0]); /* Se o buffer esta vazio, envio a mensagem de buffer vazio */
  *ucsr0b |= 0x20; /* Apos remover do buffer um comando, ativo a interrupcao para a transmissao da mensagem associada a esse comando. Se o buffer estiver vazio, a mensagem de buffer vario e enviada. Nesse caso, o buffer so e ativado se existe uma mensagem a ser transmitida, apos ser feita a varredura do buffer.  */
  
  
}

int main() {

  setup(); /* Inicializa os perifericos */ 
  comando = '0'; /* Inicializa o comando como zero para iniciar a funcao comando_led(). */
  sei(); /* Ativa as interrupcoes */
  
  while(1) { /* Looping infinito */


    verifica_buffer(); /* Verifica o estado do buffer antes de executar o comando e transmitir a mensagem.  */
    comando_led(); /* Executa o comando dado. */
  }  
  
  return 0;

}




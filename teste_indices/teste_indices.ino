long int controle[] = {0xFF00BF00, 0xFE01BF00, 0xFD02BF00, 0xFB04BF00, 0xFA05BF00};
int lengthOfControle = sizeof(controle) / sizeof(controle[0]);
int i = 0;

void setup() {
  Serial.begin(9600);
  Serial.print("Tamanho do vetor: "); Serial.println(lengthOfControle);
}

void loop() {
  int index = findIndex(controle, controle[i]);
  Serial.print("Índice: "); Serial.println(index); Serial.println("");

  i++;
  delay(1000);
}

int findIndex(long int *items, long int item) {
  int i;
  for(i = 0; i < lengthOfControle; i++) {
    if(controle[i] == item) {
      return i;
    }
  }
  return -1;
}

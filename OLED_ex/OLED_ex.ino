#include <Wire.h>
#include <OLED32.h>

OLED display(4, 5); //display(SDA, SCL), Declare OLED display

void setup() {
  // put your setup code here, to run once:
  display.begin();
  display.print("Hello World");
  display.print("Welcome to IOT Class!!!", 3, 1); //원하는 위치 print
  delay(2000);
  display.off();
  delay(2000);
  display.on();
}

void loop() {
  // put your main code here, to run repeatedly:

}

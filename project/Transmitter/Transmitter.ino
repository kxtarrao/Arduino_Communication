// Libraries
#include <LiquidCrystal.h>

// Pin Assignments
#define   tx_data             3
#define   tx_clock            4
#define   tx_request_resend   5

#define   lcd_d4              7  
#define   lcd_d5              8 
#define   lcd_d6              9
#define   lcd_d7              10 
#define   lcd_rs              11
#define   lcd_en              12

// Data Rate ( 1 / (Time per Bit) )
#define tx_rate 5

// Message and Byte Variables
String message      = ""  ;
char char_in        = 0   ;

// LDC Object
LiquidCrystal LCD_T(lcd_rs,lcd_en,lcd_d4,lcd_d5,lcd_d6,lcd_d7);

// Byte Parity Error Check Variables 0=Even / 1=Odd
bool byte_parity    = false ;
int confirm_resend  = 0     ;

// Checksum Error Check Variables
int checksum = 0;
int resend_message = 0;

// =================================================================================
void setup() 
{
    // LCD Initialize 
    LCD_T.begin(16,2);
    
    // Serial Initialize
    Serial.begin(9600);

    // Pin Mode
    pinMode(tx_data         , OUTPUT);
    pinMode(tx_clock        , OUTPUT);
    pinMode(tx_request_resend  , INPUT);

    // Initialize Clock LOW
    digitalWrite( tx_clock  , LOW     );
}
// =================================================================================
void loop()
{
    // --------------------------------------
    // RECEIVE AND DISPLAY MESSAGE TO BE SENT
    // --------------------------------------
    
    // Wait Until Serial Data Exists
    while(  Serial.available() == 0 )
    {
        LCD_T.setCursor(0,0);
        LCD_T.print("Serial Input Req");
        delay(1000);
        LCD_CLEAR_TOP();
        delay(1000);
    }
    while( Serial.available() )
    {
        char_in = Serial.read();
        if(char_in >= 32 && char_in <= 127)
        {
            message += char_in;
        }
    }
    
    // --------------------------------------
    // TRANSMIT DATA
    // --------------------------------------

    // Checksum Loop (Repeat message if Checksum is Wrong)
    do
    {   
        // Print Message on Top Line
        LCD_CLEAR_ALL();
        LCD_T.setCursor(0,0);
        LCD_T.print(message);
        
        // Select Character in String
        for(  int byte_i = 0  ; byte_i < message.length()  ; byte_i++  )
        {        
            // Get Character in Message
            char tx_byte = message[byte_i];
    
            // Clear Byte Line
            LCD_T.noCursor();
            LCD_CLEAR_BYTE();
            LCD_T.setCursor(byte_i,0);
            LCD_T.cursor();
    
            // Reset Byte Parity
            byte_parity = false;
            
            // Select Bits in Character (c1-c7) --> (b0-b6)
            for(  int bit_i = 1 ; bit_i < 8 ; bit_i++ )
            {
                // Get Data Transmit Bit
                bool tx_bit = tx_byte & (0x80 >> bit_i);
    
                // Update Byte Parity
                if(tx_bit == true){byte_parity = !byte_parity;}
                
                // Print Data Bit on LDC
                LCD_T.noCursor();
                LCD_T.setCursor(bit_i-1,1);
                LCD_T.print(tx_bit ? "1":"0");
                LCD_T.setCursor(byte_i,0);
                LCD_T.cursor();
                
                // Transmit Data Bit (Flash Light)
                digitalWrite( tx_clock  , HIGH    );
                digitalWrite( tx_data   , tx_bit  );
                delay(1000  / (2*tx_rate) );
                digitalWrite( tx_clock  , LOW     );
                delay(1000  / (2*tx_rate) );
            }
            
            // Send Data Parity Bit (b7)
            LCD_T.noCursor();
            LCD_T.setCursor(7,1);
            LCD_T.print(byte_parity ? "1":"0");
            LCD_T.setCursor(byte_i,0);
            LCD_T.cursor();
                
            // Transmit Parity Bit (Flash Light)
            digitalWrite( tx_clock  , HIGH    );
            digitalWrite( tx_data   , byte_parity  );
            
            // Resend Byte?
            confirm_resend = 0;
            for(int i = 0;i<10;i++)
            {
                if(digitalRead(tx_request_resend) == HIGH){confirm_resend = 1;}
                delay(1);
            }
            // If Byte Needs To Be Resent
            if(confirm_resend == 1)
            {
                byte_i--;
                LCD_T.setCursor(0,0);
            }
            // If Byte Is Confirmed, Add to Checksum
            else
            {
                int tx_char = tx_byte;
                checksum += tx_char;  
            }
            
            // FINISH Transmit Parity Bit (Flash Light)
            delay(  1000  / (2*tx_rate) );
            digitalWrite( tx_clock  , LOW     );
            delay(  1000  / (2*tx_rate) );
        }
    
        // --------------------------------------
        // Send 0 Byte in Preperation for Checksum
        // --------------------------------------
    
        // Parity Loop for Checksum Byte
        do
        {
            // Clear Byte Line
            LCD_T.noCursor();
            LCD_CLEAR_BYTE();
             
            // Reset Byte Parity
            byte_parity = false;
            
            for(  int bit_i = 0 ; bit_i < 7 ; bit_i++ )
            {
                // Get 0 Transmit Bit
                bool tx_bit = false;
        
                // Update Byte Parity
                if(tx_bit == true){byte_parity = !byte_parity;}
                
                // Print Data Bit on LDC
                LCD_T.noCursor();
                LCD_T.setCursor(bit_i,1);
                LCD_T.print(tx_bit ? "1":"0");
                
                // Transmit Data Bit (Flash Light)
                digitalWrite( tx_clock  , HIGH    );
                digitalWrite( tx_data   , tx_bit  );
                delay(1000  / (2*tx_rate) );
                digitalWrite( tx_clock  , LOW     );
                delay(1000  / (2*tx_rate) );
            }
            // Reset Confirm Resend
            confirm_resend == 0;
            
            // Send Parity Bit for CheckSum
            LCD_T.noCursor();
            LCD_T.setCursor(7,1);
            LCD_T.print(byte_parity ? "1":"0");
                
            // Transmit Parity Bit for CheckSum (Flash Light)
            digitalWrite( tx_clock  , HIGH    );
            digitalWrite( tx_data   , byte_parity  );
            
            // Resend Byte?
            confirm_resend = 0;
            for(int i = 0;i<10;i++)
            {
                if(digitalRead(tx_request_resend) == HIGH){confirm_resend = 1;}
                delay(1);
            }
            
            // FINISH Transmit Parity Bit (Flash Light)
            delay(  1000  / (2*tx_rate) );
            digitalWrite( tx_clock  , LOW     );
            delay(  1000  / (2*tx_rate) );
            
        }while(confirm_resend == 1);
         
        // --------------------------------------
        // Send Checksum 
        // --------------------------------------
        
        // Convert Checksum Into 7 Bits Using Ones Compliment
        checksum = checksum % 127;
    
        // Parity Loop for Checksum Byte
        do 
        {
            // Clear Byte Line
            LCD_T.noCursor();
            LCD_CLEAR_BYTE();
    
            // Reset Byte Parity
            byte_parity = false;
            
            // Send Bits in CheckSum
            for(  int bit_i = 1 ; bit_i <= 7 ; bit_i++ )
            {
                // Get Data Transmit Bit
                bool tx_bit = checksum & (0x80 >> bit_i);
        
                // Update Byte Parity
                if(tx_bit == true){byte_parity = !byte_parity;}
                
                // Print Data Bit on LDC
                LCD_T.noCursor();
                LCD_T.setCursor(bit_i-1,1);
                LCD_T.print(tx_bit ? "1":"0");
                
                // Transmit Data Bit (Flash Light)
                digitalWrite( tx_clock  , HIGH    );
                digitalWrite( tx_data   , tx_bit  );
                delay(1000  / (2*tx_rate) );
                digitalWrite( tx_clock  , LOW     );
                delay(1000  / (2*tx_rate) );
            }
    
            // Reset Confirm Resend
            confirm_resend == 0;
            
            // Send Parity Bit for CheckSum
            LCD_T.noCursor();
            LCD_T.setCursor(7,1);
            LCD_T.print(byte_parity ? "1":"0");
                
            // Transmit Parity Bit for CheckSum (Flash Light)
            digitalWrite( tx_clock  , HIGH    );
            digitalWrite( tx_data   , byte_parity  );
            
            // Resend Byte?
            confirm_resend = 0;
            for(int i = 0;i<10;i++)
            {
                if(digitalRead(tx_request_resend) == HIGH){confirm_resend = 1;}
                delay(1);
            }
            
            // Transmit Parity Bit LOW (Flash Light)
            delay(  1000  / (2*tx_rate) );
            digitalWrite( tx_clock  , LOW     );
            
            resend_message = 0;
            for(int i = 0;i<10;i++)
            {
                if(digitalRead(tx_request_resend) == HIGH){resend_message = 1;}
                delay(1);
            }
            
            // Finish Transmit Parity Bit (Flash Light)
            delay(  1000  / (2*tx_rate) );
            
        }while(confirm_resend == 1);
        
        // --------------------------------------
        // CLEAN UP
        // --------------------------------------
        
        // Clean Up
        LCD_CLEAR_ALL();
        digitalWrite(tx_data   , LOW  );
        checksum = 0;   

        if(resend_message == 0)
        {
            message = "";
        }
        
    }while(resend_message == 1);
}
// =================================================================================

// --------------------------------------
// LCD Clearing Functions
// --------------------------------------
void LCD_CLEAR_ALL()
{
    LCD_T.setCursor(0,0);
    LCD_T.print("                ");
    LCD_T.setCursor(0,1);
    LCD_T.print("                ");
}
void LCD_CLEAR_TOP()
{
    LCD_T.setCursor(0,0);
    LCD_T.print("                ");
}
void LCD_CLEAR_BOTTOM()
{
    LCD_T.setCursor(0,1);
    LCD_T.print("                ");
}
void LCD_CLEAR_BYTE()
{
    LCD_T.setCursor(0,1);
    LCD_T.print("        ");
}
